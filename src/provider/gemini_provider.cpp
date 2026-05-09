#include "gemini_provider.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkRequest>
#include <QSharedPointer>
#include <QStringList>

namespace clawpp {

GeminiProvider::GeminiProvider(const ProviderConfig& config, QObject* parent)
    : ILLMProvider(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_config(config)
    , m_lastError()
    , m_activeReply(nullptr)
    , m_timeoutTimer(nullptr)
    , m_streamBuffer()
    , m_streamAccumulatedText()
    , m_promptTokens(0)
    , m_completionTokens(0)
    , m_totalTokens(0)
    , m_requestGeneration(0) {
}

QString GeminiProvider::name() const {
    return QStringLiteral("gemini");
}

bool GeminiProvider::isAvailable() const {
    return !m_config.apiKey.trimmed().isEmpty() && !m_config.baseUrl.trimmed().isEmpty();
}

QString GeminiProvider::lastError() const {
    return m_lastError;
}

void GeminiProvider::setApiKey(const QString& apiKey) {
    m_config.apiKey = apiKey;
}

void GeminiProvider::setBaseUrl(const QString& baseUrl) {
    m_config.baseUrl = baseUrl;
}

void GeminiProvider::abort() {
    ++m_requestGeneration;

    if (m_timeoutTimer) {
        m_timeoutTimer->stop();
        m_timeoutTimer->deleteLater();
        m_timeoutTimer = nullptr;
    }

    if (m_activeReply && m_activeReply->isRunning()) {
        m_activeReply->setProperty("clawpp_aborted", true);
        m_activeReply->abort();
    }

    clearActiveRequest();
}

void GeminiProvider::chat(const MessageList& messages,
                          const ChatOptions& options,
                          std::function<void(const QString&)> onSuccess,
                          std::function<void(const QString&)> onError) {
    if (!isAvailable()) {
        m_lastError = QStringLiteral("Gemini 配置不完整，请检查 API Key 与 Base URL");
        onError(m_lastError);
        return;
    }

    abort();
    const quint64 requestGeneration = ++m_requestGeneration;

    QString modelName = options.model.isEmpty() ? m_config.model : options.model;
    if (modelName.startsWith(QStringLiteral("models/"))) {
        modelName = modelName.mid(QStringLiteral("models/").size());
    }

    QNetworkRequest request(QUrl(buildUrl(QStringLiteral("models/%1:generateContent").arg(modelName))));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("x-goog-api-key", m_config.apiKey.toUtf8());
    request.setRawHeader("Accept", "application/json");

    const QJsonObject body = buildRequestBody(messages, options);
    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    m_activeReply = reply;

    QPointer<QTimer> timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(qMax(1000, m_config.timeoutMs));
    m_timeoutTimer = timer;

    QSharedPointer<bool> handled(new bool(false));

    connect(timer.data(), &QTimer::timeout, this, [this, reply, onError, handled]() {
        if (*handled) {
            return;
        }

        *handled = true;
        if (reply) {
            reply->setProperty("clawpp_aborted", true);
            reply->abort();
        }

        m_lastError = QStringLiteral("Gemini 请求超时");
        onError(m_lastError);
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, timer, handled, onSuccess, onError, requestGeneration]() {
        if (timer) {
            timer->stop();
            timer->deleteLater();
        }
        if (m_timeoutTimer == timer) {
            m_timeoutTimer = nullptr;
        }

        if (*handled || requestGeneration != m_requestGeneration || reply->property("clawpp_aborted").toBool()) {
            reply->deleteLater();
            clearActiveRequest();
            return;
        }

        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray response = reply->readAll();

        if (reply->error() == QNetworkReply::NoError && statusCode >= 200 && statusCode < 300) {
            *handled = true;
            QJsonParseError parseError;
            const QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
            if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
                m_lastError = QStringLiteral("Gemini 响应解析失败");
                onError(m_lastError);
            } else {
                m_lastError.clear();
                onSuccess(parseResponseText(doc.object()));
            }
        } else {
            *handled = true;
            const QString detail = parseErrorResponse(response, reply->errorString());
            m_lastError = QStringLiteral("Gemini 请求失败(%1): %2").arg(statusCode).arg(detail);
            onError(m_lastError);
        }

        reply->deleteLater();
        clearActiveRequest();
    });

    timer->start();
}

void GeminiProvider::chatStream(const MessageList& messages,
                                const ChatOptions& options,
                                StreamCallback onChunk,
                                std::function<void()> onComplete,
                                ErrorCallback onError) {
    if (!isAvailable()) {
        m_lastError = QStringLiteral("Gemini 配置不完整，请检查 API Key 与 Base URL");
        onError(m_lastError);
        return;
    }

    abort();
    const quint64 requestGeneration = ++m_requestGeneration;

    m_streamBuffer.clear();
    m_streamAccumulatedText.clear();
    m_promptTokens = 0;
    m_completionTokens = 0;
    m_totalTokens = 0;

    QString modelName = options.model.isEmpty() ? m_config.model : options.model;
    if (modelName.startsWith(QStringLiteral("models/"))) {
        modelName = modelName.mid(QStringLiteral("models/").size());
    }

    QNetworkRequest request(QUrl(buildUrl(QStringLiteral("models/%1:streamGenerateContent?alt=sse").arg(modelName))));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("x-goog-api-key", m_config.apiKey.toUtf8());
    request.setRawHeader("Accept", "text/event-stream");

    const QJsonObject body = buildRequestBody(messages, options);
    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    m_activeReply = reply;

    QPointer<QTimer> timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(qMax(1000, m_config.timeoutMs));
    m_timeoutTimer = timer;

    QSharedPointer<bool> handled(new bool(false));

    connect(timer.data(), &QTimer::timeout, this, [this, reply, onError, handled]() {
        if (*handled) {
            return;
        }

        *handled = true;
        if (reply) {
            reply->setProperty("clawpp_aborted", true);
            reply->abort();
        }

        m_lastError = QStringLiteral("Gemini 流式请求超时");
        onError(m_lastError);
    });

    connect(reply, &QNetworkReply::readyRead, this, [this, reply, onChunk, handled, requestGeneration]() {
        if (*handled || requestGeneration != m_requestGeneration || reply->property("clawpp_aborted").toBool()) {
            return;
        }

        consumeSseEvents(reply->readAll(), onChunk);
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, timer, handled, onChunk, onComplete, onError, requestGeneration]() {
        if (timer) {
            timer->stop();
            timer->deleteLater();
        }
        if (m_timeoutTimer == timer) {
            m_timeoutTimer = nullptr;
        }

        if (*handled || requestGeneration != m_requestGeneration || reply->property("clawpp_aborted").toBool()) {
            reply->deleteLater();
            clearActiveRequest();
            return;
        }

        const QByteArray tail = reply->readAll();
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() == QNetworkReply::NoError && statusCode >= 200 && statusCode < 300) {
            if (!tail.isEmpty()) {
                consumeSseEvents(tail, onChunk);
            }
            *handled = true;
            m_lastError.clear();
            onComplete();
        } else {
            *handled = true;
            const QString detail = parseErrorResponse(tail, reply->errorString());
            m_lastError = QStringLiteral("Gemini 流式请求失败(%1): %2").arg(statusCode).arg(detail);
            onError(m_lastError);
        }

        reply->deleteLater();
        clearActiveRequest();
    });

    timer->start();
}

QString GeminiProvider::buildUrl(const QString& endpointPath) const {
    QString base = m_config.baseUrl.trimmed();
    if (base.endsWith('/')) {
        base.chop(1);
    }

    QString ep = endpointPath.trimmed();
    if (ep.startsWith('/')) {
        ep.remove(0, 1);
    }

    return QStringLiteral("%1/%2").arg(base, ep);
}

QJsonObject GeminiProvider::buildRequestBody(const MessageList& messages,
                                             const ChatOptions& options) const {
    QString systemPrompt = options.systemPrompt;
    QJsonArray contents = buildContents(messages, &systemPrompt);

    QJsonObject body;
    body.insert(QStringLiteral("contents"), contents);

    if (!systemPrompt.trimmed().isEmpty()) {
        QJsonObject part;
        part.insert(QStringLiteral("text"), systemPrompt.trimmed());
        QJsonObject instruction;
        instruction.insert(QStringLiteral("parts"), QJsonArray{part});
        body.insert(QStringLiteral("systemInstruction"), instruction);
    }

    QJsonObject generationConfig;
    generationConfig.insert(QStringLiteral("temperature"), qBound(0.0, options.temperature, 2.0));
    generationConfig.insert(QStringLiteral("maxOutputTokens"), options.maxTokens > 0 ? options.maxTokens : m_config.maxTokens);
    if (!options.stopSequences.isEmpty()) {
        QJsonArray stop;
        for (const QString& value : options.stopSequences) {
            stop.append(value);
        }
        generationConfig.insert(QStringLiteral("stopSequences"), stop);
    }
    body.insert(QStringLiteral("generationConfig"), generationConfig);

    return body;
}

QJsonArray GeminiProvider::buildContents(const MessageList& messages, QString* systemPrompt) const {
    QJsonArray result;

    for (const Message& message : messages) {
        const QString content = message.content.trimmed();
        if (content.isEmpty()) {
            continue;
        }

        if (message.role == MessageRole::System) {
            if (systemPrompt) {
                if (!systemPrompt->trimmed().isEmpty()) {
                    *systemPrompt += QStringLiteral("\n\n");
                }
                *systemPrompt += content;
            }
            continue;
        }

        QJsonObject part;
        part.insert(QStringLiteral("text"), content);

        QJsonObject item;
        item.insert(QStringLiteral("parts"), QJsonArray{part});
        item.insert(QStringLiteral("role"), message.role == MessageRole::Assistant ? QStringLiteral("model") : QStringLiteral("user"));
        result.append(item);
    }

    if (result.isEmpty()) {
        QJsonObject part;
        part.insert(QStringLiteral("text"), QStringLiteral("Hello"));
        QJsonObject item;
        item.insert(QStringLiteral("role"), QStringLiteral("user"));
        item.insert(QStringLiteral("parts"), QJsonArray{part});
        result.append(item);
    }

    return result;
}

QString GeminiProvider::parseResponseText(const QJsonObject& root) const {
    const QJsonArray candidates = root.value(QStringLiteral("candidates")).toArray();
    QStringList texts;

    for (const QJsonValue& value : candidates) {
        const QJsonObject candidate = value.toObject();
        const QJsonObject content = candidate.value(QStringLiteral("content")).toObject();
        const QJsonArray parts = content.value(QStringLiteral("parts")).toArray();
        for (const QJsonValue& partValue : parts) {
            const QString text = partValue.toObject().value(QStringLiteral("text")).toString();
            if (!text.isEmpty()) {
                texts.append(text);
            }
        }
    }

    return texts.join(QString());
}

QString GeminiProvider::parseErrorResponse(const QByteArray& response, const QString& fallback) const {
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(response, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return fallback.isEmpty() ? QStringLiteral("未知错误") : fallback;
    }

    const QJsonObject obj = doc.object();
    const QJsonValue errorValue = obj.value(QStringLiteral("error"));
    if (errorValue.isObject()) {
        const QString message = errorValue.toObject().value(QStringLiteral("message")).toString();
        if (!message.isEmpty()) {
            return message;
        }
    } else if (errorValue.isString()) {
        const QString message = errorValue.toString();
        if (!message.isEmpty()) {
            return message;
        }
    }

    return fallback.isEmpty() ? QStringLiteral("未知错误") : fallback;
}

void GeminiProvider::applyUsageMetadata(const QJsonObject& root, StreamChunk* chunk) {
    if (!chunk) {
        return;
    }

    const QJsonObject usage = root.value(QStringLiteral("usageMetadata")).toObject();
    if (usage.isEmpty()) {
        return;
    }

    if (usage.contains(QStringLiteral("promptTokenCount"))) {
        m_promptTokens = usage.value(QStringLiteral("promptTokenCount")).toInt(m_promptTokens);
    }
    if (usage.contains(QStringLiteral("candidatesTokenCount"))) {
        m_completionTokens = usage.value(QStringLiteral("candidatesTokenCount")).toInt(m_completionTokens);
    }
    if (usage.contains(QStringLiteral("totalTokenCount"))) {
        m_totalTokens = usage.value(QStringLiteral("totalTokenCount")).toInt(m_totalTokens);
    } else {
        m_totalTokens = m_promptTokens + m_completionTokens;
    }

    chunk->hasUsage = true;
    chunk->promptTokens = m_promptTokens;
    chunk->completionTokens = m_completionTokens;
    chunk->totalTokens = m_totalTokens;
}

void GeminiProvider::consumeSseEvents(const QByteArray& data, const StreamCallback& onChunk) {
    QByteArray normalized = data;
    normalized.replace("\r\n", "\n");
    m_streamBuffer.append(normalized);

    while (true) {
        const int separator = m_streamBuffer.indexOf("\n\n");
        if (separator < 0) {
            break;
        }

        const QByteArray rawBlock = m_streamBuffer.left(separator);
        m_streamBuffer.remove(0, separator + 2);

        const QString block = QString::fromUtf8(rawBlock).trimmed();
        if (block.isEmpty()) {
            continue;
        }

        QStringList dataLines;
        const QStringList lines = block.split('\n', Qt::SkipEmptyParts);
        for (const QString& rawLine : lines) {
            const QString line = rawLine.trimmed();
            if (line.startsWith(QStringLiteral("data:"))) {
                dataLines.append(line.mid(5).trimmed());
            }
        }

        const QString payload = dataLines.join(QStringLiteral("\n")).trimmed();
        if (payload.isEmpty() || payload == QStringLiteral("[DONE]")) {
            continue;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(payload.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            continue;
        }

        const QJsonObject obj = doc.object();
        StreamChunk chunk;
        bool shouldEmit = false;

        const QString text = parseResponseText(obj);
        if (!text.isEmpty()) {
            QString delta = text;
            if (!m_streamAccumulatedText.isEmpty() && text.startsWith(m_streamAccumulatedText)) {
                delta = text.mid(m_streamAccumulatedText.length());
                m_streamAccumulatedText = text;
            } else {
                if (m_streamAccumulatedText.isEmpty()) {
                    m_streamAccumulatedText = text;
                } else {
                    m_streamAccumulatedText += text;
                }
            }

            if (!delta.isEmpty()) {
                chunk.content = delta;
                shouldEmit = true;
            }
        }

        applyUsageMetadata(obj, &chunk);
        if (chunk.hasUsage) {
            shouldEmit = true;
        }

        const QJsonArray candidates = obj.value(QStringLiteral("candidates")).toArray();
        if (!candidates.isEmpty()) {
            const QString finishReason = candidates.first().toObject().value(QStringLiteral("finishReason")).toString();
            if (!finishReason.isEmpty()) {
                chunk.finishReason = finishReason;
                shouldEmit = true;
            }
        }

        if (shouldEmit) {
            onChunk(chunk);
            if (!chunk.content.isEmpty()) {
                emit streamToken(chunk.content);
            }
        }
    }
}

void GeminiProvider::clearActiveRequest() {
    m_activeReply = nullptr;
}

} // namespace clawpp
