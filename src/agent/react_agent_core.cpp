#include "react_agent_core.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPointer>
#include <QSet>
#include <QThread>
#include <QtConcurrent/QtConcurrent>
#include <memory>

#include "common/constants.h"
#include "common/model_context_limits.h"
#include "tool/tool_registry.h"
#include "tool/i_tool.h"
#include "infrastructure/logging/logger.h"

namespace clawpp {

namespace {

QString jsonValueToString(const QJsonValue& value) {
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

QString normalizeTaggedValue(QString value) {
    value = value.trimmed();
    if (value.startsWith(QStringLiteral("```"))) {
        const int firstNewline = value.indexOf('\n');
        if (firstNewline >= 0) {
            value = value.mid(firstNewline + 1);
            const int fenceEnd = value.lastIndexOf(QStringLiteral("```"));
            if (fenceEnd >= 0) {
                value = value.left(fenceEnd);
            }
        }
    }

    if ((value.startsWith('"') && value.endsWith('"'))
        || (value.startsWith('\'') && value.endsWith('\''))) {
        value = value.mid(1, value.size() - 2);
    }

    return value.trimmed();
}

QString buildToolCallSignature(const ToolCall& toolCall) {
    return toolCall.name.trimmed().toLower()
        + QStringLiteral("|")
        + QString::fromUtf8(QJsonDocument(toolCall.arguments).toJson(QJsonDocument::Compact));
}

QString stripInternalNoise(const QString& text) {
    QStringList kept;
    QSet<QString> seen;
    const QStringList lines = text.split('\n');
    for (QString line : lines) {
        const QString simplified = line.simplified();
        if (simplified.isEmpty()) {
            kept.append(line);
            continue;
        }

        const QString lower = simplified.toLower();
        if (lower.contains(QStringLiteral("tool execution blocked"))
            || lower.contains(QStringLiteral("tool not found"))
            || lower.contains(QStringLiteral("repetitive call detected"))
            || lower.contains(QStringLiteral("recent tool result"))
            || lower.contains(QStringLiteral("reader fallback timed out"))
            || lower.contains(QStringLiteral("target site returned 403"))) {
            continue;
        }

        const bool boilerplate = simplified.startsWith(QStringLiteral("我来帮您创作"))
            || simplified.startsWith(QStringLiteral("我来为您创作"))
            || simplified.startsWith(QStringLiteral("首先让我"))
            || simplified.startsWith(QStringLiteral("让我先"))
            || simplified.startsWith(QStringLiteral("根据当前项目状态"));
        if (boilerplate && seen.contains(simplified)) {
            continue;
        }

        seen.insert(simplified);
        kept.append(line);
    }

    QString cleaned = kept.join('\n').trimmed();
    cleaned.replace(QRegularExpression(QStringLiteral("\\n{3,}")), QStringLiteral("\n\n"));
    return cleaned;
}

void upsertToolArg(QJsonObject* args, const QString& key, const QString& rawValue) {
    if (!args) {
        return;
    }
    const QString normalizedKey = key.trimmed();
    if (normalizedKey.isEmpty()) {
        return;
    }
    const QString normalizedValue = normalizeTaggedValue(rawValue);
    if (normalizedValue.isEmpty()) {
        return;
    }
    args->insert(normalizedKey, normalizedValue);
}

void extractJsonArgs(const QString& text, QJsonObject* args) {
    if (!args) {
        return;
    }

    const int firstBrace = text.indexOf('{');
    const int lastBrace = text.lastIndexOf('}');
    if (firstBrace < 0 || lastBrace <= firstBrace) {
        return;
    }

    QJsonParseError parseError;
    const QString candidate = text.mid(firstBrace, lastBrace - firstBrace + 1).trimmed();
    const QJsonDocument doc = QJsonDocument::fromJson(candidate.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return;
    }

    const QJsonObject obj = doc.object();
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        upsertToolArg(args, it.key(), jsonValueToString(it.value()));
    }
}

QJsonObject parseTaggedArguments(const QString& text) {
    QJsonObject args;

    const QRegularExpression nestedToolArgRe(
        QStringLiteral("(?is)<tool_call>\\s*([A-Za-z0-9_\\-\\.]+)\\s*<arg_value>\\s*([\\s\\S]*?)\\s*(?:</arg_value>\\s*)?</tool_call>"));
    QRegularExpressionMatchIterator nestedIt = nestedToolArgRe.globalMatch(text);
    while (nestedIt.hasNext()) {
        const QRegularExpressionMatch m = nestedIt.next();
        upsertToolArg(&args, m.captured(1), m.captured(2));
    }

    const QRegularExpression argKeyValueRe(
        QStringLiteral("(?is)<arg_key>\\s*([A-Za-z0-9_\\-\\.]+)\\s*</arg_key>\\s*<arg_value>\\s*([\\s\\S]*?)\\s*(?:</arg_value>)?"));
    QRegularExpressionMatchIterator argPairIt = argKeyValueRe.globalMatch(text);
    while (argPairIt.hasNext()) {
        const QRegularExpressionMatch m = argPairIt.next();
        upsertToolArg(&args, m.captured(1), m.captured(2));
    }

    const QRegularExpression argKeyRawRe(
        QStringLiteral("(?is)<arg_key>\\s*([A-Za-z0-9_\\-\\.]+)\\s*</arg_key>\\s*([^<\\n][\\s\\S]*?)(?=(?:<arg_key>|<tool_call>|</tool_call>|$))"));
    QRegularExpressionMatchIterator argRawIt = argKeyRawRe.globalMatch(text);
    while (argRawIt.hasNext()) {
        const QRegularExpressionMatch m = argRawIt.next();
        upsertToolArg(&args, m.captured(1), m.captured(2));
    }

    const QRegularExpression xmlArgRe(QStringLiteral("(?is)<([A-Za-z0-9_\\-\\.]+)>\\s*([\\s\\S]*?)\\s*</\\1>"));
    QRegularExpressionMatchIterator xmlIt = xmlArgRe.globalMatch(text);
    while (xmlIt.hasNext()) {
        const QRegularExpressionMatch m = xmlIt.next();
        const QString key = m.captured(1).trimmed();
        if (key.compare(QStringLiteral("tool_call"), Qt::CaseInsensitive) == 0
            || key.compare(QStringLiteral("arg_key"), Qt::CaseInsensitive) == 0
            || key.compare(QStringLiteral("arg_value"), Qt::CaseInsensitive) == 0) {
            continue;
        }
        upsertToolArg(&args, key, m.captured(2));
    }

    if (args.isEmpty()) {
        extractJsonArgs(text, &args);
    }

    return args;
}

QString firstRequiredToolArgName(const QString& toolName) {
    ITool* tool = ToolRegistry::instance().getTool(toolName);
    if (!tool) {
        return QString();
    }

    const QJsonArray required = tool->parameters().value(QStringLiteral("required")).toArray();
    for (const QJsonValue& value : required) {
        const QString key = value.toString().trimmed();
        if (!key.isEmpty()) {
            return key;
        }
    }
    return QString();
}

void normalizeToolArgsForCall(const QString& toolName, const QString& rawCallBody, QJsonObject* args) {
    if (!args) {
        return;
    }

    const QString requiredKey = firstRequiredToolArgName(toolName);
    if (requiredKey.isEmpty()) {
        return;
    }

    auto requiredValue = [&]() -> QString {
        return args->value(requiredKey).toString().trimmed();
    };

    // 兼容形如 <tool_call>list_directory<arg_value>...</arg_value></tool_call> 的写法，
    // 这类写法会被解析为 key=list_directory，需要映射到真正的必填参数名。
    if (requiredValue().isEmpty()) {
        const QString toolKeyValue = args->value(toolName).toString().trimmed();
        if (!toolKeyValue.isEmpty()) {
            args->insert(requiredKey, toolKeyValue);
            args->remove(toolName);
        }
    }

    if (requiredValue().isEmpty()) {
        const QRegularExpression loneArgRe(QStringLiteral("(?is)<arg_value>\\s*([\\s\\S]*?)\\s*(?:</arg_value>)?"));
        const QRegularExpressionMatch loneArgMatch = loneArgRe.match(rawCallBody);
        if (loneArgMatch.hasMatch()) {
            const QString loneValue = normalizeTaggedValue(loneArgMatch.captured(1));
            if (!loneValue.isEmpty()) {
                args->insert(requiredKey, loneValue);
            }
        }
    }

    if (requiredValue().isEmpty() && args->size() == 1) {
        const QJsonObject::const_iterator it = args->constBegin();
        const QString guessed = normalizeTaggedValue(jsonValueToString(it.value()));
        if (!guessed.isEmpty()) {
            args->insert(requiredKey, guessed);
        }
    }
}

QList<CorePendingToolCall> parseTaggedToolCalls(const QString& text) {
    QList<CorePendingToolCall> parsed;
    if (!text.contains(QStringLiteral("<tool_call>"), Qt::CaseInsensitive)) {
        return parsed;
    }

    const QRegularExpression callBlockRe(
        QStringLiteral("(?is)<tool_call>\\s*([A-Za-z0-9_\\-\\.]+)\\s*([\\s\\S]*?)</tool_call>"));
    QRegularExpressionMatchIterator callIt = callBlockRe.globalMatch(text);
    while (callIt.hasNext()) {
        const QRegularExpressionMatch m = callIt.next();
        const QString toolName = m.captured(1).trimmed();
        if (toolName.isEmpty() || !ToolRegistry::instance().hasTool(toolName)) {
            continue;
        }

        QJsonObject args = parseTaggedArguments(m.captured(2));
        normalizeToolArgsForCall(toolName, m.captured(2), &args);

        CorePendingToolCall call;
        call.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        call.name = toolName;
        call.arguments = QString::fromUtf8(QJsonDocument(args).toJson(QJsonDocument::Compact));
        parsed.append(call);
    }

    if (!parsed.isEmpty()) {
        return parsed;
    }

    const QRegularExpression firstToolRe(QStringLiteral("(?is)<tool_call>\\s*([A-Za-z0-9_\\-\\.]+)"));
    const QRegularExpressionMatch firstMatch = firstToolRe.match(text);
    const QString toolName = firstMatch.hasMatch() ? firstMatch.captured(1).trimmed() : QString();
    if (toolName.isEmpty()) {
        return parsed;
    }

    QJsonObject args = parseTaggedArguments(text);
    normalizeToolArgsForCall(toolName, text, &args);

    CorePendingToolCall call;
    call.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    call.name = toolName;
    call.arguments = QString::fromUtf8(QJsonDocument(args).toJson(QJsonDocument::Compact));
    parsed.append(call);
    return parsed;
}

int contextCompressionThreshold(const ProviderConfig& config) {
    const int contextLimit = modelctx::inferModelContextLimit(config);
    return qMax(1, static_cast<int>(contextLimit * 0.6));
}

bool isAuthLikeToolFailure(const QString& text) {
    const QString normalized = text.toLower();
    return normalized.contains(QStringLiteral("apikeyinvalid"))
        || normalized.contains(QStringLiteral("api key is invalid"))
        || normalized.contains(QStringLiteral("api key invalid"))
        || normalized.contains(QStringLiteral("unauthorized"))
        || normalized.contains(QStringLiteral("forbidden"))
        || normalized.contains(QStringLiteral("invalid token"))
        || normalized.contains(QStringLiteral("http get failed (401)"))
        || normalized.contains(QStringLiteral("http post failed (401)"))
        || normalized.contains(QStringLiteral("主机需要验证"))
        || normalized.contains(QStringLiteral("鉴权"))
        || normalized.contains(QStringLiteral("认证失败"));
}

}

ReactAgentCore::ReactAgentCore(QObject* parent)
    : IAgentCore(parent)
    , m_provider(nullptr)
    , m_memory(nullptr)
    , m_executor(nullptr)
    , m_providerConfig()
    , m_runGeneration(0)
    , m_iteration(0)
    , m_noToolIterationStreak(0)
    , m_retryCount(0)
    , m_lastContextTokens(0)
    , m_consecutiveToolFailureRounds(0)
    , m_stopped(false)
    , m_hadToolFailure(false) {}

void ReactAgentCore::setProvider(ILLMProvider* provider) {
    m_provider = provider;
}

void ReactAgentCore::setMemory(IMemorySystem* memory) {
    m_memory = memory;
}

void ReactAgentCore::setExecutor(ToolExecutor* executor) {
    m_executor = executor;
}

void ReactAgentCore::setProviderConfig(const ProviderConfig& config) {
    m_providerConfig = config;
}

void ReactAgentCore::setRuntimeToolNames(const QStringList& toolNames) {
    m_runtimeToolNames = toolNames;
}

void ReactAgentCore::run(const QString& input) {
    if (!m_provider) {
        LOG_ERROR(QStringLiteral("ReactAgentCore run failed: provider is null"));
        emit errorOccurred("No provider configured");
        return;
    }

    m_stopped = false;
    ++m_runGeneration;
    m_iteration = 0;
    m_currentThought.clear();
    m_currentContent.clear();
    m_pendingToolCalls.clear();
    m_preExecutingTools.clear();
    m_preExecutedResults.clear();
    m_parsedToolIds.clear();
    m_toolCallExecutionCounts.clear();
    m_noToolIterationStreak = 0;
    m_retryCount = 0;
    m_lastContextTokens = 0;
    m_consecutiveToolFailureRounds = 0;
    m_hadToolFailure = false;

    m_messages = m_context;
    
    Message userMsg(MessageRole::User, input);
    m_messages.append(userMsg);
    if (m_memory) {
        m_memory->addMessage(userMsg);
    }

    emit conversationUpdated(m_messages);
    
    processIteration();
}

void ReactAgentCore::stop() {
    m_stopped = true;
    ++m_runGeneration;
    m_preExecutingTools.clear();
    m_preExecutedResults.clear();
    m_pendingToolCalls.clear();
    m_toolCallExecutionCounts.clear();
    if (m_provider) {
        m_provider->abort();
    }
}

void ReactAgentCore::setContext(const MessageList& messages) {
    m_context = messages;
    m_messages = messages;
}

void ReactAgentCore::setSystemPrompt(const QString& prompt) {
    m_systemPrompt = prompt;
}

void ReactAgentCore::processIteration() {
    if (finalizeIfStoppedOrExceeded()) {
        return;
    }
    
    m_iteration++;
    LOG_INFO(QStringLiteral("ReactAgentCore iteration started: %1").arg(m_iteration));
    m_parsedToolIds.clear();

    MessageList messages = buildIterationMessages();

    ChatOptions options;
    options.model = m_providerConfig.model;
    options.temperature = m_providerConfig.temperature;
    options.maxTokens = m_providerConfig.maxTokens;
    options.stream = m_provider->supportsStreaming();

    if (m_provider->supportsTools()) {
        if (!m_runtimeToolNames.isEmpty()) {
            QJsonArray scopedTools;
            for (const QString& toolName : m_runtimeToolNames) {
                ITool* tool = ToolRegistry::instance().getTool(toolName.trimmed());
                if (tool) {
                    scopedTools.append(tool->toJson());
                }
            }
            options.tools = scopedTools;
        } else {
            options.tools = ToolRegistry::instance().toolsSchema();
        }
    }

    const quint64 runGeneration = m_runGeneration;

    QPointer<ILLMProvider> provider(m_provider);
    if (!provider) {
        emit errorOccurred(QStringLiteral("Provider 已失效"));
        return;
    }

    if (!options.stream) {
        auto startNonStream = [this, provider, messages, options, runGeneration]() {
            if (!provider) {
                return;
            }
            provider->chat(messages, options,
                [this, runGeneration](const QString& response) {
                    auto handleResponse = [this, runGeneration, response]() {
                        if (m_stopped || runGeneration != m_runGeneration) {
                            return;
                        }
                        if (response.isEmpty()) {
                            handleStreamCompleted(runGeneration);
                            return;
                        }

                        auto payload = std::make_shared<QString>(response);
                        auto cursor = std::make_shared<int>(0);
                        auto pump = std::make_shared<std::function<void()>>();
                        *pump = [this, runGeneration, payload, cursor, pump]() {
                            if (m_stopped || runGeneration != m_runGeneration) {
                                return;
                            }
                            if (*cursor >= payload->size()) {
                                handleStreamCompleted(runGeneration);
                                return;
                            }
                            const int remaining = payload->size() - *cursor;
                            int chunkSize = 12;
                            if (remaining > 2400) {
                                chunkSize = 56;
                            } else if (remaining > 1400) {
                                chunkSize = 44;
                            } else if (remaining > 720) {
                                chunkSize = 32;
                            } else if (remaining > 280) {
                                chunkSize = 24;
                            } else if (remaining > 120) {
                                chunkSize = 18;
                            }
                            StreamChunk chunk;
                            chunk.content = payload->mid(*cursor, chunkSize);
                            *cursor += chunkSize;
                            handleStreamChunk(chunk, runGeneration);
                            int delayMs = 7;
                            if (!chunk.content.isEmpty()) {
                                const QChar tail = chunk.content.back();
                                if (tail == '\n') {
                                    delayMs = 16;
                                } else if (QStringLiteral(".,!?;:，。！？；：").contains(tail)) {
                                    delayMs = 11;
                                }
                            }
                            QTimer::singleShot(delayMs, this, [pump]() { (*pump)(); });
                        };
                        (*pump)();
                    };
                    if (this->thread() == QThread::currentThread()) {
                        handleResponse();
                    } else {
                        QMetaObject::invokeMethod(this, handleResponse, Qt::QueuedConnection);
                    }
                },
                [this, runGeneration](const QString& error) {
                    auto handleError = [this, runGeneration, error]() {
                        handleStreamError(error, runGeneration);
                    };
                    if (this->thread() == QThread::currentThread()) {
                        handleError();
                    } else {
                        QMetaObject::invokeMethod(this, handleError, Qt::QueuedConnection);
                    }
                }
            );
        };
        if (provider->thread() == QThread::currentThread()) {
            startNonStream();
        } else {
            QMetaObject::invokeMethod(provider, startNonStream, Qt::QueuedConnection);
        }
        return;
    }

    auto startStream = [this, provider, messages, options, runGeneration]() {
        if (!provider) {
            return;
        }
        provider->chatStream(messages, options,
            [this, runGeneration](const StreamChunk& chunk) {
                auto handleChunk = [this, runGeneration, chunk]() {
                    handleStreamChunk(chunk, runGeneration);
                };
                if (this->thread() == QThread::currentThread()) {
                    handleChunk();
                } else {
                    QMetaObject::invokeMethod(this, handleChunk, Qt::QueuedConnection);
                }
            },
            [this, runGeneration]() {
                auto handleComplete = [this, runGeneration]() {
                    handleStreamCompleted(runGeneration);
                };
                if (this->thread() == QThread::currentThread()) {
                    handleComplete();
                } else {
                    QMetaObject::invokeMethod(this, handleComplete, Qt::QueuedConnection);
                }
            },
            [this, runGeneration](const QString& error) {
                auto handleError = [this, runGeneration, error]() {
                    handleStreamError(error, runGeneration);
                };
                if (this->thread() == QThread::currentThread()) {
                    handleError();
                } else {
                    QMetaObject::invokeMethod(this, handleError, Qt::QueuedConnection);
                }
            }
        );
    };
    if (provider->thread() == QThread::currentThread()) {
        startStream();
    } else {
        QMetaObject::invokeMethod(provider, startStream, Qt::QueuedConnection);
    }
}

bool ReactAgentCore::finalizeIfStoppedOrExceeded() {
    // 使用 7 种继续策略评估当前状态
    ContinueReason reason = evaluateContinueReason();
    
    if (reason == ContinueReason::NormalCompletion) {
        // 正常情况，继续执行
        return false;
    }
    
    // 其他情况，处理继续策略
    handleContinueReason(reason);
    
    // 非 NormalCompletion 都已在 handleContinueReason 内部完成“终止或续跑”，
    // 当前这一层不应再继续向下执行，否则会出现重复迭代和状态错乱。
    return true;
}

MessageList ReactAgentCore::buildIterationMessages() const {
    MessageList messages;
    QString mergedSystemPrompt = m_systemPrompt;
    if (m_memory) {
        const QStringList semanticMemory = m_memory->queryRelevantMemory(
            m_messages.isEmpty() ? QString() : m_messages.last().content, 6);
        if (!semanticMemory.isEmpty()) {
            mergedSystemPrompt += QStringLiteral("\n\n[Relevant Memory]\n- %1")
                .arg(semanticMemory.join(QStringLiteral("\n- ")));
        }
    }
    if (!mergedSystemPrompt.isEmpty()) {
        messages.append(Message(MessageRole::System, mergedSystemPrompt));
    }

    MessageList contextMessages = m_messages;
    compressMessagesForContext(contextMessages);
    messages.append(contextMessages);
    return messages;
}

void ReactAgentCore::handleStreamChunk(const StreamChunk& chunk, quint64 runGeneration) {
    if (m_stopped || runGeneration != m_runGeneration) {
        return;
    }

    if (!chunk.content.isEmpty()) {
        m_currentThought += chunk.content;
        m_currentContent += chunk.content;
    }
    emit responseChunk(chunk);

    if (chunk.hasUsage) {
        emit usageReport(chunk.promptTokens, chunk.completionTokens, chunk.totalTokens);
    }

    for (const ToolCallDelta& tc : chunk.toolCalls) {
        if (tc.index < 0) {
            continue;
        }

        while (m_pendingToolCalls.size() <= tc.index) {
            m_pendingToolCalls.append(CorePendingToolCall{});
        }

        if (!tc.id.isEmpty()) {
            m_pendingToolCalls[tc.index].id = tc.id;
        }
        if (!tc.name.isEmpty()) {
            m_pendingToolCalls[tc.index].name = tc.name;
        }
        if (!tc.arguments.isEmpty()) {
            m_pendingToolCalls[tc.index].arguments += tc.arguments;
        }
        
        // 检测工具调用是否完整，如果完整则通知 UI
        const CorePendingToolCall& pending = m_pendingToolCalls[tc.index];
        if (isToolCallComplete(pending) && !m_parsedToolIds.contains(pending.id)) {
            m_parsedToolIds.insert(pending.id);
            ToolCall toolCall = createToolCall(pending);
            emit toolCallParsed(toolCall);
        }
    }
}

void ReactAgentCore::handleStreamCompleted(quint64 runGeneration) {
    if (m_stopped || runGeneration != m_runGeneration) {
        return;
    }
    
    // 流式完成后立即启动工具预执行（异步非阻塞）
    startToolPreExecution();
    
    processThought();
}

void ReactAgentCore::handleStreamError(const QString& error, quint64 runGeneration) {
    if (m_stopped || runGeneration != m_runGeneration) {
        return;
    }
    const QString normalizedError = error.trimmed().isEmpty()
        ? QStringLiteral("模型流式调用失败")
        : error;
    
    LOG_ERROR(QStringLiteral("ReactAgentCore stream error at iteration %1: %2")
        .arg(m_iteration)
        .arg(error.isEmpty() ? QStringLiteral("Unknown error") : error));
    emit errorOccurred(normalizedError);
    
    // 触发流式错误策略
    handleContinueReason(ContinueReason::StreamError);
}

void ReactAgentCore::processThought() {
    if (m_stopped) {
        finalizeResponse(m_currentContent.isEmpty() ? m_currentThought : m_currentContent);
        return;
    }

    if (hasFinalAnswerMarker(m_currentContent) || hasFinalAnswerMarker(m_currentThought)) {
        QString finalText = m_currentContent.isEmpty() ? m_currentThought : m_currentContent;
        finalText.remove(QRegularExpression(QStringLiteral("(?i)\\bfinal[_\\s-]*answer\\b\\s*[:：-]?")));
        finalText = finalText.trimmed();
        finalizeResponse(finalText);
        return;
    }
    
    bool hasNamedPendingTool = false;
    for (const CorePendingToolCall& pending : m_pendingToolCalls) {
        if (!pending.name.trimmed().isEmpty()) {
            hasNamedPendingTool = true;
            break;
        }
    }

    if (!hasNamedPendingTool) {
        const QList<CorePendingToolCall> taggedCalls = parseTaggedToolCalls(m_currentContent);
        if (!taggedCalls.isEmpty()) {
            for (const CorePendingToolCall& call : taggedCalls) {
                m_pendingToolCalls.append(call);
                if (!m_parsedToolIds.contains(call.id)) {
                    m_parsedToolIds.insert(call.id);
                    emit toolCallParsed(createToolCall(call));
                }
            }
            m_currentContent.remove(QRegularExpression(QStringLiteral("(?is)<tool_call>[\\s\\S]*?</tool_call>")));
            m_currentContent.remove(QRegularExpression(QStringLiteral("(?is)</?arg_(?:key|value)>")));
            m_currentThought = m_currentContent;
        }
    }

    if (!m_pendingToolCalls.isEmpty()) {
        m_noToolIterationStreak = 0;
        LOG_INFO(QStringLiteral("ReactAgentCore executing %1 pending tool call(s)").arg(m_pendingToolCalls.size()));
        executeToolCalls();
    } else {
        ++m_noToolIterationStreak;
        const QString trimmedContent = m_currentContent.trimmed();
        if (!trimmedContent.isEmpty()) {
            const bool shouldContinue = shouldContinueForInterimContent(trimmedContent)
                || isLikelyTruncatedAssistantTurn(trimmedContent);
            if (shouldContinue
                && m_noToolIterationStreak < (constants::MAX_NO_TOOL_STREAK + 2)) {
                LOG_INFO(QStringLiteral("ReactAgentCore detected interim content, continue iteration %1 (streak=%2)")
                    .arg(m_iteration)
                    .arg(m_noToolIterationStreak));
                Message partialAssistant(MessageRole::Assistant, trimmedContent);
                m_messages.append(partialAssistant);
                if (m_memory) {
                    m_memory->addMessage(partialAssistant);
                }
                appendRecoveryMessage(QStringLiteral("继续从上文末尾生成，不要重复已输出内容。"));
                emit conversationUpdated(m_messages);
                m_currentThought.clear();
                m_currentContent.clear();
                m_noToolIterationStreak = 0;
            } else {
                LOG_INFO(QStringLiteral("ReactAgentCore completed with direct assistant content in iteration %1").arg(m_iteration));
                finalizeResponse(trimmedContent);
                return;
            }
        }

        if (shouldTerminateByNoToolStreak()) {
            LOG_INFO(QStringLiteral("ReactAgentCore terminated by no-tool streak: %1").arg(m_noToolIterationStreak));
            finalizeResponse(m_currentContent);
            return;
        }

        const quint64 runGeneration = m_runGeneration;
        QTimer::singleShot(80, this, [this, runGeneration]() {
            if (m_stopped || runGeneration != m_runGeneration) {
                return;
            }
            processIteration();
        });
    }
}

void ReactAgentCore::executeToolCalls() {
    if (m_pendingToolCalls.isEmpty()) {
        finalizeResponse(m_currentContent);
        return;
    }

    // 先等待所有预执行的工具完成
    checkPreExecutionResults();

    // 准备工具调用列表
    QVector<ToolCall> validToolCalls;
    QVector<int> validToolIndexes;  // 记录有效工具在原始列表中的索引

    for (int i = 0; i < m_pendingToolCalls.size(); ++i) {
        const CorePendingToolCall& pendingToolCall = m_pendingToolCalls[i];
        
        if (pendingToolCall.name.isEmpty()) {
            LOG_WARN(QStringLiteral("Skipping pending tool call with empty name"));
            Message toolMsg(MessageRole::Tool, QStringLiteral("Tool execution failed: empty tool name"));
            toolMsg.toolCallId = pendingToolCall.id;
            m_messages.append(toolMsg);
            if (m_memory) {
                m_memory->addMessage(toolMsg);
            }
            continue;
        }

        ToolCall toolCall;
        toolCall.id = pendingToolCall.id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : pendingToolCall.id;
        toolCall.name = pendingToolCall.name;

        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(pendingToolCall.arguments.toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError && document.isObject()) {
            toolCall.arguments = document.object();
        } else if (!pendingToolCall.arguments.trimmed().isEmpty()) {
            LOG_WARN(QStringLiteral("Tool arguments JSON parse failed for %1: %2")
                         .arg(toolCall.name, parseError.errorString()));
            ToolResult invalidArgsResult(toolCall.id, false, QStringLiteral("Invalid tool arguments JSON"));
            invalidArgsResult.metadata[QStringLiteral("tool_name")] = toolCall.name;
            invalidArgsResult.metadata[QStringLiteral("parse_error")] = parseError.errorString();
            emit toolCallRequested(toolCall);

            Message toolMsg(MessageRole::Tool, invalidArgsResult.content);
            toolMsg.toolCallId = toolCall.id;
            toolMsg.name = toolCall.name;
            m_messages.append(toolMsg);
            if (m_memory) {
                m_memory->addMessage(toolMsg);
            }
            continue;
        }

        const QString signature = buildToolCallSignature(toolCall);
        const int executedCount = m_toolCallExecutionCounts.value(signature, 0);
        if (executedCount >= 3) {
            LOG_WARN(QStringLiteral("Blocking repetitive tool call '%1' after %2 executions")
                .arg(toolCall.name)
                .arg(executedCount));
            m_hadToolFailure = true;
            Message toolMsg(
                MessageRole::Tool,
                QStringLiteral("Tool execution blocked: repetitive call detected (%1). Please infer from previous result instead of repeating this call.")
                    .arg(toolCall.name));
            toolMsg.toolCallId = toolCall.id;
            toolMsg.name = toolCall.name;
            m_messages.append(toolMsg);
            if (m_memory) {
                m_memory->addMessage(toolMsg);
            }
            continue;
        }

        m_toolCallExecutionCounts.insert(signature, executedCount + 1);
        emit toolCallRequested(toolCall);
        validToolCalls.append(toolCall);
        validToolIndexes.append(i);
    }

    if (!m_executor) {
        if (!validToolCalls.isEmpty()) {
            emit errorOccurred(QStringLiteral("No tool executor configured"));
            return;
        }

        // 没有可执行工具（例如全部参数无效）时，仍继续下一轮，保持原有循环行为。
        emit conversationUpdated(m_messages);
        m_pendingToolCalls.clear();
        m_currentThought.clear();
        m_currentContent.clear();
        const quint64 runGeneration = m_runGeneration;
        QTimer::singleShot(100, this, [this, runGeneration]() {
            if (m_stopped || runGeneration != m_runGeneration) {
                return;
            }
            processIteration();
        });
        return;
    }

    LOG_INFO(QStringLiteral("Executing %1 tools (with pre-execution cache)").arg(validToolCalls.size()));
    QVector<ToolResult> results;
    results.reserve(validToolCalls.size());
    QVector<ToolCall> remainingToolCalls;
    QVector<int> remainingIndexes;

    for (int i = 0; i < validToolCalls.size(); ++i) {
        const ToolCall& call = validToolCalls[i];
        if (m_preExecutedResults.contains(call.id)) {
            results.append(m_preExecutedResults.take(call.id));
        } else {
            remainingToolCalls.append(call);
            remainingIndexes.append(i);
            results.append(ToolResult(call.id, false, QStringLiteral("__placeholder__")));
        }
    }

    if (!remainingToolCalls.isEmpty()) {
        QVector<ToolResult> executed = m_executor->executeBatch(remainingToolCalls);
        for (int j = 0; j < executed.size(); ++j) {
            results[remainingIndexes[j]] = executed[j];
        }
    }

    // 处理结果
    int failedCount = 0;
    QStringList failureSummaries;
    for (int i = 0; i < results.size(); ++i) {
        const ToolResult& result = results[i];
        const ToolCall& toolCall = validToolCalls[i];
        
        LOG_INFO(QStringLiteral("Tool executed: %1, success=%2").arg(toolCall.name, result.success ? "true" : "false"));

        // 标记工具失败
        if (!result.success) {
            m_hadToolFailure = true;
            ++failedCount;
            const QString compact = result.content.simplified();
            failureSummaries.append(QStringLiteral("%1: %2")
                                        .arg(toolCall.name,
                                             compact.left(120) + (compact.size() > 120 ? QStringLiteral("...") : QString())));
        }

        const QString toolContent = result.content.trimmed().isEmpty()
            ? (result.success ? QStringLiteral("Tool executed successfully") : QStringLiteral("Tool execution failed"))
            : result.content;
        Message toolMsg(MessageRole::Tool, toolContent);
        toolMsg.toolCallId = toolCall.id;
        toolMsg.name = toolCall.name;
        m_messages.append(toolMsg);

        if (m_memory) {
            m_memory->addMessage(toolMsg);
        }
    }

    emit conversationUpdated(m_messages);

    if (!results.isEmpty() && failedCount == results.size()) {
        bool authFailure = true;
        for (const ToolResult& result : results) {
            authFailure = authFailure && isAuthLikeToolFailure(result.content);
        }
        if (authFailure) {
            appendRecoveryMessage(QStringLiteral("检测到工具鉴权失败，已停止自动重试。"));
            finalizeResponse(QStringLiteral("工具调用鉴权失败（例如 API Key 无效/过期）。请先更新工具相关凭据后重试。"));
            return;
        }
    }
 
    if (!results.isEmpty() && failedCount == results.size()) {
        ++m_consecutiveToolFailureRounds;
    } else if (failedCount == 0) {
        m_consecutiveToolFailureRounds = 0;
    } else {
        m_consecutiveToolFailureRounds = qMax(0, m_consecutiveToolFailureRounds - 1);
    }

    if (m_consecutiveToolFailureRounds >= 2) {
        const QString details = failureSummaries.isEmpty()
            ? QStringLiteral("未知错误")
            : failureSummaries.mid(0, 3).join(QStringLiteral(" | "));
        appendRecoveryMessage(QStringLiteral("工具连续失败，已停止自动重试。"));
        finalizeResponse(QStringLiteral("工具持续调用失败，已停止继续调用。\n请检查网络、权限或工具参数后重试。\n最近失败：%1").arg(details));
        return;
    }

    m_pendingToolCalls.clear();
    m_currentThought.clear();
    m_currentContent.clear();
    const quint64 runGeneration = m_runGeneration;
    QTimer::singleShot(100, this, [this, runGeneration]() {
        if (m_stopped || runGeneration != m_runGeneration) {
            return;
        }
        processIteration();
    });
}

bool ReactAgentCore::shouldTerminateByNoToolStreak() const {
    return m_noToolIterationStreak >= constants::MAX_NO_TOOL_STREAK;
}

bool ReactAgentCore::shouldContinueForInterimContent(const QString& text) const {
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    static const QRegularExpression continuationRegex(
        QStringLiteral("(?i)(继续查看|继续读取|继续调用|继续搜索|next,? I will use|let me call|let me search)"));
    if (continuationRegex.match(trimmed).hasMatch()) {
        return true;
    }

    if (trimmed.endsWith(QStringLiteral("："))
        || trimmed.endsWith(QStringLiteral(":"))
        || trimmed.endsWith(QStringLiteral("..."))
        || trimmed.endsWith(QStringLiteral("…"))) {
        return true;
    }

    return false;
}

bool ReactAgentCore::isLikelyTruncatedAssistantTurn(const QString& text) const {
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    const int fenceCount = trimmed.count(QStringLiteral("```"));
    if ((fenceCount % 2) != 0) {
        return true;
    }

    if (trimmed.endsWith(QStringLiteral("，"))
        || trimmed.endsWith(QStringLiteral(","))
        || trimmed.endsWith(QStringLiteral("、"))
        || trimmed.endsWith(QStringLiteral("（"))
        || trimmed.endsWith(QStringLiteral("("))
        || trimmed.endsWith(QStringLiteral("例如"))
        || trimmed.endsWith(QStringLiteral("比如"))
        || trimmed.endsWith(QStringLiteral("包括"))
        || trimmed.endsWith(QStringLiteral("如下"))
        || trimmed.endsWith(QStringLiteral("-"))) {
        return true;
    }

    // 长文本但结尾无自然终止符，通常是 hit max_tokens 的截断形态。
    if (trimmed.size() >= 360) {
        const QChar tail = trimmed.back();
        const QString endings = QStringLiteral(".!?。！？”’】]}");
        if (!endings.contains(tail)) {
            return true;
        }
    }

    return false;
}

bool ReactAgentCore::hasFinalAnswerMarker(const QString& text) const {
    static const QRegularExpression markerRegex(
        QStringLiteral("(?i)\\bfinal[_\\s-]*answer\\b|\\bfinal[_\\s-]*response\\b"));
    return markerRegex.match(text).hasMatch();
}

void ReactAgentCore::finalizeResponse(const QString& finalMessage) {
    QString normalizedMessage = stripInternalNoise(finalMessage);
    if (normalizedMessage.isEmpty()) {
        QString latestToolSnippet;
        for (auto it = m_messages.crbegin(); it != m_messages.crend(); ++it) {
            if (it->role != MessageRole::Tool) {
                continue;
            }
            latestToolSnippet = it->content.simplified();
            if (!latestToolSnippet.isEmpty()) {
                break;
            }
        }

        if (!latestToolSnippet.isEmpty()) {
            if (latestToolSnippet.size() > 160) {
                latestToolSnippet = latestToolSnippet.left(160) + QStringLiteral("...");
            }
            normalizedMessage = QStringLiteral("模型未给出最终回答。最近工具结果：%1").arg(latestToolSnippet);
        } else {
            normalizedMessage = QStringLiteral("（模型返回空响应）");
        }
    }
    // 统一在收尾阶段落地 assistant 消息，保证 UI 与 memory 视图一致。
    Message assistantMsg(MessageRole::Assistant, normalizedMessage);
    m_messages.append(assistantMsg);

    if (m_memory) {
        m_memory->addMessage(assistantMsg);
    }

    emit conversationUpdated(m_messages);
    emit completed(normalizedMessage);
}

void ReactAgentCore::compressMessagesForContext(MessageList& messages) const {
    if (messages.size() <= constants::MEMORY_KEEP_FIRST + constants::MEMORY_KEEP_LAST + 1) {
        return;
    }

    const int compressionThreshold = contextCompressionThreshold(m_providerConfig);

    int totalTokens = 0;
    for (const Message& message : messages) {
        totalTokens += estimateTokens(message.content);
    }

    if (totalTokens <= compressionThreshold) {
        return;
    }

    LOG_INFO(QStringLiteral("Starting progressive compression, current tokens: %1").arg(totalTokens));

    // 四级渐进式压缩流水线
    // Level 1: 裁剪（Trim）- 截断旧工具输出的大块内容
    int savedTokens = performLevel1Trim(messages);
    LOG_INFO(QStringLiteral("Level 1 Trim saved %1 tokens").arg(savedTokens));
    
    totalTokens = 0;
    for (const Message& message : messages) {
        totalTokens += estimateTokens(message.content);
    }
    if (totalTokens <= compressionThreshold) {
        LOG_INFO(QStringLiteral("Compression sufficient at Level 1"));
        return;
    }

    // Level 2: 去重（Dedupe）- 合并重复的工具调用结果
    savedTokens = performLevel2Dedupe(messages);
    LOG_INFO(QStringLiteral("Level 2 Dedupe saved %1 tokens").arg(savedTokens));
    
    totalTokens = 0;
    for (const Message& message : messages) {
        totalTokens += estimateTokens(message.content);
    }
    if (totalTokens <= compressionThreshold) {
        LOG_INFO(QStringLiteral("Compression sufficient at Level 2"));
        return;
    }

    // Level 3: 折叠（Fold）- 标记不活跃消息段（保留可展开）
    savedTokens = performLevel3Fold(messages);
    LOG_INFO(QStringLiteral("Level 3 Fold saved %1 tokens").arg(savedTokens));
    
    totalTokens = 0;
    for (const Message& message : messages) {
        totalTokens += estimateTokens(message.content);
    }
    if (totalTokens <= compressionThreshold) {
        LOG_INFO(QStringLiteral("Compression sufficient at Level 3"));
        return;
    }

    // Level 4: 摘要（Summarize）- 最后手段，语义摘要
    LOG_INFO(QStringLiteral("Compression insufficient, performing Level 4 Summarize"));
    performLevel4Summarize(messages);
}

int ReactAgentCore::performLevel1Trim(MessageList& messages) const {
    int savedTokens = 0;
    constexpr int MAX_TOOL_OUTPUT_LENGTH = 500;  // 工具输出最多保留 500 字符

    for (int i = 0; i < messages.size(); ++i) {
        Message& msg = messages[i];
        
        // 只处理工具消息
        if (msg.role != MessageRole::Tool) {
            continue;
        }

        // 如果是旧消息（前 3 个之外）且内容很长
        if (i >= constants::MEMORY_KEEP_FIRST && msg.content.length() > MAX_TOOL_OUTPUT_LENGTH) {
            const int originalLength = msg.content.length();
            msg.content = msg.content.left(MAX_TOOL_OUTPUT_LENGTH) + 
                         QStringLiteral("\n...[输出已截断]");
            savedTokens += estimateTokens(msg.content.mid(MAX_TOOL_OUTPUT_LENGTH, 
                                                          originalLength - MAX_TOOL_OUTPUT_LENGTH));
        }
    }

    return savedTokens;
}

int ReactAgentCore::performLevel2Dedupe(MessageList& messages) const {
    int savedTokens = 0;
    QMap<QString, int> toolCallSignatures;  // 签名 -> 首次出现索引

    for (int i = 0; i < messages.size(); ++i) {
        const Message& msg = messages[i];
        
        // 只处理工具消息
        if (msg.role != MessageRole::Tool) {
            continue;
        }

        // 生成签名：工具名 + 内容摘要
        QString signature = QString("%1:%2")
            .arg(msg.name, msg.content.left(100).simplified());

        if (toolCallSignatures.contains(signature)) {
            // 发现重复，记录需删除
            savedTokens += estimateTokens(msg.content);
            toolCallSignatures[signature] = i;  // 更新为最新位置
        } else {
            toolCallSignatures[signature] = i;
        }
    }

    // 反向删除重复项（避免索引失效）
    QList<int> indicesToRemove;
    for (auto it = toolCallSignatures.begin(); it != toolCallSignatures.end(); ++it) {
        // 如果同一签名出现多次，保留最后一次
        int firstIndex = it.value();
        for (int i = firstIndex - 1; i >= 0; --i) {
            if (messages[i].role == MessageRole::Tool && 
                messages[i].name == messages[firstIndex].name) {
                indicesToRemove.append(i);
            }
        }
    }

    std::sort(indicesToRemove.begin(), indicesToRemove.end(), std::greater<int>());
    for (int idx : indicesToRemove) {
        messages.removeAt(idx);
    }

    return savedTokens;
}

int ReactAgentCore::performLevel3Fold(MessageList& messages) const {
    int savedTokens = 0;
    constexpr int INACTIVE_THRESHOLD = 5;  // 5 轮未活跃

    // 标记不活跃段落（简化实现：折叠中间的连续消息）
    const int keepFirst = constants::MEMORY_KEEP_FIRST;
    const int keepLast = constants::MEMORY_KEEP_LAST;
    const int middleStart = keepFirst;
    const int middleEnd = qMax(middleStart, messages.size() - keepLast);

    if (middleEnd <= middleStart) {
        return 0;
    }

    // 统计中间段 token
    QString foldedContent;
    for (int i = middleStart; i < middleEnd; ++i) {
        savedTokens += estimateTokens(messages[i].content);
        
        // 生成折叠摘要（只保留角色和前 50 字符）
        QString roleLabel;
        switch (messages[i].role) {
            case MessageRole::User: roleLabel = QStringLiteral("User"); break;
            case MessageRole::Assistant: roleLabel = QStringLiteral("Assistant"); break;
            case MessageRole::Tool: roleLabel = QStringLiteral("Tool"); break;
            default: roleLabel = QStringLiteral("System"); break;
        }
        
        foldedContent += QString("%1: %2\n")
            .arg(roleLabel, messages[i].content.left(50).simplified());
    }

    // 替换中间段为折叠标记
    MessageList folded;
    for (int i = 0; i < keepFirst; ++i) {
        folded.append(messages[i]);
    }

    folded.append(Message(MessageRole::System, 
        QStringLiteral("[已折叠 %1 条历史消息]\n%2")
            .arg(middleEnd - middleStart)
            .arg(foldedContent)));

    for (int i = middleEnd; i < messages.size(); ++i) {
        folded.append(messages[i]);
    }

    messages = folded;
    return savedTokens;
}

void ReactAgentCore::performLevel4Summarize(MessageList& messages) const {
    // 这是最昂贵的压缩方式，需要调用 LLM 做摘要
    // 简化实现：直接使用现有的压缩摘要功能
    const int keepFirst = qMin(constants::MEMORY_KEEP_FIRST, messages.size());
    const int keepLast = qMin(constants::MEMORY_KEEP_LAST, qMax(0, messages.size() - keepFirst));
    const int middleStart = keepFirst;
    const int middleEnd = messages.size() - keepLast;
    
    if (middleEnd <= middleStart) {
        return;
    }

    MessageList compressed;
    for (int i = 0; i < keepFirst; ++i) {
        compressed.append(messages.at(i));
    }

    const QString summary = buildCompressedSummary(messages, middleStart, middleEnd);
    compressed.append(Message(
        MessageRole::System,
        QStringLiteral("[上下文语义摘要]\n%1").arg(summary)
    ));

    for (int i = middleEnd; i < messages.size(); ++i) {
        compressed.append(messages.at(i));
    }

    messages = compressed;
    LOG_INFO(QStringLiteral("Level 4 Summarize completed, reduced to %1 messages").arg(messages.size()));
}

int ReactAgentCore::estimateTokens(const QString& text) const {
    int hanChars = 0;
    int latinOrDigitChars = 0;
    int otherVisibleChars = 0;
    for (const QChar& c : text) {
        if (c.script() == QChar::Script_Han) {
            ++hanChars;
        } else if (c.isLetterOrNumber()) {
            ++latinOrDigitChars;
        } else if (!c.isSpace() && !c.isPunct()) {
            ++otherVisibleChars;
        }
    }
    const int latinTokens = (latinOrDigitChars + 3) / 4;
    const int otherTokens = (otherVisibleChars + 1) / 2;
    return qMax(1, hanChars + latinTokens + otherTokens);
}

QString ReactAgentCore::buildCompressedSummary(const MessageList& messages, int start, int end) const {
    QStringList lines;
    lines.append(QStringLiteral("以下内容来自历史中段，已压缩为摘要："));

    for (int i = start; i < end; ++i) {
        const Message& message = messages.at(i);
        QString roleLabel;
        switch (message.role) {
            case MessageRole::User: roleLabel = QStringLiteral("User"); break;
            case MessageRole::Assistant: roleLabel = QStringLiteral("Assistant"); break;
            case MessageRole::System: roleLabel = QStringLiteral("System"); break;
            case MessageRole::Tool: roleLabel = QStringLiteral("Tool"); break;
        }

        QString snippet = message.content.simplified();
        if (snippet.length() > 120) {
            snippet = snippet.left(120) + QStringLiteral("...");
        }
        lines.append(QStringLiteral("- %1: %2").arg(roleLabel, snippet));

        if (lines.size() >= 20) {
            lines.append(QStringLiteral("- ..."));
            break;
        }
    }

    return lines.join('\n');
}

// ============================================================================
// 7 种继续策略实现
// ============================================================================

ContinueReason ReactAgentCore::evaluateContinueReason() {
    // 1. 用户主动停止
    if (m_stopped) {
        return ContinueReason::UserStop;
    }
    
    // 2. 达到最大迭代次数
    if (m_iteration >= constants::MAX_ITERATIONS) {
        return ContinueReason::MaxIterationsReached;
    }
    
    // 3. 上下文窗口超限
    int contextTokens = estimateContextTokens();
    const int threshold = contextCompressionThreshold(m_providerConfig);
    if (contextTokens > threshold) {  // 60% 阈值
        return ContinueReason::ContextOverflow;
    }
    
    // 4. 工具执行失败
    if (m_hadToolFailure) {
        return ContinueReason::ToolExecutionFailure;
    }
    
    // 5. 连续无工具调用
    if (shouldTerminateByNoToolStreak()) {
        return ContinueReason::NoToolStreak;
    }
    
    // 6. 正常完成
    return ContinueReason::NormalCompletion;
}

void ReactAgentCore::handleContinueReason(ContinueReason reason) {
    switch (reason) {
        case ContinueReason::UserStop:
            LOG_INFO(QStringLiteral("ReactAgentCore stopped by user at iteration %1").arg(m_iteration));
            finalizeResponse(m_currentContent.isEmpty() ? m_currentThought : m_currentContent);
            break;
            
        case ContinueReason::MaxIterationsReached:
            LOG_WARN(QStringLiteral("ReactAgentCore reached max iterations: %1").arg(constants::MAX_ITERATIONS));
            appendRecoveryMessage(QStringLiteral("已达到最大思考步骤，基于当前结果总结回答。"));
            finalizeResponse(m_currentContent.isEmpty() ? m_currentThought : m_currentContent);
            break;
            
        case ContinueReason::ContextOverflow:
            LOG_WARN(QStringLiteral("ReactAgentCore context overflow detected, compressing..."));
            // 自动压缩上下文；若已无法继续压缩到阈值内，则直接结束，避免重复递归压缩。
            {
                const int threshold = contextCompressionThreshold(m_providerConfig);
                const int beforeTokens = estimateContextTokens();
                compressMessagesForContext(m_messages);
                const int afterTokens = estimateContextTokens();
                m_lastContextTokens = afterTokens;

                if (afterTokens >= beforeTokens || afterTokens > threshold) {
                    LOG_WARN(QStringLiteral("ReactAgentCore compression plateaued: before=%1, after=%2, threshold=%3")
                                 .arg(beforeTokens)
                                 .arg(afterTokens)
                                 .arg(threshold));
                    appendRecoveryMessage(QStringLiteral("[上下文过长，已压缩到极限，请清空部分历史后重试。]"));
                    emit conversationUpdated(m_messages);
                    finalizeResponse(m_currentContent.isEmpty()
                        ? QStringLiteral("上下文过长，已压缩到极限，请清空部分历史后重试。")
                        : m_currentContent);
                    break;
                }

                appendRecoveryMessage(QStringLiteral("[系统自动压缩上下文，继续处理...]"));
                emit conversationUpdated(m_messages);
            }
            // 继续下一轮迭代
            processIteration();
            break;
            
        case ContinueReason::ToolExecutionFailure:
            LOG_WARN(QStringLiteral("ReactAgentCore tool failure detected at iteration %1").arg(m_iteration));
            if (canRetryAfterError()) {
                m_hadToolFailure = false;  // 重置标志
                m_retryCount++;
                appendRecoveryMessage(QStringLiteral("工具执行失败，尝试其他方法..."));
                emit conversationUpdated(m_messages);
                // 继续下一轮迭代，让模型尝试其他方案
                processIteration();
            } else {
                LOG_WARN(QStringLiteral("ReactAgentCore max retries reached, finalizing"));
                appendRecoveryMessage(QStringLiteral("多次尝试后仍有错误，基于当前结果总结回答。"));
                finalizeResponse(m_currentContent.isEmpty() ? m_currentThought : m_currentContent);
            }
            break;
            
        case ContinueReason::NoToolStreak:
            LOG_INFO(QStringLiteral("ReactAgentCore no-tool streak termination at iteration %1").arg(m_iteration));
            finalizeResponse(m_currentContent.isEmpty() ? m_currentThought : m_currentContent);
            break;
            
        case ContinueReason::NormalCompletion:
            // 正常完成，继续循环（如有工具调用）或结束（如有最终答案）
            break;
            
        case ContinueReason::StreamError:
            LOG_ERROR(QStringLiteral("ReactAgentCore stream error at iteration %1").arg(m_iteration));
            if (canRetryAfterError()) {
                m_retryCount++;
                appendRecoveryMessage(QStringLiteral("[网络错误，正在重试...]"));
                emit conversationUpdated(m_messages);
                // 重试当前迭代
                processIteration();
            } else {
                LOG_ERROR(QStringLiteral("ReactAgentCore max retries reached after stream error"));
                finalizeResponse(QStringLiteral("抱歉，由于网络问题多次重试失败，请稍后再试。"));
            }
            break;
    }
}

bool ReactAgentCore::canRetryAfterError() const {
    constexpr int MAX_RETRIES = 3;
    return m_retryCount < MAX_RETRIES;
}

void ReactAgentCore::appendRecoveryMessage(const QString& message) {
    Message sysMsg(MessageRole::System, message);
    m_messages.append(sysMsg);
    if (m_memory) {
        m_memory->addMessage(sysMsg);
    }
}

int ReactAgentCore::estimateContextTokens() const {
    int total = 0;
    total += estimateTokens(m_systemPrompt);
    for (const Message& msg : m_messages) {
        total += estimateTokens(msg.content);
        // 工具调用也占用 tokens
        for (const ToolCallData& tc : msg.toolCalls) {
            total += estimateTokens(tc.name);
            total += estimateTokens(tc.arguments);
        }
    }
    return total;
}

// ============================================================================
// 流式工具预执行实现
// ============================================================================

bool ReactAgentCore::isToolCallComplete(const CorePendingToolCall& pending) const {
    if (pending.id.isEmpty() || pending.name.isEmpty()) {
        return false;
    }
    
    // 尝试解析 JSON 参数，如果解析成功则认为完整
    QJsonParseError parseError;
    QJsonDocument::fromJson(pending.arguments.toUtf8(), &parseError);
    return parseError.error == QJsonParseError::NoError;
}

ToolCall ReactAgentCore::createToolCall(const CorePendingToolCall& pending) const {
    ToolCall toolCall;
    toolCall.id = pending.id;
    toolCall.name = pending.name;
    
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(pending.arguments.toUtf8(), &parseError);
    
    if (parseError.error == QJsonParseError::NoError && document.isObject()) {
        toolCall.arguments = document.object();
    } else {
        // 如果解析失败，创建空对象（后续执行时会报错）
        toolCall.arguments = QJsonObject();
    }
    
    return toolCall;
}

void ReactAgentCore::startToolPreExecution() {
    if (!m_executor) {
        return;
    }
    
    // 为所有完整的 pending tool calls 启动异步预执行
    for (const CorePendingToolCall& pending : m_pendingToolCalls) {
        if (!isToolCallComplete(pending)) {
            continue;
        }
        
        // 如果已经在预执行中或已有结果缓存，跳过
        if (m_preExecutingTools.contains(pending.id) || m_preExecutedResults.contains(pending.id)) {
            continue;
        }
        
        ToolCall toolCall = createToolCall(pending);
        
        // 启动异步工具执行（非阻塞）
        QFuture<ToolResult> future = QtConcurrent::run([this, toolCall]() {
            return m_executor->execute(toolCall);
        });
        
        m_preExecutingTools[pending.id] = future;
        emit toolExecutionStarted(pending.id);
        
        LOG_INFO(QStringLiteral("ReactAgentCore: Pre-executing tool '%1' (id: %2)")
            .arg(toolCall.name, toolCall.id));
    }
}

void ReactAgentCore::checkPreExecutionResults() {
    // 仅处理“当前仍待执行”的工具，避免无关预执行任务阻塞当前轮次。
    QSet<QString> relevantToolIds;
    for (const CorePendingToolCall& pending : m_pendingToolCalls) {
        if (isToolCallComplete(pending) && !pending.id.isEmpty()) {
            relevantToolIds.insert(pending.id);
        }
    }

    QStringList completedIds;
    for (auto it = m_preExecutingTools.begin(); it != m_preExecutingTools.end(); ++it) {
        const QString& toolId = it.key();
        QFuture<ToolResult>& future = it.value();
        if (!relevantToolIds.contains(toolId)) {
            // 无关工具不等待，若已完成则顺手清理，防止 map 膨胀。
            if (future.isFinished()) {
                completedIds.append(toolId);
            }
            continue;
        }

        if (future.isFinished()) {
            completedIds.append(toolId);
            m_preExecutedResults[toolId] = future.result();
            emit toolPreExecutionComplete(toolId);
            LOG_INFO(QStringLiteral("ReactAgentCore: Tool pre-execution completed for '%1'").arg(toolId));
        } else {
            // 等待未完成的工具（阻塞当前线程）
            future.waitForFinished();
            completedIds.append(toolId);
            m_preExecutedResults[toolId] = future.result();
            emit toolPreExecutionComplete(toolId);
            LOG_INFO(QStringLiteral("ReactAgentCore: Tool pre-execution waited and completed for '%1'").arg(toolId));
        }
    }
    
    // 清理已完成的预执行记录
    for (const QString& id : completedIds) {
        m_preExecutingTools.remove(id);
    }
}

}
