#include "openai_provider.h"
#include "tool/tool_registry.h"

#include <QtGlobal>

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
        const QString value = jsonValueToTaggedArgString(it.value());
        lines.append(QStringLiteral("<arg_key>%1</arg_key>").arg(it.key()));
        lines.append(QStringLiteral("<arg_value>%1</arg_value>").arg(value));
    }
    lines.append(QStringLiteral("</tool_call>"));
    return lines.join('\n');
}

QJsonObject parseFunctionArguments(const QJsonValue& argumentsValue) {
    if (argumentsValue.isObject()) {
        return argumentsValue.toObject();
    }

    const QString rawArgs = argumentsValue.toString().trimmed();
    if (rawArgs.isEmpty()) {
        return QJsonObject();
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(rawArgs.toUtf8(), &parseError);
    if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
        return doc.object();
    }

    return QJsonObject{{QStringLiteral("input"), rawArgs}};
}

QString extractTaggedToolCalls(const QJsonObject& message) {
    QStringList taggedBlocks;

    const QJsonArray toolCalls = message.value(QStringLiteral("tool_calls")).toArray();
    for (const QJsonValue& value : toolCalls) {
        const QJsonObject toolCall = value.toObject();
        const QJsonObject functionObj = toolCall.value(QStringLiteral("function")).toObject();
        const QString toolName = functionObj.value(QStringLiteral("name")).toString();
        const QJsonObject args = parseFunctionArguments(functionObj.value(QStringLiteral("arguments")));
        const QString tagged = buildTaggedToolCall(toolName, args);
        if (!tagged.isEmpty()) {
            taggedBlocks.append(tagged);
        }
    }

    if (taggedBlocks.isEmpty()) {
        const QJsonObject functionCall = message.value(QStringLiteral("function_call")).toObject();
        if (!functionCall.isEmpty()) {
            const QString toolName = functionCall.value(QStringLiteral("name")).toString();
            const QJsonObject args = parseFunctionArguments(functionCall.value(QStringLiteral("arguments")));
            const QString tagged = buildTaggedToolCall(toolName, args);
            if (!tagged.isEmpty()) {
                taggedBlocks.append(tagged);
            }
        }
    }

    return taggedBlocks.join(QStringLiteral("\n"));
}

}

namespace clawpp {

OpenAIProvider::OpenAIProvider(const ProviderConfig& config, QObject* parent)
    : ILLMProvider(parent)
    , m_httpClient(new HttpClient(this))
    , m_sseParser(new SseParser(this))
    , m_config(config)
    , m_lastError() {
    
    m_httpClient->setBaseUrl(config.baseUrl);
    m_httpClient->setApiKey(config.apiKey);
    m_httpClient->setTimeout(config.timeoutMs);
}

QString OpenAIProvider::name() const {
    return "openai";
}

bool OpenAIProvider::isAvailable() const {
    return !m_config.apiKey.isEmpty() && !m_config.baseUrl.isEmpty();
}

QString OpenAIProvider::lastError() const {
    return m_lastError;
}

bool OpenAIProvider::supportsStreaming() const {
    return true;
}

void OpenAIProvider::chat(const MessageList& messages,
                          const ChatOptions& options,
                          std::function<void(const QString&)> onSuccess,
                          std::function<void(const QString&)> onError) {
    // 非流式请求：一次性返回最终文本。
    QJsonObject body = buildRequestBody(messages, options, false);
    
    m_httpClient->post("/chat/completions", body,
        [this, onSuccess, onError](const QByteArray& response) {
            QString content = parseResponse(response);
            if (content.isEmpty() && !response.isEmpty()) {
                QString err = parseErrorResponse(response);
                if (!err.isEmpty()) {
                    m_lastError = err;
                    LOG_ERROR(QString("API Error: %1").arg(err));
                    onError(err);
                    return;
                }
            }
            m_lastError.clear();
            onSuccess(content);
        },
        [this, onError](const QString& error) {
            m_lastError = error;
            LOG_ERROR(QString("OpenAI chat error: %1").arg(error));
            onError(error);
        }
    );
}

void OpenAIProvider::chatStream(const MessageList& messages,
                                const ChatOptions& options,
                                StreamCallback onChunk,
                                std::function<void()> onComplete,
                                ErrorCallback onError) {
    // 每次新流式请求前重置解析器缓冲，避免跨请求污染。
    m_sseParser->reset();
    
    QJsonObject body = buildRequestBody(messages, options, true);
    
    m_httpClient->postStream("/chat/completions", body,
        [this, onChunk](const QByteArray& data) {
            QList<StreamChunk> chunks = m_sseParser->parse(data);
            for (const StreamChunk& chunk : chunks) {
                onChunk(chunk);
                if (!chunk.content.isEmpty()) {
                    emit streamToken(chunk.content);
                }
            }
        },
        [onComplete]() {
            onComplete();
        },
        [this, onError](const QString& error) {
            m_lastError = error;
            LOG_ERROR(QString("OpenAI stream error: %1").arg(error));
            onError(error);
        }
    );
}

void OpenAIProvider::abort() {
    m_httpClient->abort();
}

void OpenAIProvider::setApiKey(const QString& apiKey) {
    m_config.apiKey = apiKey;
    m_httpClient->setApiKey(apiKey);
}

void OpenAIProvider::setBaseUrl(const QString& baseUrl) {
    m_config.baseUrl = baseUrl;
    m_httpClient->setBaseUrl(baseUrl);
}

QJsonObject OpenAIProvider::buildRequestBody(const MessageList& messages,
                                              const ChatOptions& options,
                                              bool stream) {
    QJsonObject body;

    body["model"] = options.model.isEmpty() ? m_config.model : options.model;
    body["stream"] = stream;
    
    QJsonArray messagesArray;
    for (const Message& msg : messages) {
        messagesArray.append(msg.toJson());
    }
    body["messages"] = messagesArray;
    
    // temperature=0 是合法值，表示更确定性的输出。
    body["temperature"] = qBound(0.0, options.temperature, 2.0);
    
    if (options.maxTokens > 0) {
        body["max_tokens"] = options.maxTokens;
    } else {
        body["max_tokens"] = m_config.maxTokens;
    }
    
    if (!options.systemPrompt.isEmpty()) {
        QJsonObject systemMsg;
        systemMsg["role"] = "system";
        systemMsg["content"] = options.systemPrompt;
        messagesArray.insert(0, systemMsg);
        body["messages"] = messagesArray;
    }
    
    if (!options.stopSequences.isEmpty()) {
        QJsonArray stops;
        for (const QString& s : options.stopSequences) {
            stops.append(s);
        }
        body["stop"] = stops;
    }
    
    if (options.responseFormat.type != ResponseFormat::Text) {
        body["response_format"] = options.responseFormat.toJson();
    }
    
    if (!options.tools.isEmpty()) {
        body["tools"] = options.tools;
    }
    
    return body;
}

QString OpenAIProvider::parseResponse(const QByteArray& response) {
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(response, &error);
    
    if (error.error != QJsonParseError::NoError) {
        LOG_ERROR(QString("JSON parse error: %1").arg(error.errorString()));
        return QString();
    }
    
    QJsonObject obj = doc.object();
    QJsonArray choices = obj["choices"].toArray();
    
    if (choices.isEmpty()) {
        return QString();
    }
    
    QJsonObject choice = choices[0].toObject();
    QJsonObject message = choice["message"].toObject();

    const QString content = message.value(QStringLiteral("content")).toString();
    const QString taggedToolCalls = extractTaggedToolCalls(message);
    if (taggedToolCalls.isEmpty()) {
        return content;
    }

    if (content.trimmed().isEmpty()) {
        return taggedToolCalls;
    }

    return content + QStringLiteral("\n") + taggedToolCalls;
}

QString OpenAIProvider::parseErrorResponse(const QByteArray& response) {
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(response, &error);
    
    if (error.error != QJsonParseError::NoError) {
        return QString();
    }
    
    QJsonObject obj = doc.object();
    QJsonObject errorObj = obj["error"].toObject();
    
    return errorObj["message"].toString();
}

}
