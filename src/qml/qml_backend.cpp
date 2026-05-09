#include "qml_backend.h"

#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QMetaObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSet>
#include <QUrl>
#include <QTextStream>
#include <QInputDialog>
#include <QTimer>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>
#include <QUuid>
#include <QFileInfo>
#include <utility>
#include <functional>
#include <memory>

#include "common/constants.h"
#include "common/model_context_limits.h"
#include "permission/permission_manager.h"
#include "agent/context_builder.h"
#include "tool/tool_registry.h"
#include "tool/tools/read_file_tool.h"
#include "tool/tools/write_file_tool.h"
#include "tool/tools/list_directory_tool.h"
#include "tool/tools/edit_file_tool.h"
#include "tool/tools/search_files_tool.h"
#include "tool/tools/shell_tool.h"
#include "tool/tools/network_tool.h"
#include "tool/tools/system_tool.h"
#include "tool/tools/subagent_tool.h"
#include "infrastructure/database/database_manager.h"
#include "provider/openai_provider.h"
#include "provider/anthropic_provider.h"
#include "provider/gemini_provider.h"
#include "application/agent_orchestrator.h"

namespace clawpp {

namespace {
QString sessionStatusText(const Session& session) {
    switch (session.status) {
        case Session::Active: return QStringLiteral("active");
        case Session::Paused: return QStringLiteral("paused");
        case Session::Archived: return QStringLiteral("archived");
    }
    return QStringLiteral("active");
}

bool isWorkspaceRoot(const QString& path) {
    const QDir dir(path);
    return dir.exists(QStringLiteral("src"))
        && dir.exists(QStringLiteral("qml"))
        && dir.exists(QStringLiteral("templates"));
}

QString detectWorkspaceRoot() {
    QStringList seeds;
    seeds.append(QDir::currentPath());

    const QString appDir = QCoreApplication::applicationDirPath();
    if (!appDir.isEmpty() && !seeds.contains(appDir)) {
        seeds.append(appDir);
    }

    for (const QString& seed : seeds) {
        QDir dir(seed);
        for (int depth = 0; depth < 8; ++depth) {
            const QString candidate = dir.absolutePath();
            if (isWorkspaceRoot(candidate)) {
                return QDir(candidate).absolutePath();
            }

            if (!dir.cdUp()) {
                break;
            }
        }
    }

    return QDir::currentPath();
}

QString backendWorkspaceRoot() {
    static const QString root = detectWorkspaceRoot();
    return root;
}

QString backendKernelRoot() {
    static const QString kernel = detectWorkspaceRoot();
    return kernel;
}

void appendBackendTrace(const QString& message) {
    const QString logPath = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("startup_trace.log"));
    QFile file(logPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return;
    }

    QTextStream out(&file);
    out << QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))
        << " | " << message << "\n";
    out.flush();
}

QDateTime parseTokenUsageTimestamp(const QString& text) {
    QDateTime dt = QDateTime::fromString(text, Qt::ISODate);
    if (!dt.isValid()) {
        dt = QDateTime::fromString(text, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    }
    if (!dt.isValid()) {
        dt = QDateTime::currentDateTime();
    }
    return dt;
}

QVariantMap normalizeTokenUsageEntry(const QVariantMap& rawEntry) {
    QVariantMap entry;

    const QDateTime timestamp = parseTokenUsageTimestamp(rawEntry.value(QStringLiteral("timestamp")).toString());
    const int prompt = qMax(0, rawEntry.value(QStringLiteral("prompt")).toInt());
    const int completion = qMax(0, rawEntry.value(QStringLiteral("completion")).toInt());
    int total = rawEntry.value(QStringLiteral("total")).toInt();
    if (total <= 0) {
        total = prompt + completion;
    }
    total = qMax(0, total);

    entry.insert(QStringLiteral("timestamp"), timestamp.toString(Qt::ISODate));
    entry.insert(QStringLiteral("prompt"), prompt);
    entry.insert(QStringLiteral("completion"), completion);
    entry.insert(QStringLiteral("total"), total);
    return entry;
}

void appendUsageToAggregate(QVariantMap* aggregate, int prompt, int completion, int total) {
    if (!aggregate) {
        return;
    }

    aggregate->insert(QStringLiteral("prompt"), aggregate->value(QStringLiteral("prompt")).toInt() + prompt);
    aggregate->insert(QStringLiteral("completion"), aggregate->value(QStringLiteral("completion")).toInt() + completion);
    aggregate->insert(QStringLiteral("total"), aggregate->value(QStringLiteral("total")).toInt() + total);
}

int modelCompressionThreshold(const ProviderConfig& config) {
    const int contextLimit = modelctx::inferModelContextLimit(config);
    return qMax(1, static_cast<int>(contextLimit * 0.6));
}

QString workflowTemplateTitle(const QString& templateId) {
    if (templateId == QStringLiteral("coding")) {
        return QStringLiteral("代理编码助手");
    }
    return QStringLiteral("代理研究助手");
}

QString workflowInstructionPrompt(const QString& templateId, const QString& task, bool parallelMode) {
    const QString modeText = parallelMode
        ? QStringLiteral("parallel")
        : QStringLiteral("sequential");
    const QString domainHints = templateId == QStringLiteral("coding")
        ? QStringLiteral("侧重实现、验证、回归风险。")
        : QStringLiteral("侧重检索、交叉验证、结论可追溯。");
    return QStringLiteral(
R"(你是主Agent，执行“简化工作流”（简单性优先）。
Mode: %1
Template: %2

规则：
1) 若任务可一次完成，直接单次回答，不启用复杂循环。
2) 若任务复杂，按节点执行：planner -> tool_use -> memory -> reflecting。
3) planner：拆分2-5个可执行步骤并选工具。
4) tool_use：仅调用必要工具；失败时最多重试1次。
5) memory：汇总本轮关键信息（短期记忆）并引用已有上下文（长期记忆）。
6) reflecting：检查结果并给出下一步；避免空转。
7) 不要复述本提示词，不要输出多余解释。

限制：
- Max_Iterations: 4
- Timeout: 240s

输出仅保留三段：
【Plan】
【Node Logs】
【Final】

领域提示：%3
任务：%4)")
        .arg(modeText, workflowTemplateTitle(templateId), domainHints, task.trimmed());
}

QString workflowParallelPlanCard(const QString& templateId,
                                 const QVector<bool>& completedFlags,
                                 int completedCount) {
    const QString planA = templateId == QStringLiteral("coding")
        ? QStringLiteral("分支A：实现方案与关键改动")
        : QStringLiteral("分支A：信息收集与核心事实");
    const QString planB = templateId == QStringLiteral("coding")
        ? QStringLiteral("分支B：测试验证与回归风险")
        : QStringLiteral("分支B：交叉验证与反例风险");
    const QString markA = completedFlags.size() > 0 && completedFlags[0] ? QStringLiteral("[x]") : QStringLiteral("[ ]");
    const QString markB = completedFlags.size() > 1 && completedFlags[1] ? QStringLiteral("[x]") : QStringLiteral("[ ]");
    return QStringLiteral(
        "ToolCard|Action|workflow_parallel|RUNNING\n"
        "模板：%1\n"
        "模式：子代理并行\n"
        "计划：\n"
        "%2 %3\n"
        "%4 %5\n"
        "进度：%6/2 已完成")
        .arg(workflowTemplateTitle(templateId),
             markA, planA,
             markB, planB,
             QString::number(qBound(0, completedCount, 2)));
}

QPair<QString, QString> workflowParallelSubtasks(const QString& templateId, const QString& task) {
    if (templateId == QStringLiteral("coding")) {
        return qMakePair(
            QStringLiteral("planner+tool_use（实现分支）：输出实现步骤、关键改动与执行日志。\n任务：%1").arg(task),
            QStringLiteral("memory+reflecting（验证分支）：输出验证策略、风险结论与修正建议。\n任务：%1").arg(task));
    }
    return qMakePair(
        QStringLiteral("planner+tool_use（检索分支）：收集核心事实与来源。\n任务：%1").arg(task),
        QStringLiteral("memory+reflecting（审校分支）：交叉验证并输出不确定性与结论。\n任务：%1").arg(task));
}

}

void QmlBackend::registerTools() {
    auto& registry = ToolRegistry::instance();

    registry.registerTool(new ReadFileTool());
    registry.registerTool(new WriteFileTool());
    registry.registerTool(new ListDirectoryTool());
    registry.registerTool(new EditFileTool());
    registry.registerTool(new SearchFilesTool());
    registry.registerTool(new ShellTool());
    registry.registerTool(new NetworkTool());
    registry.registerTool(new SystemTool());
    registry.registerTool(new SubagentTool());
}

QmlBackend::QmlBackend(QObject* parent)
    : QObject(parent)
    , m_sessionsModel(new SessionListModel(this))
    , m_messagesModel(new MessageListModel(this))
    , m_sessionManager(new SessionManager(this))
    , m_agentService(new AgentService(this))
    , m_providerManager(new ProviderManager(this))
    , m_memoryManager(new MemoryManager(backendWorkspaceRoot(), this))
    , m_externalPlatformManager(new ExternalPlatformManager(this))
    , m_templateLoader(new TemplateLoader(backendWorkspaceRoot(), backendKernelRoot(), this))
    , m_workspaceRoot(backendWorkspaceRoot())
    , m_kernelRoot(backendKernelRoot())
    , m_generating(false)
    , m_promptTokens(0)
    , m_completionTokens(0)
    , m_totalTokens(0)
    , m_lastApiContextTokens(0)
    , m_accPromptTokens(0)
    , m_accCompletionTokens(0)
    , m_accTotalTokens(0)
    , m_usageDirtyCurrentRun(false)
    , m_streamingAssistantRow(-1)
    , m_chunkFlushTimer(nullptr) {
    if (m_workspaceRoot.isEmpty()) {
        m_workspaceRoot = QDir::currentPath();
    }
    if (m_kernelRoot.isEmpty()) {
        m_kernelRoot = QDir(QCoreApplication::applicationDirPath()).absolutePath();
    }

    QString workspaceOverride = qEnvironmentVariable("CLAW_WORKSPACE_ROOT");
    if (!workspaceOverride.isEmpty()) {
        m_workspaceRoot = QDir(workspaceOverride).absolutePath();
    } else {
        m_workspaceRoot = QDir(m_kernelRoot).filePath(QStringLiteral("workspace"));
    }
    QDir().mkpath(m_workspaceRoot);
    QDir().mkpath(QDir(m_workspaceRoot).filePath(QStringLiteral("templates")));
    QDir().mkpath(QDir(m_workspaceRoot).filePath(QStringLiteral("memory")));
    QDir().mkpath(QDir(m_workspaceRoot).filePath(QStringLiteral("skills")));

    const QStringList bootstrapDocs = {QStringLiteral("SOUL.md"), QStringLiteral("AGENTS.md"), QStringLiteral("USER.md"), QStringLiteral("TOOLS.md"), QStringLiteral("IDENTITY.md"), QStringLiteral("HEARTBEAT.md")};
    for (const QString& name : bootstrapDocs) {
        const QString workspaceFile = QDir(m_workspaceRoot).filePath(QStringLiteral("templates/%1").arg(name));
        if (QFileInfo::exists(workspaceFile)) {
            continue;
        }
        const QString kernelFile = QDir(m_kernelRoot).filePath(QStringLiteral("templates/%1").arg(name));
        QFile source(kernelFile);
        if (!source.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }
        QFile target(workspaceFile);
        if (target.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            target.write(source.readAll());
            target.close();
        }
        source.close();
    }

    appendBackendTrace(QStringLiteral("QmlBackend ctor start"));
    m_memoryManager->setWorkspace(m_workspaceRoot);
    m_templateLoader->setWorkspace(m_workspaceRoot);
    m_templateLoader->setKernelRoot(m_kernelRoot);
    loadTokenUsageHistory();
    m_chunkFlushTimer = new QTimer(this);
    m_chunkFlushTimer->setSingleShot(true);
    m_chunkFlushTimer->setInterval(14);
    connect(m_chunkFlushTimer, &QTimer::timeout, this, [this]() {
        flushPendingAssistantChunk(false);
    });

    appendBackendTrace(QStringLiteral("QmlBackend before initializeCore"));
    initializeCore();
    appendBackendTrace(QStringLiteral("QmlBackend after initializeCore"));
    setupConnections();
    appendBackendTrace(QStringLiteral("QmlBackend after setupConnections"));
    refreshSessions();
    appendBackendTrace(QStringLiteral("QmlBackend after refreshSessions"));
    ensureSessionExists();
    appendBackendTrace(QStringLiteral("QmlBackend after ensureSessionExists"));
    syncCurrentSession();
    appendBackendTrace(QStringLiteral("QmlBackend ctor completed"));
}

QmlBackend::~QmlBackend() = default;

void QmlBackend::initializeCore() {
    appendBackendTrace(QStringLiteral("initializeCore: start"));
    ConfigManager::instance().load();
    appendBackendTrace(QStringLiteral("initializeCore: config loaded"));

    if (!DatabaseManager::instance().initialize()) {
        m_statusText = QStringLiteral("数据库初始化失败");
    }
    appendBackendTrace(QStringLiteral("initializeCore: database initialized"));

    Logger::instance().setLevel(LogLevel::Info);
    Logger::instance().setFileEnabled(true);
    Logger::instance().info(QStringLiteral("Starting %1 v%2")
        .arg(constants::APP_NAME)
        .arg(constants::APP_VERSION));

    registerTools();
    appendBackendTrace(QStringLiteral("initializeCore: tools registered"));

    PermissionConfig permConfig;
    permConfig.defaultAction = PermissionRule::Action::Allow;
    PermissionManager::instance().initialize(permConfig);
    appendBackendTrace(QStringLiteral("initializeCore: permission manager initialized"));

    // 轻量启动优化：ContextBuilder 预热延后到事件循环空闲时，避免阻塞首屏。
    QTimer::singleShot(0, this, [this]() {
        ContextBuilder::instance().initialize(m_workspaceRoot);
        appendBackendTrace(QStringLiteral("initializeCore: context builder warmup completed"));
    });
    appendBackendTrace(QStringLiteral("initializeCore: context builder warmup scheduled"));

    auto& config = ConfigManager::instance();
    auto providerConfig = config.getProviderConfig();
    providerConfig.timeoutMs = qMax(providerConfig.timeoutMs, 600000);

    m_providerManager->setConfig(providerConfig);
    m_agentService->setProvider(m_providerManager);
    m_agentService->setProviderConfig(providerConfig);
    syncCompressionThresholdFromModel();
    m_agentService->setMemory(m_memoryManager);
    m_agentService->setToolRegistry(&ToolRegistry::instance());
    m_agentService->setPermissionManager(&PermissionManager::instance());
    m_agentService->setTemplateLoader(m_templateLoader);
    m_agentService->setWorkspaceRoot(m_workspaceRoot);
    appendBackendTrace(QStringLiteral("initializeCore: agent service configured"));

    m_sessionManager->setAgentService(m_agentService);
    m_sessionManager->setMemorySystem(m_memoryManager);
    appendBackendTrace(QStringLiteral("initializeCore: session manager configured"));

    const QString tgToken = config.getTelegramToken();
    const QString fsAppId = config.getFeishuAppId();
    const QString fsAppSecret = config.getFeishuAppSecret();
    const QString fsVerificationToken = config.getFeishuVerificationToken();
    const int fsPort = config.getFeishuPort();
    // 轻量启动优化：外部平台启动延后，不阻塞主界面初始化。
    QTimer::singleShot(0, this, [this, tgToken, fsAppId, fsAppSecret, fsVerificationToken, fsPort]() {
        if (!tgToken.isEmpty()) {
            m_externalPlatformManager->startTelegramBot(tgToken);
        }
        appendBackendTrace(QStringLiteral("initializeCore: telegram setup done (deferred)"));

        if (!fsAppId.isEmpty() && !fsAppSecret.isEmpty() && !fsVerificationToken.isEmpty()) {
            m_externalPlatformManager->startFeishuBot(fsAppId, fsAppSecret, fsVerificationToken, fsPort);
        }
        appendBackendTrace(QStringLiteral("initializeCore: feishu setup done (deferred)"));
    });
    appendBackendTrace(QStringLiteral("initializeCore: external platform startup scheduled"));
}

void QmlBackend::setupConnections() {
    connect(m_sessionManager, &SessionManager::sessionCreated, this, [this]() {
        refreshSessions();
    });

    connect(m_sessionManager, &SessionManager::sessionDeleted, this, [this](const QString& sessionId) {
        m_externalConversationContexts.remove(sessionId);
        refreshSessions();
        syncCurrentSession();
    });

    connect(m_sessionManager, &SessionManager::sessionRenamed, this, [this](const QString&, const QString&) {
        refreshSessions();
        syncCurrentSession();
    });

    connect(m_sessionManager, &SessionManager::sessionArchived, this, [this](const QString&) {
        refreshSessions();
        syncCurrentSession();
    });

    connect(m_sessionManager, &SessionManager::sessionSwitched, this, [this](const QString&) {
        syncCurrentSession();
    });

    connect(m_agentService, &AgentService::messageSent, this, [this](const QString& content) {
        // QML path adds user/assistant placeholder before calling AgentService.
        // Keep this as a fallback for non-QML callers.
        if (!m_generating || m_streamingAssistantRow < 0) {
            if (m_messagesModel->rowCount() == 0) {
                m_streamingAssistantRow = -1;
            }

            m_messagesModel->addMessage(static_cast<int>(MessageRole::User), content);
            m_streamingAssistantRow = m_messagesModel->addMessage(static_cast<int>(MessageRole::Assistant), QString());
            setGenerating(true);
            m_statusText = QStringLiteral("生成中");
            emit statusTextChanged();
        }
    });

    connect(m_agentService, &AgentService::responseChunk, this, [this](const StreamChunk& chunk) {
        if (m_streamingAssistantRow >= 0 && !chunk.content.isEmpty()) {
            m_pendingAssistantChunk += chunk.content;
            if (m_pendingAssistantChunk.size() >= 72) {
                flushPendingAssistantChunk(true);
            } else if (m_chunkFlushTimer && !m_chunkFlushTimer->isActive()) {
                m_chunkFlushTimer->start();
            }
        }
    });

    connect(m_agentService, &AgentService::toolCallRequested, this, [this](const ToolCall& call) {
        m_statusText = QStringLiteral("执行工具：%1").arg(call.name);
        emit statusTextChanged();
        setToolHintText(QStringLiteral("🔧 %1").arg(call.name));
    });

    connect(m_agentService, &AgentService::toolCallParsed, this, [this](const ToolCall& call) {
        Q_UNUSED(call);
    });

    connect(m_agentService, &AgentService::toolPreExecutionStarted, this, [this](const QString& toolId) {
        Q_UNUSED(toolId);
    });

    connect(m_agentService, &AgentService::toolPreExecutionCompleted, this, [this](const QString& toolId) {
        Q_UNUSED(toolId);
    });

    connect(m_agentService, &AgentService::toolCallCompleted, this, [this](const ToolCall& call, const ToolResult& result) {
        const QString toolName = call.name;
        const bool truncated = result.metadata.value(QStringLiteral("output_truncated")).toBool();
        const QString cachePath = result.metadata.value(QStringLiteral("output_cache_path")).toString();
        const int riskLevel = result.metadata.value(QStringLiteral("risk_level")).toInt(-1);
        const QString riskSummary = result.metadata.value(QStringLiteral("risk_summary")).toString();
        m_statusText = result.success
            ? QStringLiteral("工具已完成：%1").arg(toolName)
            : QStringLiteral("工具执行失败：%1").arg(toolName);
        if (truncated) {
            m_statusText = QStringLiteral("工具输出已截断（%1）").arg(toolName);
        }
        emit statusTextChanged();
        if (truncated) {
            setToolHintText(QStringLiteral("📎 %1 输出已截断").arg(toolName));
        } else {
            setToolHintText(result.success
                ? QStringLiteral("✅ %1").arg(toolName)
                : QStringLiteral("❌ %1").arg(toolName));
        }

        bool appended = false;
        if (!result.success) {
            QString message = result.content.trimmed();
            if (message.size() > 320) {
                message = message.left(320) + QStringLiteral("...");
            }
            const QString lowerMessage = message.toLower();
            const bool noisyRecoverable =
                lowerMessage.contains(QStringLiteral("reader fallback timed out"))
                || lowerMessage.contains(QStringLiteral("target site returned 403"))
                || lowerMessage.contains(QStringLiteral("tool not found: brainstorming"));
            if (!noisyRecoverable) {
                m_messagesModel->addMessage(
                    static_cast<int>(MessageRole::System),
                    QStringLiteral("工具 %1 执行失败：%2").arg(toolName, message));
                appended = true;
            }
        } else if (truncated && !cachePath.isEmpty()) {
            m_messagesModel->addMessage(
                static_cast<int>(MessageRole::System),
                QStringLiteral("工具 %1 输出已截断，完整内容：%2").arg(toolName, cachePath));
            appended = true;
        }

        if (appended) {
            if (riskLevel >= 0 && !riskSummary.isEmpty()) {
                m_messagesModel->addMessage(
                    static_cast<int>(MessageRole::System),
                    QStringLiteral("Shell 风险评估（级别 %1）：%2").arg(riskLevel).arg(riskSummary));
            }
            if (call.name == QStringLiteral("start_subagent") && !result.success) {
                m_messagesModel->addMessage(
                    static_cast<int>(MessageRole::System),
                    QStringLiteral("子 Agent 任务失败"));
            }
            m_sessionManager->updateMessages(m_messagesModel->messages());
        }
    });

    connect(m_agentService, &AgentService::responseComplete, this, [this](const QString& fullResponse) {
        flushPendingAssistantChunk(true);
        const QString activeSessionId = m_sessionManager ? m_sessionManager->currentSessionId() : QString();
        if (m_streamingAssistantRow >= 0) {
            const QString normalized = fullResponse.trimmed();
            const QVariantMap row = m_messagesModel->get(m_streamingAssistantRow);
            const QString existing = row.value(QStringLiteral("content")).toString();
            const QString existingTrimmed = existing.trimmed();

            // 保留流式过程中已展示的内容，避免在完成时整条被覆盖导致前文消失。
            if (normalized.isEmpty()) {
                if (existingTrimmed.isEmpty()) {
                    QString fallback = QStringLiteral("（模型返回空响应）");
                    const MessageList snapshot = m_messagesModel->messages();
                    for (auto it = snapshot.crbegin(); it != snapshot.crend(); ++it) {
                        if (it->role != MessageRole::Tool) {
                            continue;
                        }
                        const QString toolText = it->content.simplified();
                        if (toolText.isEmpty()) {
                            continue;
                        }
                        QString snippet = toolText;
                        if (snippet.size() > 160) {
                            snippet = snippet.left(160) + QStringLiteral("...");
                        }
                        fallback = QStringLiteral("模型未给出最终回答，最近工具结果：%1").arg(snippet);
                        break;
                    }
                    m_messagesModel->updateMessage(m_streamingAssistantRow, fallback);
                }
            } else if (existingTrimmed.isEmpty()) {
                m_messagesModel->updateMessage(m_streamingAssistantRow, normalized);
            } else if (normalized == existingTrimmed || normalized.startsWith(existingTrimmed)) {
                m_messagesModel->updateMessage(m_streamingAssistantRow, normalized);
            } else if (!existingTrimmed.contains(normalized)) {
                m_messagesModel->updateMessage(
                    m_streamingAssistantRow,
                    existingTrimmed + QStringLiteral("\n\n") + normalized);
            }
        }

        if (!activeSessionId.isEmpty() && m_externalPlatformManager) {
            const auto it = m_externalConversationContexts.constFind(activeSessionId);
            if (it != m_externalConversationContexts.constEnd()) {
                const ExternalConversationContext& ctx = it.value();
                if (ctx.platform == QStringLiteral("Telegram")) {
                    m_externalPlatformManager->sendTelegramMessage(ctx.userId, fullResponse);
                } else if (ctx.platform == QStringLiteral("Feishu")) {
                    if (!ctx.messageId.isEmpty()) {
                        m_externalPlatformManager->sendFeishuReply(ctx.messageId, fullResponse);
                    } else {
                        m_externalPlatformManager->sendFeishuMessage(ctx.userId, fullResponse);
                    }
                }
            }
        }

        m_sessionManager->updateMessages(m_messagesModel->messages());
        refreshSessions();
        persistUsageSnapshotIfNeeded();
        setGenerating(false);
        m_statusText = QStringLiteral("已完成");
        emit statusTextChanged();
        setToolHintText(QString());
        syncCurrentSession();
        m_streamingAssistantRow = -1;
    });

    connect(m_agentService, &AgentService::usageReport, this, [this](int promptTokens, int completionTokens, int totalTokens) {
        updateUsage(promptTokens, completionTokens, totalTokens);
    });

    connect(m_agentService, &AgentService::errorOccurred, this, [this](const QString& error) {
        flushPendingAssistantChunk(true);
        if (m_streamingAssistantRow >= 0) {
            const QVariantMap row = m_messagesModel->get(m_streamingAssistantRow);
            const QString existing = row.value(QStringLiteral("content")).toString().trimmed();
            const QString suffix = QStringLiteral("\n\n[生成中断：%1。已保留上方已生成内容，可继续发送“继续”。]").arg(error);
            m_messagesModel->updateMessage(
                m_streamingAssistantRow,
                existing.isEmpty()
                    ? QStringLiteral("请求失败: %1").arg(error)
                    : existing + suffix);
            m_sessionManager->updateMessages(m_messagesModel->messages());
            refreshSessions();
            m_streamingAssistantRow = -1;
        }

        m_statusText = error;
        emit statusTextChanged();
        persistUsageSnapshotIfNeeded();
        setGenerating(false);
        setToolHintText(QString());
    });

    connect(m_agentService, &AgentService::generationStopped, this, [this]() {
        flushPendingAssistantChunk(true);
        if (m_streamingAssistantRow >= 0) {
            const QVariantMap row = m_messagesModel->get(m_streamingAssistantRow);
            const QString existing = row.value(QStringLiteral("content")).toString().trimmed();
            m_messagesModel->updateMessage(
                m_streamingAssistantRow,
                existing.isEmpty()
                    ? QStringLiteral("已停止")
                    : existing + QStringLiteral("\n\n[生成已停止。已保留上方已生成内容。]"));
            m_sessionManager->updateMessages(m_messagesModel->messages());
            refreshSessions();
            m_streamingAssistantRow = -1;
        }

        persistUsageSnapshotIfNeeded();
        setGenerating(false);
        m_statusText = QStringLiteral("已停止");
        emit statusTextChanged();
        setToolHintText(QString());
    });

    connect(m_externalPlatformManager, &ExternalPlatformManager::externalMessageReceived,
            this, [this](const QString& platform, const QString& userId, const QString& content, const QString& messageId) {
        const QString sessionName = QStringLiteral("%1: %2").arg(platform, userId);
        QString sessionId;
        const QList<Session> sessions = m_sessionManager->listSessions();
        for (const Session& session : sessions) {
            if (session.name == sessionName) {
                sessionId = session.id;
                break;
            }
        }

        if (sessionId.isEmpty()) {
            sessionId = m_sessionManager->createSession(sessionName);
        }

        m_externalConversationContexts[sessionId] = {platform, userId, messageId};

        m_sessionManager->switchSession(sessionId);
        sendMessage(content);
    });

    connect(m_agentService, &AgentService::conversationChanged, m_sessionManager, &SessionManager::updateMessages);

    if (m_agentService && m_agentService->toolExecutor()) {
        m_agentService->toolExecutor()->setAutoApproveConfirmations(true);
        // QML 路径暂不弹权限确认对话框，避免需要确认的工具全部超时后被误判为“用户拒绝”。
        connect(m_agentService->toolExecutor(), &ToolExecutor::permissionRequest, this,
                [](const ToolCall&, const std::function<void(bool)>& callback) {
                    if (callback) {
                        callback(true);
                    }
                },
                Qt::UniqueConnection);
    }

    if (m_agentService && m_agentService->orchestrator()) {
        connect(m_agentService->orchestrator(), &AgentOrchestrator::runStarted, this, [this](const QString& requestId) {
            m_statusText = QStringLiteral("任务开始：%1").arg(requestId);
            emit statusTextChanged();
        });
        connect(m_agentService->orchestrator(), &AgentOrchestrator::runFinished, this, [this](const QString& requestId, const QString& status) {
            m_statusText = QStringLiteral("任务结束：%1（%2）").arg(requestId, status);
            emit statusTextChanged();
            setGenerating(false);
            if (status != QStringLiteral("completed")) {
                setToolHintText(QString());
            }
            if (m_streamingAssistantRow >= 0) {
                m_sessionManager->updateMessages(m_messagesModel->messages());
                refreshSessions();
                m_streamingAssistantRow = -1;
            }
        });
        connect(m_agentService->orchestrator(), &AgentOrchestrator::inputRejected, this, [this](const QString& reason) {
            m_statusText = reason;
            emit statusTextChanged();
            setGenerating(false);
            setToolHintText(QString());
            if (m_streamingAssistantRow >= 0) {
                m_messagesModel->updateMessage(m_streamingAssistantRow, QStringLiteral("请求已拒绝: %1").arg(reason));
                m_sessionManager->updateMessages(m_messagesModel->messages());
                m_streamingAssistantRow = -1;
            }
        });
    }
}

void QmlBackend::syncCurrentSession() {
    const Session session = m_sessionManager->currentSession();
    m_currentSessionId = session.id;
    m_currentSessionName = session.name.isEmpty() ? QStringLiteral("Claw++") : session.name;
    m_messagesModel->setMessages(session.messages);
    refreshSessions();
    emit currentSessionChanged();
    m_statusText = session.id.isEmpty() ? QStringLiteral("未选择会话") : QStringLiteral("会话已就绪");
    emit statusTextChanged();
    setToolHintText(QString());
}

void QmlBackend::setGenerating(bool generating) {
    if (m_generating == generating) {
        return;
    }

    m_generating = generating;
    emit generatingChanged();
}

void QmlBackend::setToolHintText(const QString& hint) {
    if (m_toolHintText == hint) {
        return;
    }
    m_toolHintText = hint;
    emit toolHintTextChanged();
}

void QmlBackend::syncCompressionThresholdFromModel() {
    if (!m_memoryManager) {
        return;
    }
    const ProviderConfig providerConfig = ConfigManager::instance().getProviderConfig();
    m_memoryManager->setCompressionTokenThreshold(modelCompressionThreshold(providerConfig));
}

void QmlBackend::flushPendingAssistantChunk(bool force) {
    if (m_pendingAssistantChunk.isEmpty() || m_streamingAssistantRow < 0) {
        return;
    }
    if (!force && m_pendingAssistantChunk.size() < 10) {
        return;
    }
    const QString delta = m_pendingAssistantChunk;
    m_pendingAssistantChunk.clear();
    m_messagesModel->appendToMessage(m_streamingAssistantRow, delta);
}

void QmlBackend::runAssistantWithInternalPrompt(const QString& prompt) {
    if (!m_agentService) {
        return;
    }
    const QString trimmed = prompt.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }
    if (m_generating) {
        return;
    }

    syncCompressionThresholdFromModel();
    ensureSessionExists();
    if (m_currentSessionId.isEmpty()) {
        syncCurrentSession();
    }

    m_streamingAssistantRow = m_messagesModel->addMessage(static_cast<int>(MessageRole::Assistant), QString());
    m_pendingAssistantChunk.clear();
    if (m_chunkFlushTimer) {
        m_chunkFlushTimer->stop();
    }
    m_sessionManager->updateMessages(m_messagesModel->messages());
    refreshSessions();

    setGenerating(true);
    m_promptTokens = 0;
    m_completionTokens = 0;
    m_totalTokens = 0;
    m_usageDirtyCurrentRun = false;
    m_usageText = QStringLiteral("本轮消耗(API)：0 Token");
    emit usageTextChanged();
    m_statusText = QStringLiteral("生成中");
    emit statusTextChanged();
    setToolHintText(QString());

    if (m_agentService->orchestrator()) {
        m_agentService->orchestrator()->sendUserInput(trimmed);
    } else {
        m_agentService->sendMessage(trimmed);
    }
}

void QmlBackend::updateUsage(int promptTokens, int completionTokens, int totalTokens) {
    int safePrompt = qMax(0, promptTokens);
    int safeCompletion = qMax(0, completionTokens);
    int safeTotal = qMax(0, totalTokens);
    if (safeTotal <= 0) {
        safeTotal = safePrompt + safeCompletion;
    }

    // 流式阶段 provider 可能回传多份 usage（甚至存在回退值），请求内取单调不减值，避免 UI 数字跳变。
    if (m_generating) {
        safePrompt = qMax(m_promptTokens, safePrompt);
        safeCompletion = qMax(m_completionTokens, safeCompletion);
        safeTotal = qMax(m_totalTokens, qMax(safeTotal, safePrompt + safeCompletion));
    }

    m_promptTokens = safePrompt;
    m_completionTokens = safeCompletion;
    m_totalTokens = safeTotal;
    if (safeTotal > 0) {
        m_lastApiContextTokens = qMax(m_lastApiContextTokens, safeTotal);
    }
    m_usageText = QStringLiteral("本轮消耗(API)：%1 Token").arg(safeTotal);

    if (safeTotal > 0 || safePrompt > 0 || safeCompletion > 0) {
        m_usageDirtyCurrentRun = true;
    }

    emit usageTextChanged();
}

void QmlBackend::persistUsageSnapshotIfNeeded() {
    if (!m_usageDirtyCurrentRun) {
        return;
    }
    if (m_totalTokens <= 0 && m_promptTokens <= 0 && m_completionTokens <= 0) {
        m_usageDirtyCurrentRun = false;
        return;
    }

    QVariantMap entry;
    entry.insert(QStringLiteral("timestamp"), QDateTime::currentDateTime().toString(Qt::ISODate));
    entry.insert(QStringLiteral("prompt"), m_promptTokens);
    entry.insert(QStringLiteral("completion"), m_completionTokens);
    entry.insert(QStringLiteral("total"), m_totalTokens);
    m_tokenUsageHistory.append(normalizeTokenUsageEntry(entry));

    m_accPromptTokens += m_promptTokens;
    m_accCompletionTokens += m_completionTokens;
    m_accTotalTokens += m_totalTokens;
    saveTokenUsageHistory();
    m_usageDirtyCurrentRun = false;
}

void QmlBackend::ensureSessionExists() {
    if (m_sessionManager->hasCurrentSession()) {
        return;
    }

    const QList<Session> sessions = m_sessionManager->listSessions();
    if (!sessions.isEmpty()) {
        m_sessionManager->switchSession(sessions.first().id);
        return;
    }

    const QString sessionId = m_sessionManager->createSession(QStringLiteral("新聊天"));
    m_sessionManager->switchSession(sessionId);
}

QString QmlBackend::tokenUsageHistoryPath() const {
    const QString memoryDirPath = m_workspaceRoot + QStringLiteral("/memory");
    QDir().mkpath(memoryDirPath);
    return QDir(memoryDirPath).filePath(QStringLiteral("token_usage_history.json"));
}

void QmlBackend::rebuildTokenUsageAccumulators() {
    m_accPromptTokens = 0;
    m_accCompletionTokens = 0;
    m_accTotalTokens = 0;

    for (const QVariant& value : m_tokenUsageHistory) {
        const QVariantMap entry = normalizeTokenUsageEntry(value.toMap());
        m_accPromptTokens += entry.value(QStringLiteral("prompt")).toInt();
        m_accCompletionTokens += entry.value(QStringLiteral("completion")).toInt();
        m_accTotalTokens += entry.value(QStringLiteral("total")).toInt();
    }
}

void QmlBackend::loadTokenUsageHistory() {
    m_tokenUsageHistory.clear();

    QFile file(tokenUsageHistoryPath());
    if (!file.exists()) {
        rebuildTokenUsageAccumulators();
        return;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        rebuildTokenUsageAccumulators();
        return;
    }

    const QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError || !doc.isArray()) {
        rebuildTokenUsageAccumulators();
        return;
    }

    const QJsonArray array = doc.array();
    for (const QJsonValue& value : array) {
        if (!value.isObject()) {
            continue;
        }

        const QJsonObject obj = value.toObject();
        QVariantMap rawEntry;
        rawEntry.insert(QStringLiteral("timestamp"), obj.value(QStringLiteral("timestamp")).toString());
        rawEntry.insert(QStringLiteral("prompt"), obj.value(QStringLiteral("prompt")).toInt());
        rawEntry.insert(QStringLiteral("completion"), obj.value(QStringLiteral("completion")).toInt());
        rawEntry.insert(QStringLiteral("total"), obj.value(QStringLiteral("total")).toInt());
        m_tokenUsageHistory.append(normalizeTokenUsageEntry(rawEntry));
    }

    rebuildTokenUsageAccumulators();

    if (!m_tokenUsageHistory.isEmpty()) {
        const QVariantMap lastEntry = normalizeTokenUsageEntry(m_tokenUsageHistory.last().toMap());
        m_promptTokens = lastEntry.value(QStringLiteral("prompt")).toInt();
        m_completionTokens = lastEntry.value(QStringLiteral("completion")).toInt();
        m_totalTokens = lastEntry.value(QStringLiteral("total")).toInt();
        m_usageText = QStringLiteral("本轮消耗(API)：%1 Token").arg(m_totalTokens);
    }
}

bool QmlBackend::saveTokenUsageHistory() const {
    QJsonArray array;
    for (const QVariant& value : m_tokenUsageHistory) {
        const QVariantMap entry = normalizeTokenUsageEntry(value.toMap());
        QJsonObject obj;
        obj.insert(QStringLiteral("timestamp"), entry.value(QStringLiteral("timestamp")).toString());
        obj.insert(QStringLiteral("prompt"), entry.value(QStringLiteral("prompt")).toInt());
        obj.insert(QStringLiteral("completion"), entry.value(QStringLiteral("completion")).toInt());
        obj.insert(QStringLiteral("total"), entry.value(QStringLiteral("total")).toInt());
        array.append(obj);
    }

    QFile file(tokenUsageHistoryPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return false;
    }

    const QByteArray data = QJsonDocument(array).toJson(QJsonDocument::Indented);
    const bool ok = file.write(data) == data.size();
    file.close();
    return ok;
}

SessionListModel* QmlBackend::sessionsModel() const {
    return m_sessionsModel;
}

MessageListModel* QmlBackend::messagesModel() const {
    return m_messagesModel;
}

QString QmlBackend::currentSessionId() const {
    return m_currentSessionId;
}

QString QmlBackend::currentSessionName() const {
    return m_currentSessionName;
}

QString QmlBackend::currentModelName() const {
    const QString model = ConfigManager::instance().getProviderConfig().model.trimmed();
    return model.isEmpty() ? QStringLiteral("assistant") : model;
}

QString QmlBackend::statusText() const {
    return m_statusText.isEmpty() ? QStringLiteral("运行中") : m_statusText;
}

QString QmlBackend::toolHintText() const {
    return m_toolHintText;
}

QString QmlBackend::usageText() const {
    return m_usageText.isEmpty() ? QStringLiteral("本轮消耗(API)：0 Token") : m_usageText;
}

bool QmlBackend::generating() const {
    return m_generating;
}

QString QmlBackend::workspacePath() const {
    return m_workspaceRoot;
}

QString QmlBackend::kernelPath() const {
    return m_kernelRoot;
}

QString QmlBackend::configFilePath() const {
    const QString configDirPath = m_workspaceRoot + QStringLiteral("/config");
    QDir().mkpath(configDirPath);
    return configDirPath;
}

QString QmlBackend::logDirectoryPath() const {
    const QString logsDirPath = m_workspaceRoot + QStringLiteral("/logs");
    QDir().mkpath(logsDirPath);
    return logsDirPath;
}

QString QmlBackend::memoryDirectoryPath() const {
    const QString memoryDirPath = m_workspaceRoot + QStringLiteral("/memory");
    QDir().mkpath(memoryDirPath);
    return memoryDirPath;
}

QString QmlBackend::readWorkspaceFile(const QString& relativePath) const {
    const QString cleaned = QDir::cleanPath(relativePath.trimmed());
    if (cleaned.isEmpty() || QDir::isAbsolutePath(cleaned) || cleaned.startsWith(QStringLiteral(".."))) {
        return {};
    }

    const QString absolutePath = QDir(m_workspaceRoot).filePath(cleaned);
    QFile file(absolutePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }

    return QString::fromUtf8(file.readAll());
}

bool QmlBackend::writeWorkspaceFile(const QString& relativePath, const QString& content) {
    const QString cleaned = QDir::cleanPath(relativePath.trimmed());
    if (cleaned.isEmpty() || QDir::isAbsolutePath(cleaned) || cleaned.startsWith(QStringLiteral(".."))) {
        return false;
    }

    const QString absolutePath = QDir(m_workspaceRoot).filePath(cleaned);
    const QFileInfo info(absolutePath);
    QDir().mkpath(info.absolutePath());

    QFile file(absolutePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return false;
    }

    const QByteArray data = content.toUtf8();
    if (file.write(data) != data.size()) {
        return false;
    }

    file.close();
    return true;
}

QVariantMap QmlBackend::getSettings() const {
    const ConfigManager& config = ConfigManager::instance();

    QVariantMap settings;
    settings.insert(QStringLiteral("apiKey"), config.getApiKey());
    settings.insert(QStringLiteral("providerType"), config.getProviderType());
    settings.insert(QStringLiteral("baseUrl"), config.getBaseUrl());
    settings.insert(QStringLiteral("model"), config.getModel());
    settings.insert(QStringLiteral("telegramToken"), config.getTelegramToken());
    settings.insert(QStringLiteral("feishuAppId"), config.getFeishuAppId());
    settings.insert(QStringLiteral("feishuAppSecret"), config.getFeishuAppSecret());
    settings.insert(QStringLiteral("feishuVerificationToken"), config.getFeishuVerificationToken());
    settings.insert(QStringLiteral("feishuPort"), config.getFeishuPort());
    settings.insert(QStringLiteral("discordWebhookUrl"), config.getExternalValue(QStringLiteral("discord_webhook_url")));
    settings.insert(QStringLiteral("dingtalkWebhookUrl"), config.getExternalValue(QStringLiteral("dingtalk_webhook_url")));
    settings.insert(QStringLiteral("wechatWebhookUrl"), config.getExternalValue(QStringLiteral("wechat_webhook_url")));
    settings.insert(QStringLiteral("qqWebhookUrl"), config.getExternalValue(QStringLiteral("qq_webhook_url")));
    settings.insert(QStringLiteral("wecomWebhookUrl"), config.getExternalValue(QStringLiteral("wecom_webhook_url")));
    settings.insert(QStringLiteral("messageChannel"), config.getExternalValue(QStringLiteral("message_channel")));
    settings.insert(QStringLiteral("chatFontSize"), config.getExternalValue(QStringLiteral("chat_font_size")).toInt());
    settings.insert(QStringLiteral("chatLineHeight"), config.getExternalValue(QStringLiteral("chat_line_height")).toDouble());
    return settings;
}

QVariantMap QmlBackend::getCompressionInfo() const {
    QVariantMap info;

    const ProviderConfig providerConfig = ConfigManager::instance().getProviderConfig();
    const int contextLimit = modelctx::inferModelContextLimit(providerConfig);
    const int threshold = m_memoryManager
        ? m_memoryManager->compressionTokenThreshold()
        : modelCompressionThreshold(providerConfig);
    const int memoryTokenCount = m_memoryManager ? m_memoryManager->getTokenCount() : 0;
    const int tokenCount = m_lastApiContextTokens > 0 ? m_lastApiContextTokens : memoryTokenCount;
    const bool needsCompression = tokenCount >= threshold;
    const int messageCount = m_memoryManager ? m_memoryManager->getAllMessages().size() : 0;

    info.insert(QStringLiteral("tokenCount"), tokenCount);
    info.insert(QStringLiteral("memoryTokenCount"), memoryTokenCount);
    info.insert(QStringLiteral("apiTokenCount"), m_lastApiContextTokens);
    info.insert(QStringLiteral("apiContextTokenCount"), m_lastApiContextTokens);
    info.insert(QStringLiteral("threshold"), threshold);
    info.insert(QStringLiteral("contextLimit"), contextLimit);
    info.insert(QStringLiteral("messageCount"), messageCount);
    info.insert(QStringLiteral("needsCompression"), needsCompression);
    info.insert(
        QStringLiteral("status"),
        needsCompression
            ? QStringLiteral("已达到最大上下文 60% 阈值，下一轮将进行压缩")
            : QStringLiteral("上下文正常（压缩阈值 = 模型最大上下文 %1 × 60%%）").arg(contextLimit)
    );

    return info;
}

QVariantMap QmlBackend::getMemoryOverview() const {
    QVariantMap info;
    if (!m_memoryManager) {
        info.insert(QStringLiteral("status"), QStringLiteral("memory manager unavailable"));
        return info;
    }

    const QString memoryText = m_memoryManager->readLongTerm();
    const QString historyText = m_memoryManager->readHistoryLog();
    QStringList historyLines = historyText.split('\n', Qt::SkipEmptyParts);
    const int maxHistoryLines = 40;
    if (historyLines.size() > maxHistoryLines) {
        historyLines = historyLines.mid(historyLines.size() - maxHistoryLines);
    }

    QString memoryPreview = memoryText;
    if (memoryPreview.size() > 2000) {
        memoryPreview = memoryPreview.left(2000) + QStringLiteral("\n...[已截断]...");
    }

    info.insert(QStringLiteral("memoryPath"), QDir(m_workspaceRoot).filePath(QStringLiteral("memory/MEMORY.md")));
    info.insert(QStringLiteral("historyPath"), QDir(m_workspaceRoot).filePath(QStringLiteral("memory/HISTORY.md")));
    info.insert(QStringLiteral("memoryChars"), memoryText.size());
    info.insert(QStringLiteral("historyLines"), historyLines.size());
    info.insert(QStringLiteral("memoryPreview"), memoryPreview);
    info.insert(QStringLiteral("historyPreview"), historyLines.join('\n'));
    info.insert(QStringLiteral("tokenCount"), m_memoryManager->getTokenCount());
    info.insert(QStringLiteral("messageCount"), m_memoryManager->getAllMessages().size());
    info.insert(QStringLiteral("needsCompression"), m_memoryManager->needsCompression());
    info.insert(QStringLiteral("status"), QStringLiteral("ok"));
    return info;
}

QVariantList QmlBackend::searchMemory(const QString& query, int topK) const {
    QVariantList rows;
    if (!m_memoryManager) {
        return rows;
    }

    QString normalizedQuery = query.trimmed();
    if (normalizedQuery.isEmpty()) {
        normalizedQuery = m_currentSessionName.isEmpty() ? m_currentSessionId : m_currentSessionName;
    }

    const QStringList hits = m_memoryManager->queryRelevantMemory(normalizedQuery, topK);
    for (const QString& hit : hits) {
        rows.append(hit);
    }
    return rows;
}

bool QmlBackend::appendMemoryNote(const QString& note) {
    if (!m_memoryManager) {
        return false;
    }

    const QString trimmed = note.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    const QString sessionLabel = m_currentSessionName.isEmpty()
        ? (m_currentSessionId.isEmpty() ? QStringLiteral("active") : m_currentSessionId)
        : m_currentSessionName;
    const QString entry = QStringLiteral(
        "### Manual Memory Note (%1)\n- Session: %2\n- Note: %3")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm")),
             sessionLabel,
             trimmed);
    m_memoryManager->saveMemoryUpdate(QString(), entry);
    return true;
}

bool QmlBackend::clearMemoryStorage() {
    if (!m_memoryManager) {
        return false;
    }
    m_memoryManager->clearPersistentStorage();
    return true;
}

QVariantList QmlBackend::getSkillsOverview() const {
    QVariantList result;
    const QList<Skill> skills = ContextBuilder::instance().skills();

    for (const Skill& skill : skills) {
        QVariantMap item;
        item.insert(QStringLiteral("name"), skill.name);
        item.insert(QStringLiteral("description"), skill.description);
        item.insert(QStringLiteral("triggers"), skill.triggers.join(QStringLiteral(", ")));
        item.insert(QStringLiteral("available"), skill.meta.available);
        item.insert(QStringLiteral("always"), skill.meta.always);
        item.insert(QStringLiteral("path"), skill.filePath);
        result.append(item);
    }

    return result;
}

QString QmlBackend::reloadSkills() {
    const bool ok = ContextBuilder::instance().reloadSkills();
    if (ok) {
        const int skillCount = ContextBuilder::instance().skills().size();
        m_statusText = QStringLiteral("Skills 已重载：%1 项（目录：%2）")
            .arg(skillCount)
            .arg(QDir::toNativeSeparators(ContextBuilder::instance().skillsDirectory()));
    } else {
        m_statusText = QStringLiteral("Skills 重载失败：未找到有效 skills 目录");
    }
    emit statusTextChanged();
    return m_statusText;
}

QStringList QmlBackend::getSkillLoadErrors() const {
    return ContextBuilder::instance().skillLoadErrors();
}

QVariantMap QmlBackend::getTokenStats() const {
    QVariantMap stats;
    stats.insert(QStringLiteral("lastPrompt"), m_promptTokens);
    stats.insert(QStringLiteral("lastCompletion"), m_completionTokens);
    stats.insert(QStringLiteral("lastTotal"), m_totalTokens);
    stats.insert(QStringLiteral("accPrompt"), m_accPromptTokens);
    stats.insert(QStringLiteral("accCompletion"), m_accCompletionTokens);
    stats.insert(QStringLiteral("accTotal"), m_accTotalTokens);
    stats.insert(QStringLiteral("messageCount"), m_messagesModel ? m_messagesModel->rowCount() : 0);
    stats.insert(QStringLiteral("usageText"), usageText());

    const QVariantMap analytics = getTokenUsageAnalytics(30, QDate::currentDate().year());
    stats.insert(QStringLiteral("currentMonthTotal"), analytics.value(QStringLiteral("currentMonthTotal")).toInt());
    stats.insert(QStringLiteral("currentYearTotal"), analytics.value(QStringLiteral("currentYearTotal")).toInt());
    stats.insert(QStringLiteral("allTimeTotal"), analytics.value(QStringLiteral("allTimeTotal")).toInt());

    return stats;
}

QVariantMap QmlBackend::getTokenUsageAnalytics(int days, int year) const {
    QVariantMap analytics;

    const QDate today = QDate::currentDate();
    const int safeDays = qBound(7, days, 3650);
    const QDate startDate = today.addDays(-(safeDays - 1));
    const int targetYear = year > 0 ? year : today.year();

    QMap<QDate, QVariantMap> dailyAggregates;
    for (int i = 0; i < safeDays; ++i) {
        const QDate date = startDate.addDays(i);
        QVariantMap row;
        row.insert(QStringLiteral("date"), date.toString(QStringLiteral("yyyy-MM-dd")));
        row.insert(QStringLiteral("label"), date.toString(QStringLiteral("MM-dd")));
        row.insert(QStringLiteral("prompt"), 0);
        row.insert(QStringLiteral("completion"), 0);
        row.insert(QStringLiteral("total"), 0);
        dailyAggregates.insert(date, row);
    }

    QMap<int, QVariantMap> monthlyAggregates;
    for (int month = 1; month <= 12; ++month) {
        QVariantMap row;
        row.insert(QStringLiteral("year"), targetYear);
        row.insert(QStringLiteral("month"), month);
        row.insert(QStringLiteral("label"), QStringLiteral("%1-%2").arg(targetYear).arg(month, 2, 10, QLatin1Char('0')));
        row.insert(QStringLiteral("prompt"), 0);
        row.insert(QStringLiteral("completion"), 0);
        row.insert(QStringLiteral("total"), 0);
        monthlyAggregates.insert(month, row);
    }

    QMap<int, QVariantMap> yearlyAggregates;

    QVariantList recentEvents;
    int allTimePrompt = 0;
    int allTimeCompletion = 0;
    int allTimeTotal = 0;
    int currentMonthTotal = 0;
    int currentYearTotal = 0;

    const int currentYear = today.year();
    const int currentMonth = today.month();

    for (const QVariant& value : m_tokenUsageHistory) {
        const QVariantMap entry = normalizeTokenUsageEntry(value.toMap());
        const QDateTime timestamp = parseTokenUsageTimestamp(entry.value(QStringLiteral("timestamp")).toString());
        const QDate date = timestamp.date();
        const int prompt = entry.value(QStringLiteral("prompt")).toInt();
        const int completion = entry.value(QStringLiteral("completion")).toInt();
        const int total = entry.value(QStringLiteral("total")).toInt();

        allTimePrompt += prompt;
        allTimeCompletion += completion;
        allTimeTotal += total;

        if (date.year() == currentYear) {
            currentYearTotal += total;
            if (date.month() == currentMonth) {
                currentMonthTotal += total;
            }
        }

        if (date >= startDate && date <= today) {
            QVariantMap row = dailyAggregates.value(date);
            appendUsageToAggregate(&row, prompt, completion, total);
            dailyAggregates.insert(date, row);
        }

        if (date.year() == targetYear) {
            QVariantMap row = monthlyAggregates.value(date.month());
            appendUsageToAggregate(&row, prompt, completion, total);
            monthlyAggregates.insert(date.month(), row);
        }

        QVariantMap yearlyRow = yearlyAggregates.value(date.year());
        if (yearlyRow.isEmpty()) {
            yearlyRow.insert(QStringLiteral("year"), date.year());
            yearlyRow.insert(QStringLiteral("label"), QString::number(date.year()));
            yearlyRow.insert(QStringLiteral("prompt"), 0);
            yearlyRow.insert(QStringLiteral("completion"), 0);
            yearlyRow.insert(QStringLiteral("total"), 0);
        }
        appendUsageToAggregate(&yearlyRow, prompt, completion, total);
        yearlyAggregates.insert(date.year(), yearlyRow);
    }

    QVariantList dailySeries;
    for (auto it = dailyAggregates.begin(); it != dailyAggregates.end(); ++it) {
        dailySeries.append(it.value());
    }

    QVariantList monthlySeries;
    for (int month = 1; month <= 12; ++month) {
        monthlySeries.append(monthlyAggregates.value(month));
    }

    QVariantList yearlySeries;
    QVariantList availableYears;
    for (auto it = yearlyAggregates.begin(); it != yearlyAggregates.end(); ++it) {
        yearlySeries.append(it.value());
        availableYears.append(it.key());
    }
    if (availableYears.isEmpty()) {
        availableYears.append(currentYear);
    }

    for (int i = m_tokenUsageHistory.size() - 1; i >= 0 && recentEvents.size() < 240; --i) {
        recentEvents.append(normalizeTokenUsageEntry(m_tokenUsageHistory.at(i).toMap()));
    }

    analytics.insert(QStringLiteral("days"), safeDays);
    analytics.insert(QStringLiteral("targetYear"), targetYear);
    analytics.insert(QStringLiteral("dailySeries"), dailySeries);
    analytics.insert(QStringLiteral("monthlySeries"), monthlySeries);
    analytics.insert(QStringLiteral("yearlySeries"), yearlySeries);
    analytics.insert(QStringLiteral("availableYears"), availableYears);
    analytics.insert(QStringLiteral("recentEvents"), recentEvents);
    analytics.insert(QStringLiteral("allTimePrompt"), allTimePrompt);
    analytics.insert(QStringLiteral("allTimeCompletion"), allTimeCompletion);
    analytics.insert(QStringLiteral("allTimeTotal"), allTimeTotal);
    analytics.insert(QStringLiteral("currentMonthTotal"), currentMonthTotal);
    analytics.insert(QStringLiteral("currentYearTotal"), currentYearTotal);
    return analytics;
}

bool QmlBackend::resetTokenUsage(const QString& scope, int year, int month) {
    const QString normalizedScope = scope.trimmed().toLower();
    const QDate today = QDate::currentDate();
    const int targetYear = year > 0 ? year : today.year();
    const int targetMonth = month > 0 ? month : today.month();

    if (normalizedScope == QStringLiteral("all")) {
        m_tokenUsageHistory.clear();
        m_statusText = QStringLiteral("Token 统计已全部重置");
    } else if (normalizedScope == QStringLiteral("year")) {
        QVariantList kept;
        for (const QVariant& value : m_tokenUsageHistory) {
            const QVariantMap entry = normalizeTokenUsageEntry(value.toMap());
            const QDateTime timestamp = parseTokenUsageTimestamp(entry.value(QStringLiteral("timestamp")).toString());
            if (timestamp.date().year() != targetYear) {
                kept.append(entry);
            }
        }
        m_tokenUsageHistory = kept;
        m_statusText = QStringLiteral("已重置 %1 年 Token 统计").arg(targetYear);
    } else if (normalizedScope == QStringLiteral("month")) {
        QVariantList kept;
        for (const QVariant& value : m_tokenUsageHistory) {
            const QVariantMap entry = normalizeTokenUsageEntry(value.toMap());
            const QDateTime timestamp = parseTokenUsageTimestamp(entry.value(QStringLiteral("timestamp")).toString());
            const QDate date = timestamp.date();
            if (!(date.year() == targetYear && date.month() == targetMonth)) {
                kept.append(entry);
            }
        }
        m_tokenUsageHistory = kept;
        m_statusText = QStringLiteral("已重置 %1-%2 Token 统计").arg(targetYear).arg(targetMonth, 2, 10, QLatin1Char('0'));
    } else {
        return false;
    }

    rebuildTokenUsageAccumulators();

    if (m_tokenUsageHistory.isEmpty()) {
        m_promptTokens = 0;
        m_completionTokens = 0;
        m_totalTokens = 0;
        m_usageText = QStringLiteral("本轮消耗(API)：0 Token");
    } else {
        const QVariantMap lastEntry = normalizeTokenUsageEntry(m_tokenUsageHistory.last().toMap());
        m_promptTokens = lastEntry.value(QStringLiteral("prompt")).toInt();
        m_completionTokens = lastEntry.value(QStringLiteral("completion")).toInt();
        m_totalTokens = lastEntry.value(QStringLiteral("total")).toInt();
        m_usageText = QStringLiteral("本轮消耗(API)：%1 Token").arg(m_totalTokens);
    }

    saveTokenUsageHistory();
    emit usageTextChanged();
    emit statusTextChanged();
    return true;
}

bool QmlBackend::applySettings(const QVariantMap& settings) {
    ConfigManager& config = ConfigManager::instance();

    if (settings.contains(QStringLiteral("apiKey"))) {
        config.setApiKey(settings.value(QStringLiteral("apiKey")).toString().trimmed());
    }
    if (settings.contains(QStringLiteral("providerType"))) {
        config.setProviderType(settings.value(QStringLiteral("providerType")).toString().trimmed());
    }
    if (settings.contains(QStringLiteral("baseUrl"))) {
        config.setBaseUrl(settings.value(QStringLiteral("baseUrl")).toString().trimmed());
    }
    if (settings.contains(QStringLiteral("model"))) {
        config.setModel(settings.value(QStringLiteral("model")).toString().trimmed());
    }
    if (settings.contains(QStringLiteral("telegramToken"))) {
        config.setTelegramToken(settings.value(QStringLiteral("telegramToken")).toString().trimmed());
    }
    if (settings.contains(QStringLiteral("feishuAppId"))) {
        config.setFeishuAppId(settings.value(QStringLiteral("feishuAppId")).toString().trimmed());
    }
    if (settings.contains(QStringLiteral("feishuAppSecret"))) {
        config.setFeishuAppSecret(settings.value(QStringLiteral("feishuAppSecret")).toString().trimmed());
    }
    if (settings.contains(QStringLiteral("feishuVerificationToken"))) {
        config.setFeishuVerificationToken(settings.value(QStringLiteral("feishuVerificationToken")).toString().trimmed());
    }
    if (settings.contains(QStringLiteral("feishuPort"))) {
        config.setFeishuPort(settings.value(QStringLiteral("feishuPort")).toInt());
    }
    if (settings.contains(QStringLiteral("discordWebhookUrl"))) {
        config.setExternalValue(QStringLiteral("discord_webhook_url"), settings.value(QStringLiteral("discordWebhookUrl")).toString().trimmed());
    }
    if (settings.contains(QStringLiteral("dingtalkWebhookUrl"))) {
        config.setExternalValue(QStringLiteral("dingtalk_webhook_url"), settings.value(QStringLiteral("dingtalkWebhookUrl")).toString().trimmed());
    }
    if (settings.contains(QStringLiteral("wechatWebhookUrl"))) {
        config.setExternalValue(QStringLiteral("wechat_webhook_url"), settings.value(QStringLiteral("wechatWebhookUrl")).toString().trimmed());
    }
    if (settings.contains(QStringLiteral("qqWebhookUrl"))) {
        config.setExternalValue(QStringLiteral("qq_webhook_url"), settings.value(QStringLiteral("qqWebhookUrl")).toString().trimmed());
    }
    if (settings.contains(QStringLiteral("wecomWebhookUrl"))) {
        config.setExternalValue(QStringLiteral("wecom_webhook_url"), settings.value(QStringLiteral("wecomWebhookUrl")).toString().trimmed());
    }
    if (settings.contains(QStringLiteral("messageChannel"))) {
        config.setExternalValue(QStringLiteral("message_channel"), settings.value(QStringLiteral("messageChannel")).toString().trimmed());
    }
    if (settings.contains(QStringLiteral("chatFontSize"))) {
        const int fontSize = qBound(11, settings.value(QStringLiteral("chatFontSize")).toInt(), 20);
        config.setExternalValue(QStringLiteral("chat_font_size"), QString::number(fontSize));
    }
    if (settings.contains(QStringLiteral("chatLineHeight"))) {
        const double lineHeight = qBound(1.15, settings.value(QStringLiteral("chatLineHeight")).toDouble(), 2.0);
        config.setExternalValue(QStringLiteral("chat_line_height"), QString::number(lineHeight, 'f', 2));
    }

    config.save();

    ProviderConfig providerConfig = config.getProviderConfig();
    providerConfig.timeoutMs = qMax(providerConfig.timeoutMs, 600000);
    m_providerManager->setConfig(providerConfig);
    m_agentService->setProvider(m_providerManager);
    m_agentService->setProviderConfig(providerConfig);
    syncCompressionThresholdFromModel();
    emit currentModelNameChanged();

    m_externalPlatformManager->stopTelegramBot();
    m_externalPlatformManager->stopFeishuBot();

    const QString tgToken = config.getTelegramToken();
    if (!tgToken.isEmpty()) {
        m_externalPlatformManager->startTelegramBot(tgToken);
    }

    const QString fsAppId = config.getFeishuAppId();
    const QString fsAppSecret = config.getFeishuAppSecret();
    const QString fsVerificationToken = config.getFeishuVerificationToken();
    if (!fsAppId.isEmpty() && !fsAppSecret.isEmpty() && !fsVerificationToken.isEmpty()) {
        m_externalPlatformManager->startFeishuBot(fsAppId, fsAppSecret, fsVerificationToken, config.getFeishuPort());
    }

    m_providerTestPassed = false;
    m_statusText = QStringLiteral("设置已保存，请先重新测试 API");
    emit statusTextChanged();
    return true;
}

bool QmlBackend::canSendMessage() const {
    // 不再强制“先测试 API 才能输入”，避免聊天功能被锁死。
    return true;
}

QString QmlBackend::quickApiProbe() {
    ProviderConfig config = ConfigManager::instance().getProviderConfig();
    if (config.apiKey.trimmed().isEmpty() || config.baseUrl.trimmed().isEmpty()) {
        m_providerTestPassed = false;
        m_statusText = QStringLiteral("API 测试失败：请先在设置中填写 API Key 与 Base URL");
        emit statusTextChanged();
        return m_statusText;
    }

    const QString normalizedType = config.type.trimmed().toLower().isEmpty()
        ? QStringLiteral("openai")
        : config.type.trimmed().toLower();

    std::unique_ptr<ILLMProvider> provider;
    if (normalizedType == QStringLiteral("anthropic")) {
        provider = std::make_unique<AnthropicProvider>(config);
    } else if (normalizedType == QStringLiteral("gemini")) {
        provider = std::make_unique<GeminiProvider>(config);
    } else {
        provider = std::make_unique<OpenAIProvider>(config);
    }

    MessageList messages;
    messages.append(Message(MessageRole::User, QStringLiteral("ping")));

    ChatOptions options;
    options.model = config.model;
    options.maxTokens = 8;
    options.temperature = 0.0;
    options.stream = false;

    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);

    QString result;
    QObject::connect(&timeout, &QTimer::timeout, &loop, [&]() {
        result = QStringLiteral("API 测试超时");
        loop.quit();
    });

    timeout.start(12000);
    provider->chat(
        messages,
        options,
        [&](const QString&) {
            result = QStringLiteral("API 测试通过");
            loop.quit();
        },
        [&](const QString& error) {
            result = QStringLiteral("API 测试失败: %1").arg(error);
            loop.quit();
        });

    loop.exec();
    const QString finalResult = result.isEmpty() ? QStringLiteral("API 测试失败") : result;
    m_providerTestPassed = finalResult.startsWith(QStringLiteral("API 测试通过"));
    m_statusText = finalResult;
    emit statusTextChanged();
    return finalResult;
}

QString QmlBackend::runQuickSubagentTask(const QString& task) {
    const QString trimmedTask = task.trimmed();
    if (trimmedTask.isEmpty()) {
        return QStringLiteral("子 Agent 任务为空");
    }
    if (!m_agentService || !m_agentService->toolExecutor()) {
        return QStringLiteral("子 Agent 不可用：工具执行器未初始化");
    }

    ToolCall call;
    call.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    call.name = QStringLiteral("start_subagent");
    QJsonObject args;
    args[QStringLiteral("agent_type")] = QStringLiteral("analysis");
    args[QStringLiteral("task")] = trimmedTask;
    args[QStringLiteral("goal")] = QStringLiteral("提供简洁、可执行的结果");
    call.arguments = args;

    m_messagesModel->addMessage(
        static_cast<int>(MessageRole::System),
        QStringLiteral("ToolCard|Action|start_subagent|RUNNING\n%1").arg(
            QString::fromUtf8(QJsonDocument(args).toJson(QJsonDocument::Indented))));

    ToolResult result = m_agentService->toolExecutor()->execute(call);
    const QString status = result.success ? QStringLiteral("OK") : QStringLiteral("ERR");
    QString body = result.content.trimmed().isEmpty()
        ? QStringLiteral("子 Agent 执行完成（无输出）")
        : result.content;
    if (body.size() > 1800) {
        body = body.left(1800) + QStringLiteral("\n...[已省略后续内容]...");
    }

    m_messagesModel->addMessage(
        static_cast<int>(MessageRole::System),
        QStringLiteral("ToolCard|Observation|start_subagent|%1\n%2").arg(status, body));
    m_sessionManager->updateMessages(m_messagesModel->messages());
    refreshSessions();

    const QString resultText = result.success
        ? QStringLiteral("子 Agent 执行成功")
        : QStringLiteral("子 Agent 执行失败");
    m_statusText = resultText;
    emit statusTextChanged();
    return QStringLiteral("%1：%2").arg(resultText, body.left(280));
}

QVariantList QmlBackend::getWorkflowTemplates() const {
    QVariantList templates;

    QVariantMap research;
    research.insert(QStringLiteral("id"), QStringLiteral("research"));
    research.insert(QStringLiteral("title"), QStringLiteral("代理研究助手"));
    research.insert(QStringLiteral("description"), QStringLiteral("规划检索路径，调用工具执行，反思并迭代到可落地结论。"));
    templates.append(research);

    QVariantMap coding;
    coding.insert(QStringLiteral("id"), QStringLiteral("coding"));
    coding.insert(QStringLiteral("title"), QStringLiteral("代理编码助手"));
    coding.insert(QStringLiteral("description"), QStringLiteral("规划实现与验证步骤，执行代码相关动作并迭代修复。"));
    templates.append(coding);

    return templates;
}

QString QmlBackend::startAgentWorkflow(const QString& templateId,
                                       const QString& mode,
                                       const QString& task) {
    const QString normalizedTask = task.trimmed();
    if (normalizedTask.isEmpty()) {
        return QStringLiteral("工作流任务不能为空");
    }
    if (m_generating) {
        return QStringLiteral("上一条回复仍在生成，请先停止或等待完成");
    }

    const QString normalizedTemplate =
        templateId.trimmed().toLower() == QStringLiteral("coding")
        ? QStringLiteral("coding")
        : QStringLiteral("research");
    const bool parallelMode = mode.trimmed().toLower() == QStringLiteral("parallel");

    ensureSessionExists();
    if (m_currentSessionId.isEmpty()) {
        syncCurrentSession();
    }

    if (!parallelMode) {
        m_messagesModel->addMessage(
            static_cast<int>(MessageRole::System),
            QStringLiteral("ToolCard|Action|workflow_sequential|RUNNING\n模板：%1\n模式：单代理顺序\n计划：\n[ ] Make a plan\n[ ] Execute actions with tools\n[ ] Reflect and iterate")
                .arg(workflowTemplateTitle(normalizedTemplate)));
        m_sessionManager->updateMessages(m_messagesModel->messages());
        refreshSessions();
        const QString prompt = workflowInstructionPrompt(normalizedTemplate, normalizedTask, false);
        runAssistantWithInternalPrompt(prompt);
        return QStringLiteral("已启动顺序工作流：%1").arg(workflowTemplateTitle(normalizedTemplate));
    }

    if (!m_agentService || !m_agentService->toolExecutor()) {
        const QString prompt = workflowInstructionPrompt(normalizedTemplate, normalizedTask, false);
        runAssistantWithInternalPrompt(prompt);
        return QStringLiteral("并行子代理不可用，已回退为顺序工作流");
    }

    const int workflowProgressRow = m_messagesModel->addMessage(
        static_cast<int>(MessageRole::System),
        workflowParallelPlanCard(normalizedTemplate, QVector<bool>{false, false}, 0));
    m_sessionManager->updateMessages(m_messagesModel->messages());
    refreshSessions();

    const QPair<QString, QString> subtasks = workflowParallelSubtasks(normalizedTemplate, normalizedTask);
    QVector<ToolCall> calls;
    QVector<QString> branchTasks;
    calls.reserve(2);
    branchTasks.reserve(2);
    for (int i = 0; i < 2; ++i) {
        ToolCall call;
        call.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        call.name = QStringLiteral("start_subagent");
        QJsonObject args;
        const QString branchTask = i == 0 ? subtasks.first : subtasks.second;
        args.insert(QStringLiteral("agent_type"), i == 0 ? QStringLiteral("analysis") : QStringLiteral("review"));
        args.insert(QStringLiteral("task"), branchTask);
        args.insert(QStringLiteral("goal"), QStringLiteral("给出结构化结果，包含结论、依据、风险与下一步建议"));
        args.insert(QStringLiteral("max_iterations"), 5);
        call.arguments = args;
        calls.append(call);
        branchTasks.append(branchTask);
    }

    m_statusText = QStringLiteral("工作流并行执行中（子代理）");
    emit statusTextChanged();

    struct ParallelWorkflowState {
        QVector<ToolResult> results;
        QVector<bool> doneFlags;
        int completedCount = 0;
        bool hasFailure = false;
    };
    auto state = std::make_shared<ParallelWorkflowState>();
    state->results = QVector<ToolResult>(2);
    state->doneFlags = QVector<bool>(2, false);
    auto branchActionRows = std::make_shared<QVector<int>>();
    auto branchTaskTexts = std::make_shared<QVector<QString>>(branchTasks);
    branchActionRows->reserve(2);
    for (int i = 0; i < branchTasks.size(); ++i) {
        const QString branchName = i == 0 ? QStringLiteral("subagent_A") : QStringLiteral("subagent_B");
        const int row = m_messagesModel->addMessage(
            static_cast<int>(MessageRole::System),
            QStringLiteral("ToolCard|Action|%1|RUNNING\n主Agent分派任务：\n%2")
                .arg(branchName, branchTasks[i]));
        branchActionRows->append(row);
    }
    m_sessionManager->updateMessages(m_messagesModel->messages());
    refreshSessions();

    for (int i = 0; i < calls.size(); ++i) {
        auto* watcher = new QFutureWatcher<ToolResult>(this);
        connect(watcher, &QFutureWatcher<ToolResult>::finished, this, [this,
                                                                       watcher,
                                                                       state,
                                                                       branchActionRows,
                                                                       branchTaskTexts,
                                                                       i,
                                                                       workflowProgressRow,
                                                                       normalizedTemplate,
                                                                       normalizedTask]() {
            const ToolResult result = watcher->result();
            watcher->deleteLater();

            if (i < 0 || i >= state->results.size() || i >= state->doneFlags.size()) {
                return;
            }

            if (!state->doneFlags[i]) {
                state->doneFlags[i] = true;
                state->completedCount++;
            }
            state->results[i] = result;
            if (!result.success) {
                state->hasFailure = true;
            }

            QString branchBody = result.content.trimmed();
            if (branchBody.isEmpty()) {
                branchBody = result.success
                    ? QStringLiteral("子代理完成（无输出）")
                    : QStringLiteral("子代理失败（无错误详情）");
            }
            if (branchBody.size() > 2200) {
                branchBody = branchBody.left(2200) + QStringLiteral("\n...[已省略后续内容]...");
            }

            const QString branchLabel = i == 0 ? QStringLiteral("subagent_A") : QStringLiteral("subagent_B");
            if (branchActionRows && i >= 0 && i < branchActionRows->size()) {
                const int actionRow = branchActionRows->at(i);
                if (actionRow >= 0) {
                    const QString dispatchedTask = (branchTaskTexts && i < branchTaskTexts->size())
                        ? branchTaskTexts->at(i)
                        : QString();
                    m_messagesModel->updateMessage(
                        actionRow,
                        QStringLiteral("ToolCard|Action|%1|%2\n主Agent分派任务：\n%3\n\n子Agent结果：\n%4")
                            .arg(branchLabel,
                                 result.success ? QStringLiteral("DONE") : QStringLiteral("ERR"),
                                 dispatchedTask,
                                 branchBody));
                }
            }

            if (workflowProgressRow >= 0) {
                m_messagesModel->updateMessage(
                    workflowProgressRow,
                    workflowParallelPlanCard(normalizedTemplate, state->doneFlags, state->completedCount));
            }
            m_sessionManager->updateMessages(m_messagesModel->messages());
            refreshSessions();

            if (state->completedCount < 2) {
                m_statusText = QStringLiteral("工作流并行执行中（%1/2）").arg(state->completedCount);
                emit statusTextChanged();
                return;
            }

            QStringList branchSummaries;
            for (int idx = 0; idx < state->results.size(); ++idx) {
                const ToolResult& r = state->results[idx];
                QString body = r.content.trimmed();
                if (body.isEmpty()) {
                    body = r.success ? QStringLiteral("子代理完成（无输出）") : QStringLiteral("子代理失败（无错误详情）");
                }
                if (body.size() > 1000) {
                    body = body.left(1000) + QStringLiteral("\n...[已省略后续内容]...");
                }
                branchSummaries.append(QStringLiteral("分支%1(%2):\n%3")
                                           .arg(idx == 0 ? QStringLiteral("A") : QStringLiteral("B"),
                                                r.success ? QStringLiteral("OK") : QStringLiteral("ERR"),
                                                body));
            }

            m_messagesModel->addMessage(
                static_cast<int>(MessageRole::System),
                QStringLiteral("ToolCard|Observation|workflow_parallel|%1\n并行分支执行完成（2/2）")
                    .arg(state->hasFailure ? QStringLiteral("ERR") : QStringLiteral("OK")));
            m_sessionManager->updateMessages(m_messagesModel->messages());
            refreshSessions();

            const QString synthPrompt = QStringLiteral(
                "你是主Agent，整合并行子Agent结果，输出简洁结论。\n"
                "要求：\n"
                "1) 只输出【Plan】【Node Logs】【Final】三段；\n"
                "2) 不复述提示词；\n"
                "3) 若工具调用失败（如网络/API/权限），停止重试并给离线可执行替代方案；\n"
                "4) 标注失败原因与下一步最小行动。\n\n"
                "任务：%1\n"
                "分支结果：\n%2")
                .arg(normalizedTask,
                     branchSummaries.join(QStringLiteral("\n\n")));
            runAssistantWithInternalPrompt(synthPrompt);
        });

        QFuture<ToolResult> future = QtConcurrent::run([executor = m_agentService->toolExecutor(), call = calls[i]]() {
            return executor->execute(call);
        });
        watcher->setFuture(future);
    }
    return QStringLiteral("已启动并行工作流：%1").arg(workflowTemplateTitle(normalizedTemplate));
}

void QmlBackend::clearCurrentContext() {
    if (m_agentService && m_agentService->orchestrator()) {
        m_agentService->orchestrator()->stopActiveRun();
    } else if (m_agentService) {
        m_agentService->stopGeneration();
    }
    m_messagesModel->clear();
    m_streamingAssistantRow = -1;
    m_pendingAssistantChunk.clear();
    if (m_chunkFlushTimer) {
        m_chunkFlushTimer->stop();
    }
    m_lastApiContextTokens = 0;
    m_promptTokens = 0;
    m_completionTokens = 0;
    m_totalTokens = 0;
    m_usageText = QStringLiteral("本轮消耗(API)：0 Token");
    emit usageTextChanged();
    if (m_sessionManager && m_sessionManager->hasCurrentSession()) {
        m_sessionManager->updateMessages(MessageList());
    }
    setGenerating(false);
    setToolHintText(QString());
    m_statusText = QStringLiteral("已清空当前上下文");
    emit statusTextChanged();
    refreshSessions();
}

QString QmlBackend::compressCurrentContext() {
    if (!m_memoryManager) {
        const QString failed = QStringLiteral("压缩失败：内存管理器不可用");
        m_statusText = failed;
        emit statusTextChanged();
        return failed;
    }

    const bool compressed = m_memoryManager->forceCompress();
    const QString result = compressed
        ? QStringLiteral("已手动压缩当前上下文")
        : QStringLiteral("当前上下文较短，暂无可压缩内容");
    m_statusText = result;
    emit statusTextChanged();
    return result;
}

QString QmlBackend::exportCurrentSession() {
    if (!m_sessionManager || !m_sessionManager->hasCurrentSession()) {
        return QStringLiteral("导出失败：当前无会话");
    }
    const QString sessionId = m_sessionManager->currentSessionId();
    const QString exportRoot = workspacePath().isEmpty() ? QDir::currentPath() : workspacePath();
    QDir dir(exportRoot);
    dir.mkpath(QStringLiteral("exports"));
    const QString filePath = dir.filePath(QStringLiteral("exports/session_%1.json").arg(sessionId.left(8)));

    QString errorMessage;
    const bool ok = m_sessionManager->exportSession(sessionId, filePath, &errorMessage);
    const QString result = ok
        ? QStringLiteral("已导出会话：%1").arg(QDir::toNativeSeparators(filePath))
        : QStringLiteral("导出失败：%1").arg(errorMessage.isEmpty() ? QStringLiteral("未知错误") : errorMessage);

    m_statusText = result;
    emit statusTextChanged();
    return result;
}

QString QmlBackend::testProviderConfig(const QString& providerType,
                                       const QString& apiKey,
                                       const QString& baseUrl,
                                       const QString& model) {
    if (apiKey.trimmed().isEmpty()) {
        return QStringLiteral("API Key 不能为空");
    }
    if (baseUrl.trimmed().isEmpty()) {
        return QStringLiteral("Base URL 不能为空");
    }

    const QString normalizedType = providerType.trimmed().toLower().isEmpty()
        ? QStringLiteral("openai")
        : providerType.trimmed().toLower();

    ProviderConfig config = ConfigManager::instance().getProviderConfig();
    config.type = normalizedType;
    config.apiKey = apiKey.trimmed();
    config.baseUrl = baseUrl.trimmed();
    if (!model.trimmed().isEmpty()) {
        config.model = model.trimmed();
    }

    std::unique_ptr<ILLMProvider> provider;
    if (normalizedType == QStringLiteral("anthropic")) {
        provider = std::make_unique<AnthropicProvider>(config);
    } else if (normalizedType == QStringLiteral("gemini")) {
        provider = std::make_unique<GeminiProvider>(config);
    } else {
        provider = std::make_unique<OpenAIProvider>(config);
    }

    MessageList messages;
    messages.append(Message(MessageRole::User, QStringLiteral("ping")));

    ChatOptions options;
    options.model = config.model;
    options.maxTokens = 8;
    options.temperature = 0.0;
    options.stream = false;

    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);

    QString result;
    QObject::connect(&timeout, &QTimer::timeout, &loop, [&]() {
        result = QStringLiteral("API 测试超时");
        loop.quit();
    });

    timeout.start(12000);
    provider->chat(
        messages,
        options,
        [&](const QString&) {
            result = QStringLiteral("API 测试通过");
            loop.quit();
        },
        [&](const QString& error) {
            result = QStringLiteral("API 测试失败: %1").arg(error);
            loop.quit();
        });

    loop.exec();
    const QString finalResult = result.isEmpty() ? QStringLiteral("API 测试失败") : result;
    m_providerTestPassed = finalResult.startsWith(QStringLiteral("API 测试通过"));
    return finalResult;
}

QString QmlBackend::testExternalConfig(const QString& telegramToken,
                                       const QString& feishuAppId,
                                       const QString& feishuAppSecret) const {
    QStringList parts;

    QNetworkAccessManager manager;

    if (!telegramToken.trimmed().isEmpty()) {
        QNetworkRequest request(QUrl(QStringLiteral("https://api.telegram.org/bot%1/getMe").arg(telegramToken.trimmed())));
        QNetworkReply* reply = manager.get(request);

        QEventLoop loop;
        QTimer timeout;
        timeout.setSingleShot(true);
        QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        timeout.start(10000);
        loop.exec();

        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            parts.append(QStringLiteral("Telegram: 失败(%1)").arg(reply->errorString()));
        } else {
            const QJsonDocument doc = QJsonDocument::fromJson(data);
            const bool ok = doc.object().value(QStringLiteral("ok")).toBool(false);
            parts.append(ok ? QStringLiteral("Telegram: 通过") : QStringLiteral("Telegram: 失败(无效 token)"));
        }
        reply->deleteLater();
    } else {
        parts.append(QStringLiteral("Telegram: 未配置"));
    }

    if (!feishuAppId.trimmed().isEmpty() && !feishuAppSecret.trimmed().isEmpty()) {
        QNetworkRequest request(QUrl(QStringLiteral("https://open.feishu.cn/open-apis/auth/v3/tenant_access_token/internal")));
        request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

        QJsonObject payload;
        payload[QStringLiteral("app_id")] = feishuAppId.trimmed();
        payload[QStringLiteral("app_secret")] = feishuAppSecret.trimmed();

        QNetworkReply* reply = manager.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));

        QEventLoop loop;
        QTimer timeout;
        timeout.setSingleShot(true);
        QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        timeout.start(10000);
        loop.exec();

        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            parts.append(QStringLiteral("飞书: 失败(%1)").arg(reply->errorString()));
        } else {
            const QJsonDocument doc = QJsonDocument::fromJson(data);
            const int code = doc.object().value(QStringLiteral("code")).toInt(-1);
            parts.append(code == 0 ? QStringLiteral("飞书: 通过") : QStringLiteral("飞书: 失败(凭据无效)"));
        }
        reply->deleteLater();
    } else {
        parts.append(QStringLiteral("飞书: 未配置"));
    }

    return parts.join(QStringLiteral(" | "));
}

void QmlBackend::createSession(const QString& name) {
    const QString sessionId = m_sessionManager->createSession(name);
    if (!sessionId.isEmpty()) {
        m_sessionManager->switchSession(sessionId);
    }
    refreshSessions();
}

void QmlBackend::selectSession(const QString& sessionId) {
    if (sessionId.isEmpty()) {
        return;
    }

    m_sessionManager->switchSession(sessionId);
    syncCurrentSession();
}

void QmlBackend::refreshSessions() {
    m_sessionsModel->setSessions(m_sessionManager->listSessions(), m_sessionManager->currentSessionId());
}

void QmlBackend::importSession(const QUrl& fileUrl) {
    const QString filePath = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();
    if (filePath.isEmpty()) {
        return;
    }

    QString errorMessage;
    const QString importedId = m_sessionManager->importSession(filePath, &errorMessage);
    if (importedId.isEmpty()) {
        m_statusText = errorMessage.isEmpty() ? QStringLiteral("导入失败") : errorMessage;
        emit statusTextChanged();
        return;
    }

    m_sessionManager->switchSession(importedId);
    refreshSessions();
    syncCurrentSession();
}

void QmlBackend::renameCurrentSession(const QString& name) {
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty() || m_currentSessionId.isEmpty()) {
        return;
    }

    m_sessionManager->renameSession(m_currentSessionId, trimmed);
    refreshSessions();
    syncCurrentSession();
}

void QmlBackend::renameSessionById(const QString& sessionId, const QString& name) {
    const QString sid = sessionId.trimmed();
    const QString trimmed = name.trimmed();
    if (sid.isEmpty() || trimmed.isEmpty()) {
        return;
    }

    m_sessionManager->renameSession(sid, trimmed);
    refreshSessions();
    if (sid == m_currentSessionId) {
        syncCurrentSession();
    }
}

void QmlBackend::deleteCurrentSession() {
    if (m_currentSessionId.isEmpty()) {
        return;
    }

    const QString deletedId = m_currentSessionId;
    m_sessionManager->deleteSession(deletedId);
    refreshSessions();

    const QList<Session> sessions = m_sessionManager->listSessions();
    if (!sessions.isEmpty()) {
        m_sessionManager->switchSession(sessions.first().id);
    } else {
        const QString sessionId = m_sessionManager->createSession(QStringLiteral("新聊天"));
        m_sessionManager->switchSession(sessionId);
    }

    syncCurrentSession();
}

void QmlBackend::deleteSessionById(const QString& sessionId) {
    if (sessionId.isEmpty()) {
        return;
    }

    bool exists = false;
    const QList<Session> sessions = m_sessionManager->listSessions();
    for (const Session& session : sessions) {
        if (session.id == sessionId) {
            exists = true;
            break;
        }
    }
    if (!exists) {
        return;
    }

    if (sessionId != m_currentSessionId) {
        m_sessionManager->switchSession(sessionId);
        syncCurrentSession();
    }

    deleteCurrentSession();
}

int QmlBackend::deleteSessionsByIds(const QVariantList& sessionIds) {
    QSet<QString> targetIds;
    for (const QVariant& value : sessionIds) {
        const QString id = value.toString().trimmed();
        if (!id.isEmpty()) {
            targetIds.insert(id);
        }
    }
    if (targetIds.isEmpty()) {
        return 0;
    }

    int deleted = 0;
    for (const QString& id : std::as_const(targetIds)) {
        const QList<Session> sessions = m_sessionManager->listSessions();
        for (const Session& session : sessions) {
            if (session.id == id) {
                m_sessionManager->deleteSession(id);
                ++deleted;
                break;
            }
        }
    }

    refreshSessions();
    const QList<Session> remaining = m_sessionManager->listSessions();
    if (remaining.isEmpty()) {
        const QString sid = m_sessionManager->createSession(QStringLiteral("新聊天"));
        m_sessionManager->switchSession(sid);
    } else if (m_sessionManager->currentSessionId().isEmpty()) {
        m_sessionManager->switchSession(remaining.first().id);
    }
    syncCurrentSession();
    return deleted;
}

int QmlBackend::deleteUnpinnedSessions() {
    const QList<Session> sessions = m_sessionManager->listSessions();
    QStringList toDelete;
    for (const Session& session : sessions) {
        if (!session.isPinned) {
            toDelete.append(session.id);
        }
    }

    for (const QString& sid : toDelete) {
        m_sessionManager->deleteSession(sid);
    }

    refreshSessions();
    const QList<Session> remaining = m_sessionManager->listSessions();
    if (remaining.isEmpty()) {
        const QString sessionId = m_sessionManager->createSession(QStringLiteral("新聊天"));
        m_sessionManager->switchSession(sessionId);
    } else if (m_sessionManager->currentSessionId().isEmpty()) {
        m_sessionManager->switchSession(remaining.first().id);
    }

    syncCurrentSession();
    return toDelete.size();
}

void QmlBackend::togglePinCurrentSession() {
    if (m_currentSessionId.isEmpty()) {
        return;
    }

    const Session session = m_sessionManager->currentSession();
    m_sessionManager->setPinned(m_currentSessionId, !session.isPinned);
    refreshSessions();
    syncCurrentSession();
}

void QmlBackend::togglePinSessionById(const QString& sessionId) {
    const QString sid = sessionId.trimmed();
    if (sid.isEmpty()) {
        return;
    }

    const QList<Session> sessions = m_sessionManager->listSessions();
    for (const Session& session : sessions) {
        if (session.id == sid) {
            m_sessionManager->setPinned(sid, !session.isPinned);
            refreshSessions();
            if (sid == m_currentSessionId) {
                syncCurrentSession();
            }
            break;
        }
    }
}

void QmlBackend::sendMessage(const QString& content) {
    if (m_generating && m_streamingAssistantRow < 0) {
        setGenerating(false);
    }
    if (m_generating) {
        m_statusText = QStringLiteral("上一条回复仍在生成中，请先等待完成或点击停止");
        emit statusTextChanged();
        return;
    }
    const QString trimmed = content.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }

    syncCompressionThresholdFromModel();
    ensureSessionExists();
    if (m_currentSessionId.isEmpty()) {
        syncCurrentSession();
    }

    if (m_messagesModel->rowCount() == 0) {
        m_streamingAssistantRow = -1;
    }

    m_messagesModel->addMessage(static_cast<int>(MessageRole::User), trimmed);
    m_streamingAssistantRow = m_messagesModel->addMessage(static_cast<int>(MessageRole::Assistant), QString());
    m_pendingAssistantChunk.clear();
    if (m_chunkFlushTimer) {
        m_chunkFlushTimer->stop();
    }
    m_sessionManager->updateMessages(m_messagesModel->messages());
    refreshSessions();

    setGenerating(true);
    m_promptTokens = 0;
    m_completionTokens = 0;
    m_totalTokens = 0;
    m_usageDirtyCurrentRun = false;
    m_usageText = QStringLiteral("本轮消耗(API)：0 Token");
    emit usageTextChanged();
    m_statusText = QStringLiteral("生成中");
    emit statusTextChanged();
    setToolHintText(QString());

    if (m_agentService && m_agentService->orchestrator()) {
        m_agentService->orchestrator()->sendUserInput(trimmed);
    } else {
        m_agentService->sendMessage(trimmed);
    }
}

void QmlBackend::deleteMessageAt(int row) {
    const MessageList currentMessages = m_messagesModel->messages();
    if (row < 0 || row >= currentMessages.size()) {
        return;
    }

    if (!m_messagesModel->removeMessageAt(row)) {
        return;
    }

    if (m_streamingAssistantRow == row) {
        m_streamingAssistantRow = -1;
    } else if (m_streamingAssistantRow > row) {
        --m_streamingAssistantRow;
    }

    m_sessionManager->updateMessages(m_messagesModel->messages());
    refreshSessions();
}

void QmlBackend::stopGeneration() {
    if (m_agentService && m_agentService->orchestrator()) {
        m_agentService->orchestrator()->stopActiveRun();
    } else {
        m_agentService->stopGeneration();
    }
}

} // namespace clawpp
