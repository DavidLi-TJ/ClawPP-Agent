#include "agent_service.h"
#include "template_loader.h"
#include "agent/react_agent_core.h"
#include "agent/context_builder.h"
#include "application/session_thread.h"
#include "application/agent_orchestrator.h"
#include <QDir>
#include <QMetaObject>
#include <QThread>

namespace clawpp {

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
        emit conversationChanged(messages);
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
    m_systemPrompt = ContextBuilder::instance().buildSystemPromptForInput(content);
    m_runtimeToolNames.clear();
    for (const QJsonValue& value : matchedSkill.tools) {
        const QString toolName = value.toString().trimmed();
        if (!toolName.isEmpty() && !m_runtimeToolNames.contains(toolName)) {
            m_runtimeToolNames.append(toolName);
        }
    }

    syncCoreState();
    m_isRunning = true;
    const quint64 runGeneration = ++m_runGeneration;
    // runtime context 只携带元数据（例如渠道、会话号），不直接覆盖用户原始输入。
    QString runtimeContext = ContextBuilder::instance().buildRuntimeContext(QStringLiteral("gui"), m_currentSessionId);
    QString payload = runtimeContext.isEmpty() ? content : QString("%1\n\n%2").arg(runtimeContext, content);
    if (m_agentCore) {
        QMetaObject::invokeMethod(m_agentCore, [this, payload]() {
            m_agentCore->run(payload);
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

ToolExecutor* AgentService::toolExecutor() const {
    return m_toolExecutor;
}

AgentOrchestrator* AgentService::orchestrator() const {
    return m_orchestrator;
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
    {
        QMutexLocker locker(&m_mutex);
        historySnapshot = m_messageHistory;
        systemPromptSnapshot = m_systemPrompt;
        runtimeToolNamesSnapshot = m_runtimeToolNames;
        providerConfigSnapshot = m_providerConfig;
        currentSessionIdSnapshot = m_currentSessionId;
    }

    ILLMProvider* providerSnapshot = m_provider;
    IMemorySystem* memorySnapshot = m_memory;
    ToolExecutor* executorSnapshot = m_toolExecutor;

    QMetaObject::invokeMethod(m_agentCore, [this,
                                            providerSnapshot,
                                            memorySnapshot,
                                            executorSnapshot,
                                            providerConfigSnapshot,
                                            historySnapshot,
                                            systemPromptSnapshot,
                                            runtimeToolNamesSnapshot]() {
        if (!m_agentCore) {
            return;
        }

        m_agentCore->setProvider(providerSnapshot);
        m_agentCore->setMemory(memorySnapshot);
        m_agentCore->setExecutor(executorSnapshot);
        m_agentCore->setProviderConfig(providerConfigSnapshot);
        m_agentCore->setContext(historySnapshot);
        m_agentCore->setSystemPrompt(systemPromptSnapshot);
        m_agentCore->setRuntimeToolNames(runtimeToolNamesSnapshot);
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
