#include "http_client.h"
#include "infrastructure/logging/logger.h"
#include <QUrl>
#include <QSharedPointer>

namespace {
constexpr int kMaxRetryAttempts = 2;
constexpr int kInitialRetryDelayMs = 1000;
constexpr int kMaxRetryDelayMs = 15000;
}

namespace clawpp {

HttpClient::HttpClient(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_timeoutMs(60000)
    , m_requestGeneration(0) {
}

void HttpClient::setTimeout(int timeoutMs) {
    m_timeoutMs = timeoutMs;
}

void HttpClient::setBaseUrl(const QString& baseUrl) {
    m_baseUrl = baseUrl;
    if (m_baseUrl.endsWith('/')) {
        m_baseUrl.chop(1);
    }
}

void HttpClient::setApiKey(const QString& apiKey) {
    m_apiKey = apiKey;
}

void HttpClient::post(const QString& endpoint,
                      const QJsonObject& body,
                      std::function<void(const QByteArray&)> onSuccess,
                      std::function<void(const QString&)> onError) {
    const quint64 requestGeneration = m_requestGeneration;
    QMetaObject::invokeMethod(this, [this, endpoint, body, requestGeneration, onSuccess = std::move(onSuccess), onError = std::move(onError)]() {
        postImpl(endpoint, body, 0, requestGeneration, std::move(onSuccess), std::move(onError));
    }, Qt::QueuedConnection);
}

void HttpClient::postStream(const QString& endpoint,
                            const QJsonObject& body,
                            std::function<void(const QByteArray&)> onData,
                            std::function<void()> onComplete,
                            std::function<void(const QString&)> onError) {
    const quint64 requestGeneration = m_requestGeneration;
    QMetaObject::invokeMethod(this, [this, endpoint, body, requestGeneration, onData = std::move(onData), onComplete = std::move(onComplete), onError = std::move(onError)]() {
        postStreamImpl(endpoint, body, 0, requestGeneration, std::move(onData), std::move(onComplete), std::move(onError));
    }, Qt::QueuedConnection);
}

void HttpClient::postImpl(const QString& endpoint,
                          const QJsonObject& body,
                          int attempt,
                          quint64 requestGeneration,
                          std::function<void(const QByteArray&)> onSuccess,
                          std::function<void(const QString&)> onError) {
    if (requestGeneration != m_requestGeneration) {
        return;
    }

    QString ep = endpoint;
    if (ep.startsWith('/') && !m_baseUrl.isEmpty()) {
        ep = ep.mid(1);
    }
    QString fullUrl = m_baseUrl + "/" + ep;
    QUrl url(fullUrl);
    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());

    QJsonDocument doc(body);
    QByteArray data = doc.toJson();

    QNetworkReply* reply = m_networkManager->post(request, data);
    m_activeReplies.insert(reply);
    connect(reply, &QObject::destroyed, this, [this](QObject* obj) {
        m_activeReplies.remove(static_cast<QNetworkReply*>(obj));
    });

    QPointer<QTimer> timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(m_timeoutMs);

    // 通过共享标记保证超时回调与 finished 回调只会有一个生效。
    QSharedPointer<bool> handled(new bool(false));

    connect(timer.data(), &QTimer::timeout, [reply, timer, onError, handled]() {
        if (*handled) {
            return;
        }

        if (reply->property("clawpp_aborted").toBool()) {
            *handled = true;
            if (timer) {
                timer->deleteLater();
            }
            return;
        }

        *handled = true;
        reply->abort();
        if (timer) {
            timer->deleteLater();
        }
        onError(QStringLiteral("请求超时"));
    });

    connect(reply, &QNetworkReply::finished, [this, reply, timer, onSuccess, onError, attempt, body, endpoint, requestGeneration, handled]() {
        if (timer) {
            timer->stop();
            timer->deleteLater();
        }

        if (*handled) {
            m_activeReplies.remove(reply);
            reply->deleteLater();
            return;
        }

        if (reply->property("clawpp_aborted").toBool()) {
            *handled = true;
            m_activeReplies.remove(reply);
            reply->deleteLater();
            return;
        }

        const int statusCode = httpStatusCode(reply);
        const QByteArray responseData = reply->readAll();
        const QString defaultError = reply->errorString();
        QString errorStr = responseData.isEmpty() ? defaultError : parseErrorResponse(responseData, defaultError);

        if ((reply->error() == QNetworkReply::NoError || statusCode > 0)
            && statusCode >= 200 && statusCode < 300) {
            *handled = true;
            onSuccess(responseData);
            m_activeReplies.remove(reply);
            reply->deleteLater();
            return;
        }

        if (isRetryableStatusCode(statusCode) && attempt < kMaxRetryAttempts) {
            const int delayMs = retryDelayMs(reply, attempt);
            LOG_WARN(QStringLiteral("HTTP %1 请求被限流或暂时失败，%2 毫秒后重试（第 %3/%4 次）。")
                         .arg(statusCode)
                         .arg(delayMs)
                         .arg(attempt + 1)
                         .arg(kMaxRetryAttempts));
            *handled = true;
            QTimer::singleShot(delayMs, this, [=]() {
                postImpl(endpoint, body, attempt + 1, requestGeneration, onSuccess, onError);
            });
            m_activeReplies.remove(reply);
            reply->deleteLater();
            return;
        }

        if (errorStr.isEmpty()) {
            errorStr = QStringLiteral("网络请求失败");
        }

        *handled = true;
        onError(buildHttpErrorMessage(statusCode, errorStr));
        m_activeReplies.remove(reply);
        reply->deleteLater();
    });

    timer->start();
}

void HttpClient::postStreamImpl(const QString& endpoint,
                                const QJsonObject& body,
                                int attempt,
                                quint64 requestGeneration,
                                std::function<void(const QByteArray&)> onData,
                                std::function<void()> onComplete,
                                std::function<void(const QString&)> onError) {
    if (requestGeneration != m_requestGeneration) {
        return;
    }

    QString ep = endpoint;
    if (ep.startsWith('/') && !m_baseUrl.isEmpty()) {
        ep = ep.mid(1);
    }
    QString fullUrl = m_baseUrl + "/" + ep;
    QUrl url(fullUrl);
    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());
    request.setRawHeader("Accept", "text/event-stream");

    QJsonDocument doc(body);
    QByteArray data = doc.toJson();

    QNetworkReply* reply = m_networkManager->post(request, data);
    m_activeReplies.insert(reply);
    connect(reply, &QObject::destroyed, this, [this](QObject* obj) {
        m_activeReplies.remove(static_cast<QNetworkReply*>(obj));
    });

    QPointer<QTimer> timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(m_timeoutMs);

    QSharedPointer<bool> handled(new bool(false));
    QSharedPointer<bool> receivedAnyData(new bool(false));

    connect(timer.data(), &QTimer::timeout, [reply, timer, onError, handled]() {
        if (*handled) {
            return;
        }

        if (reply->property("clawpp_aborted").toBool()) {
            *handled = true;
            if (timer) {
                timer->deleteLater();
            }
            return;
        }

        *handled = true;
        reply->abort();
        if (timer) {
            timer->deleteLater();
        }
        onError(QStringLiteral("请求超时"));
    });

    connect(reply, &QNetworkReply::readyRead, [reply, onData, receivedAnyData, handled]() {
        if (reply->property("clawpp_aborted").toBool() || *handled) {
            return;
        }
        *receivedAnyData = true;
        QByteArray data = reply->readAll();
        onData(data);
    });

    connect(reply, &QNetworkReply::finished, [this, reply, timer, onData, onComplete, onError, attempt, body, endpoint, requestGeneration, handled, receivedAnyData]() {
        if (timer) {
            timer->stop();
            timer->deleteLater();
        }

        if (*handled) {
            m_activeReplies.remove(reply);
            reply->deleteLater();
            return;
        }

        if (reply->property("clawpp_aborted").toBool()) {
            *handled = true;
            m_activeReplies.remove(reply);
            reply->deleteLater();
            return;
        }

        const int statusCode = httpStatusCode(reply);
        const QByteArray responseData = reply->readAll();
        const QString defaultError = reply->errorString();
        QString errorStr = responseData.isEmpty() ? defaultError : parseErrorResponse(responseData, defaultError);

        if (reply->error() == QNetworkReply::NoError
            && statusCode >= 200 && statusCode < 300) {
            *handled = true;
            onComplete();
            m_activeReplies.remove(reply);
            reply->deleteLater();
            return;
        }

        if (isRetryableStatusCode(statusCode) && attempt < kMaxRetryAttempts && !*receivedAnyData) {
            const int delayMs = retryDelayMs(reply, attempt);
            LOG_WARN(QStringLiteral("HTTP %1 流式请求被限流或暂时失败，%2 毫秒后重试（第 %3/%4 次）。")
                         .arg(statusCode)
                         .arg(delayMs)
                         .arg(attempt + 1)
                         .arg(kMaxRetryAttempts));
            *handled = true;
            QTimer::singleShot(delayMs, this, [=]() {
                postStreamImpl(endpoint, body, attempt + 1, requestGeneration, onData, onComplete, onError);
            });
            m_activeReplies.remove(reply);
            reply->deleteLater();
            return;
        }

        if (errorStr.isEmpty()) {
            errorStr = QStringLiteral("网络请求失败");
        }

        *handled = true;
        onError(buildHttpErrorMessage(statusCode, errorStr));
        m_activeReplies.remove(reply);
        reply->deleteLater();
    });

    timer->start();
}

void HttpClient::abort() {
    ++m_requestGeneration;

    QList<QPointer<QNetworkReply>> replySnapshot;
    replySnapshot.reserve(m_activeReplies.size());
    for (QNetworkReply* reply : m_activeReplies) {
        replySnapshot.append(QPointer<QNetworkReply>(reply));
    }

    for (const QPointer<QNetworkReply>& replyPtr : replySnapshot) {
        if (!replyPtr) {
            continue;
        }
        QNetworkReply* reply = replyPtr.data();
        if (!reply || !reply->isRunning()) {
            continue;
        }

        reply->setProperty("clawpp_aborted", true);
        reply->abort();
    }
}

QString HttpClient::parseErrorResponse(const QByteArray& data, const QString& defaultError) {
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        return defaultError;
    }
    
    QJsonObject obj = doc.object();
    
    if (obj.contains("error")) {
        QJsonObject errorObj = obj["error"].toObject();
        if (errorObj.contains("message")) {
            return errorObj["message"].toString();
        }
    }
    
    return defaultError;
}

int HttpClient::httpStatusCode(QNetworkReply* reply) {
    return reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
}

bool HttpClient::isRetryableStatusCode(int statusCode) {
    return statusCode == 429 || statusCode >= 500;
}

int HttpClient::retryDelayMs(QNetworkReply* reply, int attempt) {
    int delayMs = kInitialRetryDelayMs;
    for (int i = 0; i < attempt; ++i) {
        if (delayMs >= kMaxRetryDelayMs / 2) {
            delayMs = kMaxRetryDelayMs;
            break;
        }
        delayMs *= 2;
    }

    const QByteArray retryAfterHeader = reply->rawHeader("Retry-After");
    if (!retryAfterHeader.isEmpty()) {
        bool ok = false;
        const int retryAfterSeconds = QString::fromUtf8(retryAfterHeader).trimmed().toInt(&ok);
        if (ok && retryAfterSeconds > 0) {
            delayMs = qMax(delayMs, qMin(retryAfterSeconds * 1000, kMaxRetryDelayMs));
        }
    }

    return qMin(delayMs, kMaxRetryDelayMs);
}

QString HttpClient::buildHttpErrorMessage(int statusCode, const QString& detail) {
    const QString trimmedDetail = detail.trimmed();

    if (statusCode == 429) {
        if (trimmedDetail.isEmpty()) {
            return QStringLiteral("请求过于频繁（HTTP 429），已触发服务端限流，请稍后重试。");
        }
        return QStringLiteral("请求过于频繁（HTTP 429），已触发服务端限流。%1").arg(trimmedDetail);
    }

    if (statusCode >= 500) {
        if (trimmedDetail.isEmpty()) {
            return QStringLiteral("服务端暂时不可用（HTTP %1），请稍后重试。").arg(statusCode);
        }
        return QStringLiteral("服务端暂时不可用（HTTP %1）。%2").arg(statusCode).arg(trimmedDetail);
    }

    if (statusCode >= 400) {
        if (trimmedDetail.isEmpty()) {
            return QStringLiteral("请求失败（HTTP %1）。").arg(statusCode);
        }
        return QStringLiteral("请求失败（HTTP %1）。%2").arg(statusCode).arg(trimmedDetail);
    }

    if (trimmedDetail.isEmpty()) {
        return QStringLiteral("网络请求失败");
    }

    return trimmedDetail;
}

}
