#include "agent_service.h"
#include "template_loader.h"
#include "agent/react_agent_core.h"
#include "agent/context_builder.h"
#include "application/session_thread.h"
#include "application/agent_orchestrator.h"
#include <QRegularExpression>
#include <QSet>
#include <QDir>
#include <QMetaObject>
#include <QThread>

namespace clawpp {

namespace {

bool isInternalOnlyMessage(const Message& message) {
    return message.metadata.value(QStringLiteral("internal")).toBool()
        || message.metadata.value(QStringLiteral("ui_hidden")).toBool();
}

QString buildVisibleToolSummary(const Message& message) {
    const QString toolName = message.name.trimmed().isEmpty()
        ? QStringLiteral("tool")
        : message.name.trimmed();
    const QString explicitDisplay = message.metadata.value(QStringLiteral("display_content")).toString().trimmed();
    if (!explicitDisplay.isEmpty()) {
        return explicitDisplay;
    }
    if (message.metadata.value(QStringLiteral("output_truncated")).toBool()) {
        return QStringLiteral("工具 %1 已返回结果（输出较长，已截断）").arg(toolName);
    }
    if (message.content.trimmed().isEmpty()) {
        return QStringLiteral("工具 %1 已执行").arg(toolName);
    }
    QString summary = message.content.simplified();
    if (summary.size() > 120) {
        summary = summary.left(120) + QStringLiteral("...");
    }
    if (summary.startsWith('{') || summary.startsWith('[')) {
        return QStringLiteral("工具 %1 已返回结构化结果").arg(toolName);
    }
    return QStringLiteral("工具 %1：%2").arg(toolName, summary);
}

QString detectSavePathFromInput(const QString& input) {
    static const QRegularExpression savePathRe(
        QStringLiteral(R"((?is)(?:保存到|存到|写入|save\s+(?:it\s+)?to|save\s+result\s+to)\s*["“”']?([A-Za-z0-9_./\\-]+\.[A-Za-z0-9_]+)["“”']?)"));
    const QRegularExpressionMatch match = savePathRe.match(input);
    return match.hasMatch() ? match.captured(1).trimmed() : QString();
}

bool containsExplicitUrl(const QString& input) {
    static const QRegularExpression urlRe(
        QStringLiteral(R"((https?://[^\s"'<>]+))"),
        QRegularExpression::CaseInsensitiveOption);
    return urlRe.match(input).hasMatch();
}

bool looksLikeExternalLookupRequest(const QString& input) {
    const QString trimmed = input.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    const QString lower = trimmed.toLower();
    const bool githubIntent =
        lower.contains(QStringLiteral("github"))
        || trimmed.contains(QStringLiteral("GitHub"));
    const bool webIntent =
        lower.contains(QStringLiteral("search"))
        || lower.contains(QStringLiteral("query"))
        || lower.contains(QStringLiteral("lookup"))
        || lower.contains(QStringLiteral("website"))
        || lower.contains(QStringLiteral("web"))
        || trimmed.contains(QStringLiteral("搜索"))
        || trimmed.contains(QStringLiteral("查询"))
        || trimmed.contains(QStringLiteral("网页"))
        || trimmed.contains(QStringLiteral("网站"))
        || trimmed.contains(QStringLiteral("链接"));
    const bool localWorkspaceIntent =
        lower.contains(QStringLiteral("file"))
        || lower.contains(QStringLiteral("folder"))
        || lower.contains(QStringLiteral("directory"))
        || lower.contains(QStringLiteral("workspace"))
        || lower.contains(QStringLiteral("project"))
        || lower.contains(QStringLiteral("code"))
        || trimmed.contains(QStringLiteral("文件"))
        || trimmed.contains(QStringLiteral("目录"))
        || trimmed.contains(QStringLiteral("工作区"))
        || trimmed.contains(QStringLiteral("项目"))
        || trimmed.contains(QStringLiteral("代码"));

    return containsExplicitUrl(trimmed)
        || githubIntent
        || (webIntent && !localWorkspaceIntent);
}

}

AgentService::AgentService(QObject* parent)
    : QObject(parent)
    , m_provider(nullptr)
    , m_memory(nullptr)
    , m_toolRegistry(nullptr)
    , m_toolExecutor(nullptr)
    , m_permissionManager(nullptr)
    , m_templateLoader(nullptr)
    , m_agentCore(new ReactAgentCore())
    , m_sessionThread(new SessionThread(this))
    , m_orchestrator(new AgentOrchestrator(this))
    , m_runGeneration(0)
    , m_lightweightMode(false)
    , m_toolsEnabled(true)
    , m_isRunning(false) {
    // AgentCore 在独立线程运行，避免阻塞 UI 线程。
    m_sessionThread->setWorker(m_agentCore);
    m_sessionThread->start();

    connect(m_agentCore, &ReactAgentCore::responseChunk, this, &AgentService::responseChunk);
    connect(m_agentCore, &ReactAgentCore::toolCallRequested, this, &AgentService::toolCallRequested);
    connect(m_agentCore, &ReactAgentCore::usageReport, this, &AgentService::usageReport);
    connect(m_agentCore, &ReactAgentCore::toolCallParsed, this, &AgentService::toolCallParsed);
    connect(m_agentCore, &ReactAgentCore::toolExecutionStarted, this, &AgentService::toolPreExecutionStarted);
    connect(m_agentCore, &ReactAgentCore::toolPreExecutionComplete, this, &AgentService::toolPreExecutionCompleted);
    connect(m_agentCore, &ReactAgentCore::completed, this, [this](const QString& fullResponse) {
        m_isRunning = false;
        ++m_runGeneration;
        m_currentResponse = fullResponse;
        emit responseComplete(fullResponse);
    });
    connect(m_agentCore, &ReactAgentCore::errorOccurred, this, [this](const QString& error) {
        m_isRunning = false;
        ++m_runGeneration;
        emit errorOccurred(error);
    });
    connect(m_agentCore, &ReactAgentCore::conversationUpdated, this, [this](const MessageList& messages) {
        {
            QMutexLocker locker(&m_mutex);
            m_messageHistory = messages;
        }
        emit conversationUpdatedRaw(messages);
        emit conversationChanged(sanitizeConversationForDisplay(messages));
    });

    connect(&ContextBuilder::instance(), &ContextBuilder::skillsChanged, this, [this]() {
        if (!m_currentSessionId.isEmpty()) {
            m_systemPrompt = ContextBuilder::instance().buildSystemPrompt();
            syncCoreState();
        }
    });

    m_orchestrator->setAgentService(this);
}

AgentService::~AgentService() {
    stopGeneration();

    if (m_agentCore
        && m_agentCore->thread()
        && m_agentCore->thread() != QThread::currentThread()
        && m_agentCore->thread()->isRunning()) {
        QThread* ownerThread = QThread::currentThread();
        QMetaObject::invokeMethod(m_agentCore, [this, ownerThread]() {
            m_agentCore->moveToThread(ownerThread);
        }, Qt::BlockingQueuedConnection);
    }

    if (m_sessionThread) {
        m_sessionThread->stop();
    }
    if (m_agentCore) {
        delete m_agentCore;
        m_agentCore = nullptr;
    }
}

void AgentService::setProvider(ILLMProvider* provider) {
    m_provider = provider;
    syncCoreState();
}

void AgentService::setProviderConfig(const ProviderConfig& config) {
    m_providerConfig = config;
    m_providerConfig.timeoutMs = qMax(m_providerConfig.timeoutMs, 600000);
    syncCoreState();
}

void AgentService::setMemory(IMemorySystem* memory) {
    m_memory = memory;
    syncCoreState();
}

void AgentService::setToolRegistry(ToolRegistry* registry) {
    m_toolRegistry = registry;
    if (!m_toolExecutor) {
        m_toolExecutor = new ToolExecutor(registry, m_permissionManager, this);
    } else {
        m_toolExecutor->setRegistry(registry);
    }
    connectToolExecutorSignals();
    syncCoreState();
}

void AgentService::setPermissionManager(PermissionManager* manager) {
    m_permissionManager = manager;
    if (m_toolExecutor) {
        m_toolExecutor->setPermissionManager(manager);
    }
    connectToolExecutorSignals();
    syncCoreState();
}

void AgentService::setTemplateLoader(TemplateLoader* loader) {
    m_templateLoader = loader;
}

void AgentService::setWorkspaceRoot(const QString& workspaceRoot) {
    m_workspaceRoot = workspaceRoot.trimmed();
}

void AgentService::buildSystemPromptFromTemplates() {
    const QString workspace = m_workspaceRoot.isEmpty() ? QDir::currentPath() : m_workspaceRoot;
    // 每次会话切换都重新初始化上下文，确保模板/技能变更可以生效。
    ContextBuilder::instance().initialize(workspace);
    const QString prompt = ContextBuilder::instance().buildSystemPrompt();
    {
        QMutexLocker locker(&m_mutex);
        m_systemPrompt = prompt;
    }
    syncCoreState();
}

void AgentService::startSession(const QString& sessionId, const QString& name) {
    const QString normalizedName = name.isEmpty() ? sessionId : name;
    {
        QMutexLocker locker(&m_mutex);
        m_currentSessionId = sessionId;
        m_sessionName = normalizedName;
        m_messageHistory.clear();
        m_currentResponse.clear();
    }
    m_isRunning = false;

    if (m_memory) {
        m_memory->setSessionContext(sessionId, normalizedName);
    }

    if (m_toolExecutor) {
        m_toolExecutor->setSessionId(sessionId);
    }
    
    buildSystemPromptFromTemplates();
    syncCoreState();
    
    emit sessionStarted(sessionId);
}

void AgentService::endSession() {
    stopGeneration();
    {
        QMutexLocker locker(&m_mutex);
        m_currentSessionId.clear();
        m_sessionName.clear();
        m_messageHistory.clear();
        m_currentResponse.clear();
    }

    if (m_memory) {
        m_memory->setSessionContext(QString(), QString());
    }

    if (m_toolExecutor) {
        m_toolExecutor->setSessionId(QString());
    }

    syncCoreState();
    
    emit sessionEnded();
}

void AgentService::sendMessage(const QString& content) {
    if (m_isRunning || !m_agentCore) {
        emit errorOccurred("上一条回复仍在生成，请先停止或等待完成");
        return;
    }

    emit messageSent(content);

    const QString workspace = m_workspaceRoot.isEmpty() ? QDir::currentPath() : m_workspaceRoot;
    ContextBuilder::instance().initialize(workspace);
    Skill matchedSkill = ContextBuilder::instance().matchSkillForInput(content);//skill匹配机制增强
    if (!matchedSkill.isValid()) {
        const QString normalizedInput = content.toLower();
        int bestScore = 0;
        Skill bestSkill;
        const QList<Skill> candidates = ContextBuilder::instance().skills();
        for (const Skill& candidate : candidates) {
            if (!candidate.meta.available) {
                continue;
            }
            int score = 0;
            const QString candidateName = candidate.name.trimmed().toLower();
            if (!candidateName.isEmpty() && normalizedInput.contains(candidateName)) {
                score += 3;
            }
            const QString candidateDescription = candidate.description.trimmed().toLower();
            if (candidateDescription.size() >= 4 && normalizedInput.contains(candidateDescription.left(12))) {
                score += 1;
            }
            for (const QString& trigger : candidate.triggers) {
                QString t = trigger.trimmed().toLower();
                if (t.isEmpty()) {
                    continue;
                }
                if (normalizedInput.contains(t)) {
                    score += 3;
                    continue;
                }
                if (t.startsWith('/')) {
                    t.remove(0, 1);
                }
                if (t.size() >= 2 && normalizedInput.contains(t)) {
                    score += 2;
                }
            }
            if (score > bestScore) {
                bestScore = score;
                bestSkill = candidate;
            }
        }
        if (bestScore >= 3 && bestSkill.isValid()) {
            matchedSkill = bestSkill;
        }
    }
    const bool toolFirstExternalMode = shouldUseToolFirstExternalMode(content);
    const bool multiStepMode = shouldUseMultiStepMode(content, matchedSkill) || toolFirstExternalMode;
    m_lightweightMode = !multiStepMode;
    m_toolsEnabled = multiStepMode;
    m_systemPrompt = multiStepMode
        ? ContextBuilder::instance().buildSystemPromptForInput(content)
        : ContextBuilder::instance().buildFastSystemPromptForInput(content);
    if (toolFirstExternalMode) {
        m_systemPrompt += buildToolFirstExternalPrompt(content);
    }
    m_runtimeToolNames = inferredRuntimeToolsForInput(content, matchedSkill);

    syncCoreState();
    m_isRunning = true;
    const quint64 runGeneration = ++m_runGeneration;
    QString runtimeContext = ContextBuilder::instance().buildRuntimeContext(QStringLiteral("gui"), m_currentSessionId);
    QString payload = content;
    if (m_agentCore) {
        QMetaObject::invokeMethod(m_agentCore, [this, payload, runtimeContext]() {
            m_agentCore->run(payload, runtimeContext);
        }, Qt::QueuedConnection);
    }

    const int watchdogMs = qMax(600000, m_providerConfig.timeoutMs + 60000);
    QTimer::singleShot(watchdogMs, this, [this, runGeneration]() {
        if (!m_isRunning || runGeneration != m_runGeneration) {
            return;
        }

        // 双保险：provider 侧超时未触发时，服务层 watchdog 主动中断。
        if (m_provider) {
            m_provider->abort();
        }
        m_isRunning = false;
        ++m_runGeneration;
        emit errorOccurred(QStringLiteral("请求超时，请检查网络或模型配置后重试"));
    });
}

bool AgentService::shouldUseToolFirstExternalMode(const QString& content) const {
    return looksLikeExternalLookupRequest(content);
}

QString AgentService::buildToolFirstExternalPrompt(const QString& content) const {
    QStringList lines;
    lines.append(QStringLiteral("\n\n---\n\n# Tool-First External Lookup Mode"));
    lines.append(QStringLiteral("- This request likely needs external facts or a web fetch."));
    lines.append(QStringLiteral("- Do not narrate a plan. Call the minimum required tool immediately."));
    lines.append(QStringLiteral("- If the exact URL/endpoint/target is unknown, you MUST use `network` with `operation=web_search` first."));
    lines.append(QStringLiteral("- Do not guess a URL, repo path, API endpoint, or website identifier from memory when the target is ambiguous."));
    lines.append(QStringLiteral("- Use `http_get` or `web_fetch` only after you already know the concrete target URL from the user or from search results."));
    const QString savePath = detectSavePathFromInput(content);
    if (!savePath.isEmpty()) {
        lines.append(QStringLiteral("- The user asked to save the result to `%1`. After obtaining the final content, call `write_file` with that exact path before your final answer.").arg(savePath));
    }
    if (containsExplicitUrl(content)) {
        lines.append(QStringLiteral("- A URL is present in the request. Fetch that URL directly instead of searching elsewhere first."));
    } else {
        lines.append(QStringLiteral("- If a fetch returns 404/Not Found or equivalent, do not retry the same URL. Return to search results or refine the query."));
    }
    lines.append(QStringLiteral("- If one tool result is not enough, continue with the next necessary tool. Do not stop after an interim sentence."));
    lines.append(QStringLiteral("- After all required tools succeed, give a concise final answer that reflects the actual tool results."));
    lines.append(QStringLiteral("- If the conversation becomes too long or you want to preserve only the key working state, you may call `compact` with an optional `focus` string."));
    return lines.join(QStringLiteral("\n"));
}

QStringList AgentService::inferredRuntimeToolsForInput(const QString& content, const Skill& matchedSkill) const {
    QStringList toolNames;
    auto appendTool = [&toolNames](const QString& toolName) {
        const QString normalized = toolName.trimmed();
        if (!normalized.isEmpty() && !toolNames.contains(normalized)) {
            toolNames.append(normalized);
        }
    };

    for (const QJsonValue& value : matchedSkill.tools) {
        appendTool(value.toString());
    }

    appendTool(QStringLiteral("compact"));

    if (shouldUseToolFirstExternalMode(content)) {
        appendTool(QStringLiteral("network"));
        appendTool(QStringLiteral("write_file"));
        appendTool(QStringLiteral("read_file"));
    }

    return toolNames;
}

void AgentService::stopGeneration() {
    ++m_runGeneration;

    if (m_agentCore) {
        QMetaObject::invokeMethod(m_agentCore, [this]() {
            m_agentCore->stop();
        }, Qt::QueuedConnection);
    }

    if (m_provider) {
        m_provider->abort();
    }

    if (m_isRunning) {
        emit generationStopped();
    }

    m_isRunning = false;
}

bool AgentService::shouldUseMultiStepMode(const QString& content, const Skill& matchedSkill) const {
    if (matchedSkill.isValid()) {
        return true;
    }

    const QString trimmed = content.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    if (trimmed.size() >= 120 || (trimmed.contains('\n') && trimmed.size() >= 80)) {
        return true;
    }

    static const QRegularExpression complexTaskRegex(
        QStringLiteral(
            "(?i)(```|\\b(cmd|powershell|bash|shell|terminal|g\\+\\+|cmake|git|python|npm|node)\\b|"
            "\\b(create|write|edit|modify|update|fix|debug|compile|build|run|execute|install|search|list|open|read|save|generate)\\b|"
            "\\b(file|folder|directory|path|workspace|desktop|project|code|source)\\b|"
            "创建|编写|修改|更新|修复|调试|编译|构建|运行|执行|安装|搜索|列出|读取|打开|生成|文件|目录|路径|桌面|工作区|项目|代码|程序)"));
    return complexTaskRegex.match(trimmed).hasMatch();
}

void AgentService::setSystemPrompt(const QString& prompt) {
    m_systemPrompt = prompt;
    if (m_agentCore) {
        m_agentCore->setSystemPrompt(prompt);
    }
}

void AgentService::setHistory(const MessageList& history) {
    {
        QMutexLocker locker(&m_mutex);
        m_messageHistory = history;
    }
    syncCoreState();
}

QString AgentService::currentSessionId() const {
    QMutexLocker locker(&m_mutex);
    return m_currentSessionId;
}

MessageList AgentService::messageHistory() const {
    QMutexLocker locker(&m_mutex);
    return m_messageHistory;
}

bool AgentService::isRunning() const {
    return m_isRunning;
}

MessageList AgentService::displayConversation(const MessageList& messages) const {
    return sanitizeConversationForDisplay(messages);
}

ToolExecutor* AgentService::toolExecutor() const {
    return m_toolExecutor;
}

AgentOrchestrator* AgentService::orchestrator() const {
    return m_orchestrator;
}

MessageList AgentService::sanitizeConversationForDisplay(const MessageList& messages) const {
    MessageList sanitized;
    sanitized.reserve(messages.size());
    for (const Message& message : messages) {
        if (isInternalOnlyMessage(message)) {
            continue;
        }
        if (message.role == MessageRole::Tool) {
            continue;
        }
        if (message.role == MessageRole::Assistant && !message.toolCalls.isEmpty()) {
            continue;
        }
        sanitized.append(sanitizeMessageForDisplay(message));
    }
    return sanitized;
}

Message AgentService::sanitizeMessageForDisplay(const Message& message) const {
    Message out = message;
    out.reasoningContent.clear();

    if (out.role == MessageRole::Tool) {
        out.metadata[QStringLiteral("display_content")] = buildVisibleToolSummary(out);
        out.metadata.remove(QStringLiteral("output_cache_path"));
        out.metadata.remove(QStringLiteral("output_original_size"));
        out.metadata.remove(QStringLiteral("query"));
        out.metadata.remove(QStringLiteral("search_url"));
        out.metadata.remove(QStringLiteral("url"));
        out.metadata.remove(QStringLiteral("fallback_url"));
    }

    if (out.role == MessageRole::System && out.metadata.value(QStringLiteral("internal")).toBool()) {
        out.metadata[QStringLiteral("ui_hidden")] = true;
    }

    out.metadata.remove(QStringLiteral("runtime_context"));
    out.metadata.remove(QStringLiteral("internal"));
    return out;
}

void AgentService::syncCoreState() {
    if (!m_agentCore) {
        return;
    }

    MessageList historySnapshot;
    QString systemPromptSnapshot;
    QStringList runtimeToolNamesSnapshot;
    ProviderConfig providerConfigSnapshot;
    QString currentSessionIdSnapshot;
    QString runtimeContextSnapshot;
    bool lightweightModeSnapshot = false;
    bool toolsEnabledSnapshot = true;
    {
        QMutexLocker locker(&m_mutex);
        historySnapshot = m_messageHistory;
        systemPromptSnapshot = m_systemPrompt;
        runtimeToolNamesSnapshot = m_runtimeToolNames;
        providerConfigSnapshot = m_providerConfig;
        currentSessionIdSnapshot = m_currentSessionId;
        runtimeContextSnapshot = ContextBuilder::instance().buildRuntimeContext(QStringLiteral("gui"), m_currentSessionId);
        lightweightModeSnapshot = m_lightweightMode;
        toolsEnabledSnapshot = m_toolsEnabled;
    }

    ILLMProvider* providerSnapshot = m_provider;
    IMemorySystem* memorySnapshot = m_memory;
    ToolExecutor* executorSnapshot = m_toolExecutor;

    QMetaObject::invokeMethod(m_agentCore, [this,
                                            providerSnapshot,
                                            memorySnapshot,
                                            executorSnapshot,
                                            providerConfigSnapshot,
                                            currentSessionIdSnapshot,
                                            runtimeContextSnapshot,
                                            historySnapshot,
                                            systemPromptSnapshot,
                                            runtimeToolNamesSnapshot,
                                            lightweightModeSnapshot,
                                            toolsEnabledSnapshot]() {
        if (!m_agentCore) {
            return;
        }

        m_agentCore->setProvider(providerSnapshot);
        m_agentCore->setMemory(memorySnapshot);
        m_agentCore->setExecutor(executorSnapshot);
        m_agentCore->setProviderConfig(providerConfigSnapshot);
        m_agentCore->setSessionId(currentSessionIdSnapshot);
        m_agentCore->setRuntimeContext(runtimeContextSnapshot);
        m_agentCore->setContext(historySnapshot);
        m_agentCore->setSystemPrompt(systemPromptSnapshot);
        m_agentCore->setRuntimeToolNames(runtimeToolNamesSnapshot);
        m_agentCore->setLightweightMode(lightweightModeSnapshot);
        m_agentCore->setToolsEnabled(toolsEnabledSnapshot);
    }, Qt::QueuedConnection);

    if (m_toolExecutor) {
        m_toolExecutor->setProvider(providerSnapshot);
        m_toolExecutor->setProviderConfig(providerConfigSnapshot);
        m_toolExecutor->setSessionId(currentSessionIdSnapshot);
    }
}

void AgentService::connectToolExecutorSignals() {
    if (!m_toolExecutor) {
        return;
    }

    connect(m_toolExecutor, &ToolExecutor::toolCompleted,
            this, &AgentService::toolCallCompleted, Qt::UniqueConnection);
}

}
