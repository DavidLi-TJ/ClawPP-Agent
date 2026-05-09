#include "anthropic_provider.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkRequest>
#include <QSharedPointer>
#include <QStringList>

namespace clawpp {

namespace {

QString jsonValueToTaggedArgString(const QJsonValue& value) {
    if (value.isString()) {
        return value.toString();
    }
    if (value.isBool()) {
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    }
    if (value.isDouble()) {
        return QString::number(value.toDouble(), 'g', 15);
    }
    if (value.isArray()) {
        return QString::fromUtf8(QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact));
    }
    if (value.isObject()) {
        return QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));
    }
    return QString();
}

QString buildTaggedToolCall(const QString& toolName, const QJsonObject& args) {
    if (toolName.trimmed().isEmpty()) {
        return QString();
    }

    QStringList lines;
    lines.append(QStringLiteral("<tool_call>%1").arg(toolName.trimmed()));
    for (auto it = args.begin(); it != args.end(); ++it) {
        lines.append(QStringLiteral("<arg_key>%1</arg_key>").arg(it.key()));
        lines.append(QStringLiteral("<arg_value>%1</arg_value>").arg(jsonValueToTaggedArgString(it.value())));
    }
    lines.append(QStringLiteral("</tool_call>"));
    return lines.join('\n');
}

}

AnthropicProvider::AnthropicProvider(const ProviderConfig& config, QObject* parent)
    : ILLMProvider(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_config(config)
    , m_lastError()
    , m_activeReply(nullptr)
    , m_timeoutTimer(nullptr)
    , m_streamBuffer()
    , m_inputTokens(0)
    , m_outputTokens(0)
    , m_requestGeneration(0) {
}

QString AnthropicProvider::name() const {
    return QStringLiteral("anthropic");
}

bool AnthropicProvider::isAvailable() const {
    return !m_config.apiKey.trimmed().isEmpty() && !m_config.baseUrl.trimmed().isEmpty();
}

QString AnthropicProvider::lastError() const {
    return m_lastError;
}

void AnthropicProvider::setApiKey(const QString& apiKey) {
    m_config.apiKey = apiKey;
}

void AnthropicProvider::setBaseUrl(const QString& baseUrl) {
    m_config.baseUrl = baseUrl;
}

void AnthropicProvider::abort() {
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

void AnthropicProvider::chat(const MessageList& messages,
                             const ChatOptions& options,
                             std::function<void(const QString&)> onSuccess,
                             std::function<void(const QString&)> onError) {
    if (!isAvailable()) {
        m_lastError = QStringLiteral("Anthropic 配置不完整，请检查 API Key 与 Base URL");
        onError(m_lastError);
        return;
    }

    abort();
    const quint64 requestGeneration = ++m_requestGeneration;

    QNetworkRequest request(QUrl(buildUrl(QStringLiteral("v1/messages"))));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("x-api-key", m_config.apiKey.toUtf8());
    request.setRawHeader("anthropic-version", "2023-06-01");
    request.setRawHeader("Accept", "application/json");

    const QJsonObject body = buildRequestBody(messages, options, false);
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

        m_lastError = QStringLiteral("Anthropic 请求超时");
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
            const QString text = parseResponseText(response);
            m_lastError.clear();
            onSuccess(text);
        } else {
            *handled = true;
            const QString detail = parseErrorResponse(response, reply->errorString());
            m_lastError = QStringLiteral("Anthropic 请求失败(%1): %2").arg(statusCode).arg(detail);
            onError(m_lastError);
        }

        reply->deleteLater();
        clearActiveRequest();
    });

    timer->start();
}

void AnthropicProvider::chatStream(const MessageList& messages,
                                   const ChatOptions& options,
                                   StreamCallback onChunk,
                                   std::function<void()> onComplete,
                                   ErrorCallback onError) {
    if (!isAvailable()) {
        m_lastError = QStringLiteral("Anthropic 配置不完整，请检查 API Key 与 Base URL");
        onError(m_lastError);
        return;
    }

    abort();
    const quint64 requestGeneration = ++m_requestGeneration;

    m_streamBuffer.clear();
    m_inputTokens = 0;
    m_outputTokens = 0;

    QNetworkRequest request(QUrl(buildUrl(QStringLiteral("v1/messages"))));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("x-api-key", m_config.apiKey.toUtf8());
    request.setRawHeader("anthropic-version", "2023-06-01");
    request.setRawHeader("Accept", "text/event-stream");

    const QJsonObject body = buildRequestBody(messages, options, true);
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

        m_lastError = QStringLiteral("Anthropic 流式请求超时");
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

        // Consume any trailing bytes before completion.
        const QByteArray tail = reply->readAll();
        if (!tail.isEmpty()) {
            consumeSseEvents(tail, onChunk);
        }

        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() == QNetworkReply::NoError && statusCode >= 200 && statusCode < 300) {
            *handled = true;
            m_lastError.clear();
            onComplete();
        } else {
            *handled = true;
            const QString detail = parseErrorResponse(tail, reply->errorString());
            m_lastError = QStringLiteral("Anthropic 流式请求失败(%1): %2").arg(statusCode).arg(detail);
            onError(m_lastError);
        }

        reply->deleteLater();
        clearActiveRequest();
    });

    timer->start();
}

QString AnthropicProvider::buildUrl(const QString& endpoint) const {
    QString base = m_config.baseUrl.trimmed();
    if (base.endsWith('/')) {
        base.chop(1);
    }

    QString ep = endpoint.trimmed();
    if (ep.startsWith('/')) {
        ep.remove(0, 1);
    }

    return QStringLiteral("%1/%2").arg(base, ep);
}

QJsonObject AnthropicProvider::buildRequestBody(const MessageList& messages,
                                                const ChatOptions& options,
                                                bool stream) const {
    QJsonObject body;
    body.insert(QStringLiteral("model"), options.model.isEmpty() ? m_config.model : options.model);
    body.insert(QStringLiteral("max_tokens"), options.maxTokens > 0 ? options.maxTokens : m_config.maxTokens);
    body.insert(QStringLiteral("temperature"), qBound(0.0, options.temperature, 1.0));
    body.insert(QStringLiteral("stream"), stream);

    QString systemPrompt = options.systemPrompt;
    for (const Message& message : messages) {
        if (message.role == MessageRole::System && !message.content.trimmed().isEmpty()) {
            if (!systemPrompt.trimmed().isEmpty()) {
                systemPrompt += QStringLiteral("\n\n");
            }
            systemPrompt += message.content;
        }
    }
    if (!systemPrompt.trimmed().isEmpty()) {
        body.insert(QStringLiteral("system"), systemPrompt.trimmed());
    }

    body.insert(QStringLiteral("messages"), buildMessages(messages));
    
    // Native function calling support
    if (!options.tools.isEmpty()) {
        body.insert(QStringLiteral("tools"), options.tools);
    }
    
    return body;
}

QJsonArray AnthropicProvider::buildMessages(const MessageList& messages) const {
    QJsonArray result;

    for (const Message& message : messages) {
        if (message.role == MessageRole::System) {
            continue;
        }

        QJsonObject row;
        
        // Handle different message roles
        if (message.role == MessageRole::Assistant) {
            row.insert(QStringLiteral("role"), QStringLiteral("assistant"));
            
            // If assistant message has tool calls, use content array format
            if (!message.toolCalls.isEmpty()) {
                QJsonArray contentArray;
                
                // Add text content if present
                if (!message.content.trimmed().isEmpty()) {
                    contentArray.append(QJsonObject{
                        {QStringLiteral("type"), QStringLiteral("text")},
                        {QStringLiteral("text"), message.content}
                    });
                }
                
                // Add tool_use blocks
                for (const ToolCallData& tc : message.toolCalls) {
                    QJsonObject toolUse;
                    toolUse.insert(QStringLiteral("type"), QStringLiteral("tool_use"));
                    toolUse.insert(QStringLiteral("id"), tc.id);
                    toolUse.insert(QStringLiteral("name"), tc.name);
                    
                    // Parse arguments JSON string to object
                    QJsonParseError parseError;
                    QJsonDocument argsDoc = QJsonDocument::fromJson(tc.arguments.toUtf8(), &parseError);
                    if (parseError.error == QJsonParseError::NoError && argsDoc.isObject()) {
                        toolUse.insert(QStringLiteral("input"), argsDoc.object());
                    } else {
                        toolUse.insert(QStringLiteral("input"), QJsonObject{});
                    }
                    
                    contentArray.append(toolUse);
                }
                
                row.insert(QStringLiteral("content"), contentArray);
            } else if (!message.content.trimmed().isEmpty()) {
                row.insert(QStringLiteral("content"), message.content);
            } else {
                continue; // Skip empty assistant messages
            }
        }
        else if (message.role == MessageRole::Tool) {
            // Tool results are sent as user messages with tool_result content blocks
            row.insert(QStringLiteral("role"), QStringLiteral("user"));
            
            QJsonArray contentArray;
            QJsonObject toolResult;
            toolResult.insert(QStringLiteral("type"), QStringLiteral("tool_result"));
            toolResult.insert(QStringLiteral("tool_use_id"), message.toolCallId);
            toolResult.insert(QStringLiteral("content"), message.content);
            contentArray.append(toolResult);
            
            row.insert(QStringLiteral("content"), contentArray);
        }
        else { // User messages
            row.insert(QStringLiteral("role"), QStringLiteral("user"));
            
            if (!message.content.trimmed().isEmpty()) {
                row.insert(QStringLiteral("content"), message.content);
            } else {
                continue; // Skip empty user messages
            }
        }
        
        result.append(row);
    }

    return result;
}

QString AnthropicProvider::parseResponseText(const QByteArray& response) const {
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(response, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return QString();
    }

    const QJsonObject obj = doc.object();
    const QJsonArray contentArray = obj.value(QStringLiteral("content")).toArray();

    QStringList texts;
    QStringList taggedToolCalls;
    for (const QJsonValue& value : contentArray) {
        const QJsonObject block = value.toObject();
        const QString blockType = block.value(QStringLiteral("type")).toString();
        
        if (blockType == QStringLiteral("text")) {
            const QString text = block.value(QStringLiteral("text")).toString();
            if (!text.isEmpty()) {
                texts.append(text);
            }
        }
        else if (blockType == QStringLiteral("tool_use")) {
            const QString toolName = block.value(QStringLiteral("name")).toString();
            const QJsonObject input = block.value(QStringLiteral("input")).toObject();
            const QString tagged = buildTaggedToolCall(toolName, input);
            if (!tagged.isEmpty()) {
                taggedToolCalls.append(tagged);
            }
        }
    }

    if (taggedToolCalls.isEmpty()) {
        return texts.join(QString());
    }

    if (texts.isEmpty()) {
        return taggedToolCalls.join(QStringLiteral("\n"));
    }

    return texts.join(QString()) + QStringLiteral("\n") + taggedToolCalls.join(QStringLiteral("\n"));
}

QString AnthropicProvider::parseErrorResponse(const QByteArray& response, const QString& fallback) const {
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

void AnthropicProvider::consumeSseEvents(const QByteArray& data, const StreamCallback& onChunk) {
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

        QString eventType;
        QStringList dataLines;
        const QStringList lines = block.split('\n', Qt::SkipEmptyParts);
        for (const QString& rawLine : lines) {
            const QString line = rawLine.trimmed();
            if (line.startsWith(QStringLiteral("event:"))) {
                eventType = line.mid(6).trimmed();
            } else if (line.startsWith(QStringLiteral("data:"))) {
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
        if (eventType.isEmpty()) {
            eventType = obj.value(QStringLiteral("type")).toString();
        }

        StreamChunk chunk;
        bool shouldEmit = false;

        if (eventType == QStringLiteral("content_block_start")) {
            // New content block starting - could be text or tool_use
            const QJsonObject contentBlock = obj.value(QStringLiteral("content_block")).toObject();
            const QString blockType = contentBlock.value(QStringLiteral("type")).toString();
            
            if (blockType == QStringLiteral("tool_use")) {
                // Tool use block starting
                const int index = obj.value(QStringLiteral("index")).toInt(-1);
                const QString toolId = contentBlock.value(QStringLiteral("id")).toString();
                const QString toolName = contentBlock.value(QStringLiteral("name")).toString();
                
                if (index >= 0 && !toolId.isEmpty() && !toolName.isEmpty()) {
                    ToolCallDelta tc;
                    tc.index = index;
                    tc.id = toolId;
                    tc.type = QStringLiteral("function");
                    tc.name = toolName;
                    tc.arguments = QString(); // Will be filled in delta events
                    chunk.toolCalls.append(tc);
                    shouldEmit = true;
                }
            }
        }

        if (eventType == QStringLiteral("content_block_delta")) {
            const QJsonObject delta = obj.value(QStringLiteral("delta")).toObject();
            const QString deltaType = delta.value(QStringLiteral("type")).toString();
            
            if (deltaType == QStringLiteral("text_delta")) {
                chunk.content = delta.value(QStringLiteral("text")).toString();
                shouldEmit = !chunk.content.isEmpty();
            }
            else if (deltaType == QStringLiteral("input_json_delta")) {
                // Tool input arguments streaming
                const int index = obj.value(QStringLiteral("index")).toInt(-1);
                const QString partialJson = delta.value(QStringLiteral("partial_json")).toString();
                
                if (index >= 0 && !partialJson.isEmpty()) {
                    ToolCallDelta tc;
                    tc.index = index;
                    tc.arguments = partialJson;
                    chunk.toolCalls.append(tc);
                    shouldEmit = true;
                }
            }
        }

        if (eventType == QStringLiteral("content_block_stop")) {
            // Content block finished - could signal tool use completion
            const QString stopReason = obj.value(QStringLiteral("stop_reason")).toString();
            if (stopReason == QStringLiteral("tool_use")) {
                chunk.finishReason = QStringLiteral("tool_calls");
                shouldEmit = true;
            }
        }

        if (eventType == QStringLiteral("message_start")) {
            const QJsonObject usage = obj.value(QStringLiteral("message")).toObject().value(QStringLiteral("usage")).toObject();
            if (usage.contains(QStringLiteral("input_tokens"))) {
                m_inputTokens = usage.value(QStringLiteral("input_tokens")).toInt(m_inputTokens);
            }

            if (m_inputTokens > 0 || m_outputTokens > 0) {
                chunk.hasUsage = true;
                chunk.promptTokens = m_inputTokens;
                chunk.completionTokens = m_outputTokens;
                chunk.totalTokens = m_inputTokens + m_outputTokens;
                shouldEmit = true;
            }
        }

        if (eventType == QStringLiteral("message_delta")) {
            const QJsonObject usage = obj.value(QStringLiteral("usage")).toObject();
            if (usage.contains(QStringLiteral("input_tokens"))) {
                m_inputTokens = usage.value(QStringLiteral("input_tokens")).toInt(m_inputTokens);
            }
            if (usage.contains(QStringLiteral("output_tokens"))) {
                m_outputTokens = usage.value(QStringLiteral("output_tokens")).toInt(m_outputTokens);
            }

            if (m_inputTokens > 0 || m_outputTokens > 0) {
                chunk.hasUsage = true;
                chunk.promptTokens = m_inputTokens;
                chunk.completionTokens = m_outputTokens;
                chunk.totalTokens = m_inputTokens + m_outputTokens;
                shouldEmit = true;
            }

            const QString stopReason = obj.value(QStringLiteral("delta")).toObject().value(QStringLiteral("stop_reason")).toString();
            if (!stopReason.isEmpty()) {
                chunk.finishReason = stopReason;
                shouldEmit = true;
            }
        }

        if (eventType == QStringLiteral("message_stop")) {
            if (chunk.finishReason.isEmpty()) {
                chunk.finishReason = QStringLiteral("stop");
            }
            shouldEmit = true;
        }

        if (shouldEmit) {
            onChunk(chunk);
            if (!chunk.content.isEmpty()) {
                emit streamToken(chunk.content);
            }
        }
    }
}

void AnthropicProvider::clearActiveRequest() {
    m_activeReply = nullptr;
}

} // namespace clawpp
