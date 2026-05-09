#include "external_platform_manager.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHostAddress>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QTcpSocket>
#include <QUrlQuery>
#include <QUuid>

#include "infrastructure/logging/logger.h"

namespace {

QString statusText(int statusCode) {
    switch (statusCode) {
    case 200:
        return QStringLiteral("OK");
    case 400:
        return QStringLiteral("Bad Request");
    case 401:
        return QStringLiteral("Unauthorized");
    case 404:
        return QStringLiteral("Not Found");
    case 405:
        return QStringLiteral("Method Not Allowed");
    default:
        return QStringLiteral("OK");
    }
}

QByteArray textResponseBody(const QString& text) {
    return text.toUtf8();
}

}

namespace clawpp {

ExternalPlatformManager::ExternalPlatformManager(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_tgPollTimer(new QTimer(this))
    , m_tgLastUpdateId(0)
    , m_tgIsRunning(false)
    , m_fsTokenExpireEpoch(0)
    , m_fsIsRunning(false)
    , m_fsServer(new QTcpServer(this)) {
    connect(m_tgPollTimer, &QTimer::timeout, this, &ExternalPlatformManager::onTelegramPoll);
    connect(m_fsServer, &QTcpServer::newConnection, this, &ExternalPlatformManager::onFeishuNewConnection);
}

ExternalPlatformManager::~ExternalPlatformManager() {
    stopTelegramBot();
    stopFeishuBot();
}

void ExternalPlatformManager::startTelegramBot(const QString& token) {
    if (token.isEmpty()) {
        return;
    }

    m_tgToken = token;
    m_tgIsRunning = true;
    m_tgLastUpdateId = 0;

    m_tgPollTimer->start(2000);

    emit statusChanged(QStringLiteral("Telegram"), true);
}

void ExternalPlatformManager::stopTelegramBot() {
    m_tgIsRunning = false;
    m_tgPollTimer->stop();
    emit statusChanged(QStringLiteral("Telegram"), false);
}

void ExternalPlatformManager::startFeishuBot(const QString& appId,
                                             const QString& appSecret,
                                             const QString& verificationToken,
                                             int port) {
    if (appId.isEmpty() || appSecret.isEmpty() || verificationToken.isEmpty()) {
        emit statusChanged(QStringLiteral("Feishu"), false, QStringLiteral("飞书应用 ID、App Secret 和 Verification Token 不能为空。"));
        return;
    }

    m_fsAppId = appId;
    m_fsAppSecret = appSecret;
    m_fsVerificationToken = verificationToken;
    m_fsTenantAccessToken.clear();
    m_fsTokenExpireEpoch = 0;

    if (m_fsServer->isListening()) {
        m_fsServer->close();
    }

    if (m_fsServer->listen(QHostAddress::Any, port)) {
        m_fsIsRunning = true;
        emit statusChanged(QStringLiteral("Feishu"), true);
        LOG_INFO(QStringLiteral("飞书回调服务已启动，监听端口 %1").arg(port));
    } else {
        m_fsIsRunning = false;
        emit statusChanged(QStringLiteral("Feishu"), false, QStringLiteral("无法启动飞书回调服务，端口 %1 被占用或无权限。").arg(port));
    }
}

void ExternalPlatformManager::stopFeishuBot() {
    m_fsIsRunning = false;
    m_fsPendingBuffers.clear();
    m_fsTenantAccessToken.clear();
    m_fsTokenExpireEpoch = 0;
    if (m_fsServer->isListening()) {
        m_fsServer->close();
    }
    emit statusChanged(QStringLiteral("Feishu"), false);
}

void ExternalPlatformManager::updateFeishuTenantToken(std::function<void(bool)> callback) {
    if (m_fsAppId.isEmpty() || m_fsAppSecret.isEmpty()) {
        if (callback) {
            callback(false);
        }
        return;
    }

    QNetworkRequest request{QUrl("https://open.feishu.cn/open-apis/auth/v3/tenant_access_token/internal")};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");

    QJsonObject payload;
    payload["app_id"] = m_fsAppId;
    payload["app_secret"] = m_fsAppSecret;

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, [this, reply, callback]() {
        bool success = false;
        QString errorMessage;

        if (reply->error() == QNetworkReply::NoError) {
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
            if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
                QJsonObject root = doc.object();
                if (root.value("code").toInt() == 0) {
                    m_fsTenantAccessToken = root.value("tenant_access_token").toString();
                    int expireSeconds = root.value("expire").toInt(7200);
                    m_fsTokenExpireEpoch = QDateTime::currentSecsSinceEpoch() + qMax(60, expireSeconds - 60);
                    success = true;
                } else {
                    errorMessage = root.value("msg").toString(QStringLiteral("飞书 tenant_access_token 请求失败"));
                }
            } else {
                errorMessage = QStringLiteral("飞书 tenant_access_token 响应解析失败");
            }
        } else {
            errorMessage = reply->errorString();
        }

        if (!success) {
            LOG_WARN(QStringLiteral("刷新飞书 tenant_access_token 失败：%1").arg(errorMessage));
        }

        if (callback) {
            callback(success);
        }
        reply->deleteLater();
    });
}

void ExternalPlatformManager::sendFeishuMessage(const QString& userId, const QString& text) {
    if (userId.isEmpty() || text.isEmpty()) {
        return;
    }

    QJsonObject payload;
    payload["receive_id"] = userId;
    payload["msg_type"] = "text";
    payload["content"] = QString::fromUtf8(buildFeishuContent(text));
    payload["uuid"] = QUuid::createUuid().toString(QUuid::WithoutBraces);

    sendFeishuMessageInternal("/im/v1/messages?receive_id_type=open_id", payload);
}

void ExternalPlatformManager::sendFeishuReply(const QString& messageId, const QString& text) {
    if (messageId.isEmpty() || text.isEmpty()) {
        return;
    }

    QJsonObject payload;
    payload["msg_type"] = "text";
    payload["content"] = QString::fromUtf8(buildFeishuContent(text));
    payload["reply_in_thread"] = true;
    payload["uuid"] = QUuid::createUuid().toString(QUuid::WithoutBraces);

    sendFeishuMessageInternal(QString("/im/v1/messages/%1/reply").arg(messageId), payload);
}

void ExternalPlatformManager::sendFeishuMessageInternal(const QString& endpoint,
                                                        const QJsonObject& payload,
                                                        std::function<void(bool)> callback) {
    auto sendNow = [this, endpoint, payload, callback]() {
        QNetworkRequest request{QUrl(QStringLiteral("https://open.feishu.cn/open-apis") + endpoint)};
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
        request.setRawHeader("Authorization", QString("Bearer %1").arg(m_fsTenantAccessToken).toUtf8());

        QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
        connect(reply, &QNetworkReply::finished, [this, reply, endpoint, callback]() {
            bool success = false;
            QString errorMessage;

            if (reply->error() == QNetworkReply::NoError) {
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
                if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
                    QJsonObject root = doc.object();
                    if (root.value("code").toInt() == 0) {
                        success = true;
                    } else {
                        errorMessage = root.value("msg").toString(QStringLiteral("飞书消息发送失败"));
                    }
                } else {
                    errorMessage = QStringLiteral("飞书消息发送响应解析失败");
                }
            } else {
                errorMessage = reply->errorString();
            }

            if (!success) {
                LOG_WARN(QStringLiteral("飞书消息发送失败[%1]：%2").arg(endpoint, errorMessage));
            }

            if (callback) {
                callback(success);
            }
            reply->deleteLater();
        });
    };

    if (m_fsTenantAccessToken.isEmpty() || QDateTime::currentSecsSinceEpoch() >= m_fsTokenExpireEpoch) {
        updateFeishuTenantToken([this, sendNow, callback](bool success) mutable {
            if (success) {
                sendNow();
            } else if (callback) {
                callback(false);
            }
        });
    } else {
        sendNow();
    }
}

void ExternalPlatformManager::sendTelegramMessage(const QString& userId, const QString& text) {
    if (m_tgToken.isEmpty() || userId.isEmpty() || text.isEmpty()) {
        return;
    }

    QNetworkRequest request{QUrl("https://api.telegram.org/bot" + m_tgToken + "/sendMessage")};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject payload;
    payload["chat_id"] = userId;
    payload["text"] = text;

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
}

void ExternalPlatformManager::onTelegramPoll() {
    if (!m_tgIsRunning || m_tgToken.isEmpty()) {
        return;
    }

    QString url = QString("https://api.telegram.org/bot%1/getUpdates?offset=%2&timeout=1")
                      .arg(m_tgToken)
                      .arg(m_tgLastUpdateId + 1);

    QNetworkRequest request{QUrl(url)};
    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        onTelegramReplyFinished(reply);
    });
}

void ExternalPlatformManager::onTelegramReplyFinished(QNetworkReply* reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            if (root.value("ok").toBool()) {
                QJsonArray result = root.value("result").toArray();
                for (const QJsonValue& val : result) {
                    QJsonObject update = val.toObject();
                    qint64 updateId = update.value("update_id").toVariant().toLongLong();
                    if (updateId > m_tgLastUpdateId) {
                        m_tgLastUpdateId = updateId;
                    }

                    if (update.contains("message")) {
                        QJsonObject msg = update.value("message").toObject();
                        QString text = msg.value("text").toString();
                        QJsonObject from = msg.value("from").toObject();
                        QString userId = QString::number(from.value("id").toVariant().toLongLong());

                        if (!text.isEmpty() && !userId.isEmpty()) {
                            emit externalMessageReceived(QStringLiteral("Telegram"), userId, text, QString());
                        }
                    }
                }
            }
        }
    }
    reply->deleteLater();
}

void ExternalPlatformManager::onFeishuNewConnection() {
    while (m_fsServer->hasPendingConnections()) {
        QTcpSocket* client = m_fsServer->nextPendingConnection();
        m_fsPendingBuffers.insert(client, QByteArray());
        connect(client, &QTcpSocket::readyRead, this, [this, client]() {
            onFeishuReadyRead(client);
        });
        connect(client, &QTcpSocket::disconnected, this, [this, client]() {
            m_fsPendingBuffers.remove(client);
            client->deleteLater();
        });
    }
}

void ExternalPlatformManager::onFeishuReadyRead(QTcpSocket* client) {
    if (!client) {
        return;
    }

    QByteArray& buffer = m_fsPendingBuffers[client];
    buffer.append(client->readAll());

    const int headerEnd = buffer.indexOf("\r\n\r\n");
    if (headerEnd < 0) {
        return;
    }

    QByteArray headerPart = buffer.left(headerEnd);
    QMap<QString, QString> headers = parseHeaders(headerPart);
    bool ok = false;
    int contentLength = headerValue(headers, QStringLiteral("content-length")).toInt(&ok);
    if (!ok || contentLength < 0) {
        contentLength = 0;
    }

    const int totalLength = headerEnd + 4 + contentLength;
    if (buffer.size() < totalLength) {
        return;
    }

    QByteArray requestData = buffer.left(totalLength);
    buffer.remove(0, totalLength);

    if (handleFeishuRequest(client, requestData)) {
        m_fsPendingBuffers.remove(client);
    }
}

bool ExternalPlatformManager::handleFeishuRequest(QTcpSocket* client, const QByteArray& requestData) {
    const int requestLineEnd = requestData.indexOf("\r\n");
    if (requestLineEnd < 0) {
        sendHttpResponse(client, 400, textResponseBody(QStringLiteral("无效的请求行")));
        return true;
    }

    const QByteArray requestLine = requestData.left(requestLineEnd);
    const QList<QByteArray> requestParts = requestLine.split(' ');
    if (requestParts.size() < 2) {
        sendHttpResponse(client, 400, textResponseBody(QStringLiteral("无效的 HTTP 请求")));
        return true;
    }

    const QString method = QString::fromUtf8(requestParts[0]).trimmed().toUpper();
    const int headerEnd = requestData.indexOf("\r\n\r\n");
    const QByteArray headerBytes = requestData.left(headerEnd);
    const QByteArray body = requestData.mid(headerEnd + 4);
    const QMap<QString, QString> headers = parseHeaders(headerBytes);

    if (method == QStringLiteral("GET")) {
        sendHttpResponse(client, 200, textResponseBody(QStringLiteral("success")));
        return true;
    }

    if (method != QStringLiteral("POST")) {
        sendHttpResponse(client, 405, textResponseBody(QStringLiteral("仅支持 POST 请求")));
        return true;
    }

    const QString signature = headerValue(headers, QStringLiteral("x-lark-signature"));
    const QString timestamp = headerValue(headers, QStringLiteral("x-lark-request-timestamp"));
    const QString nonce = headerValue(headers, QStringLiteral("x-lark-request-nonce"));

    if (!m_fsAppSecret.isEmpty()) {
        if (signature.isEmpty() || timestamp.isEmpty() || nonce.isEmpty()) {
            sendHttpResponse(client, 401, textResponseBody(QStringLiteral("缺少飞书签名头")));
            return true;
        }

        if (!validateFeishuSignature(headers, body)) {
            sendHttpResponse(client, 401, textResponseBody(QStringLiteral("签名校验失败")));
            return true;
        }
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        sendHttpResponse(client, 400, textResponseBody(QStringLiteral("事件体解析失败")));
        return true;
    }

    const QJsonObject root = doc.object();
    const QJsonObject header = root.value("header").toObject();
    const QString verificationToken = header.value("token").toString();

    if (!m_fsVerificationToken.isEmpty() && !verificationToken.isEmpty() && verificationToken != m_fsVerificationToken) {
        sendHttpResponse(client, 401, textResponseBody(QStringLiteral("Verification Token 不匹配")));
        return true;
    }

    if (root.value("type").toString() == QStringLiteral("url_verification")) {
        const QString challenge = root.value("challenge").toString();
        QJsonObject response;
        response["challenge"] = challenge;
        sendHttpResponse(client, 200, QJsonDocument(response).toJson(QJsonDocument::Compact), "application/json; charset=utf-8");
        return true;
    }

    if (root.value("schema").toString() == QStringLiteral("2.0")) {
        const QJsonObject event = root.value("event").toObject();
        if (event.value("type").toString() == QStringLiteral("im.message.receive_v1")) {
            const QJsonObject message = event.value("message").toObject();
            if (message.value("message_type").toString() == QStringLiteral("text")) {
                const QString contentStr = message.value("content").toString();
                const QJsonDocument contentDoc = QJsonDocument::fromJson(contentStr.toUtf8());
                const QString realText = contentDoc.object().value("text").toString();

                const QJsonObject sender = event.value("sender").toObject();
                const QJsonObject senderId = sender.value("sender_id").toObject();
                QString userId = senderId.value("open_id").toString();
                if (userId.isEmpty()) {
                    userId = sender.value("open_id").toString();
                }

                const QString messageId = message.value("message_id").toString();
                if (!realText.isEmpty() && !userId.isEmpty()) {
                    emit externalMessageReceived(QStringLiteral("Feishu"), userId, realText, messageId);
                }
            }
        }
    }

    sendHttpResponse(client, 200, textResponseBody(QStringLiteral("success")));
    return true;
}

bool ExternalPlatformManager::validateFeishuSignature(const QMap<QString, QString>& headers, const QByteArray& body) const {
    if (m_fsAppSecret.isEmpty()) {
        return true;
    }

    const QString signature = headerValue(headers, QStringLiteral("x-lark-signature"));
    const QString timestamp = headerValue(headers, QStringLiteral("x-lark-request-timestamp"));
    const QString nonce = headerValue(headers, QStringLiteral("x-lark-request-nonce"));
    if (signature.isEmpty() || timestamp.isEmpty() || nonce.isEmpty()) {
        return false;
    }

    const QByteArray payload = timestamp.toUtf8() + '\n' + nonce.toUtf8() + '\n' + body;
    const QByteArray expectedSignature = hmacSha256(m_fsAppSecret.toUtf8(), payload);
    return QString::fromUtf8(expectedSignature) == signature;
}

QByteArray ExternalPlatformManager::hmacSha256(const QByteArray& key, const QByteArray& message) const {
    constexpr int blockSize = 64;
    QByteArray normalizedKey = key;
    if (normalizedKey.size() > blockSize) {
        normalizedKey = QCryptographicHash::hash(normalizedKey, QCryptographicHash::Sha256);
    }
    if (normalizedKey.size() < blockSize) {
        normalizedKey.append(QByteArray(blockSize - normalizedKey.size(), '\0'));
    }

    QByteArray oKeyPad(blockSize, '\x5c');
    QByteArray iKeyPad(blockSize, '\x36');
    for (int i = 0; i < blockSize; ++i) {
        oKeyPad[i] = static_cast<char>(oKeyPad[i] ^ normalizedKey[i]);
        iKeyPad[i] = static_cast<char>(iKeyPad[i] ^ normalizedKey[i]);
    }

    QByteArray inner = QCryptographicHash::hash(iKeyPad + message, QCryptographicHash::Sha256);
    QByteArray outer = QCryptographicHash::hash(oKeyPad + inner, QCryptographicHash::Sha256);
    return outer.toBase64();
}

QMap<QString, QString> ExternalPlatformManager::parseHeaders(const QByteArray& requestHead) {
    QMap<QString, QString> headers;
    const QList<QByteArray> lines = requestHead.split('\n');
    for (int i = 1; i < lines.size(); ++i) {
        QByteArray line = lines[i].trimmed();
        if (line.endsWith('\r')) {
            line.chop(1);
        }
        const int colonIndex = line.indexOf(':');
        if (colonIndex <= 0) {
            continue;
        }

        const QString key = QString::fromUtf8(line.left(colonIndex)).trimmed().toLower();
        const QString value = QString::fromUtf8(line.mid(colonIndex + 1)).trimmed();
        headers.insert(key, value);
    }
    return headers;
}

QString ExternalPlatformManager::headerValue(const QMap<QString, QString>& headers, const QString& key) {
    return headers.value(key.trimmed().toLower());
}

QByteArray ExternalPlatformManager::httpResponse(int statusCode, const QByteArray& body, const QByteArray& contentType) {
    QByteArray response;
    response += "HTTP/1.1 ";
    response += QByteArray::number(statusCode);
    response += ' ';
    response += statusText(statusCode).toUtf8();
    response += "\r\n";
    response += "Content-Type: ";
    response += contentType;
    response += "\r\n";
    response += "Content-Length: ";
    response += QByteArray::number(body.size());
    response += "\r\nConnection: close\r\n\r\n";
    response += body;
    return response;
}

void ExternalPlatformManager::sendHttpResponse(QTcpSocket* client,
                                                int statusCode,
                                                const QByteArray& body,
                                                const QByteArray& contentType) {
    if (!client) {
        return;
    }

    client->write(httpResponse(statusCode, body, contentType));
    client->flush();
    client->disconnectFromHost();
}

QByteArray ExternalPlatformManager::buildFeishuContent(const QString& text) const {
    QJsonObject content;
    content["text"] = text;
    return QJsonDocument(content).toJson(QJsonDocument::Compact);
}

} // namespace clawpp
