#ifndef CLAWPP_APPLICATION_AGENT_SERVICE_H
#define CLAWPP_APPLICATION_AGENT_SERVICE_H

#include <QObject>
#include <QMutex>
#include "common/types.h"
#include "provider/i_llm_provider.h"
#include "memory/i_memory_system.h"
#include "tool/tool_registry.h"
#include "tool/tool_executor.h"
#include "permission/permission_manager.h"

namespace clawpp {

class TemplateLoader;
class ReactAgentCore;
class SessionThread;
class AgentOrchestrator;

/**
 * @class AgentService
 * @brief Agent 服务核心类
 * 
 * 负责管理会话、处理消息、调用 LLM、执行工具
 * 是连接 UI 和底层 Agent 核心的桥梁
 * 
 * 主要功能：
 * - 会话管理（开始/结束会话）
 * - 消息处理（发送/接收）
 * - 系统提示词构建
 * - 流式响应处理
 * - 工具调用协调
 */
class AgentService : public QObject {
    Q_OBJECT

public:
    explicit AgentService(QObject* parent = nullptr);
    ~AgentService();
    
    void setProvider(ILLMProvider* provider);              ///< 设置 LLM Provider
    void setProviderConfig(const ProviderConfig& config);   ///< 设置 Provider 配置
    void setMemory(IMemorySystem* memory);                 ///< 设置内存系统
    void setToolRegistry(ToolRegistry* registry);          ///< 设置工具注册表
    void setPermissionManager(PermissionManager* manager); ///< 设置权限管理器
    void setTemplateLoader(TemplateLoader* loader);        ///< 设置模板加载器
    void setWorkspaceRoot(const QString& workspaceRoot);   ///< 设置工作区根目录
    ToolExecutor* toolExecutor() const;
    AgentOrchestrator* orchestrator() const;
    
    void startSession(const QString& sessionId, const QString& name = QString());  ///< 开始会话
    void endSession();  ///< 结束会话
    void sendMessage(const QString& content);  ///< 发送消息
    void stopGeneration();  ///< 停止生成
    void setSystemPrompt(const QString& prompt);  ///< 设置系统提示词
    void buildSystemPromptFromTemplates();  ///< 从模板构建系统提示词
    void setHistory(const MessageList& history);  ///< 设置消息历史
    
    QString currentSessionId() const;  ///< 获取当前会话 ID
    MessageList messageHistory() const;  ///< 获取消息历史
    bool isRunning() const;  ///< 当前是否仍在处理请求

signals:
    void sessionStarted(const QString& sessionId);         ///< 会话开始信号
    void sessionEnded();                                    ///< 会话结束信号
    void messageSent(const QString& content);              ///< 消息发送信号
    void responseChunk(const StreamChunk& chunk);          ///< 响应片段信号
    void responseComplete(const QString& fullResponse);    ///< 响应完成信号
    void toolCallRequested(const ToolCall& call);          ///< 工具调用请求信号
    void toolCallCompleted(const ToolCall& call, const ToolResult& result);  ///< 工具调用完成信号
    void conversationChanged(const MessageList& messages);  ///< 会话消息变化信号
    void errorOccurred(const QString& error);              ///< 错误发生信号
    void generationStopped();                               ///< 生成停止信号
    void usageReport(int promptTokens, int completionTokens, int totalTokens);  ///< 使用量报告信号
    void toolCallParsed(const ToolCall& call);                 ///< 流式解析到完整工具调用
    void toolPreExecutionStarted(const QString& toolId);       ///< 工具预执行开始
    void toolPreExecutionCompleted(const QString& toolId);     ///< 工具预执行完成

private:
    void syncCoreState();  ///< 同步核心状态
    void connectToolExecutorSignals(); ///< 连接工具执行器信号
    
    ILLMProvider* m_provider;              ///< LLM Provider（外部管理）
    IMemorySystem* m_memory;               ///< 内存系统（外部管理）
    ToolRegistry* m_toolRegistry;          ///< 工具注册表（外部管理）
    ToolExecutor* m_toolExecutor;          ///< 工具执行器（Qt 管理）
    PermissionManager* m_permissionManager;///< 权限管理器（外部管理）
    TemplateLoader* m_templateLoader;      ///< 模板加载器（外部管理）
    ReactAgentCore* m_agentCore;           ///< 实际 Agent 核心（Qt 管理）
    SessionThread* m_sessionThread;        ///< 会话工作线程（Qt 管理）
    AgentOrchestrator* m_orchestrator;     ///< 轻量编排门面（Qt 管理）
    
    ProviderConfig m_providerConfig;       ///< Provider 配置
    QString m_systemPrompt;                ///< 系统提示词
    QString m_workspaceRoot;               ///< 工作区根目录
    
    QString m_currentSessionId;            ///< 当前会话 ID
    QString m_sessionName;                 ///< 会话名称
    MessageList m_messageHistory;          ///< 消息历史
    QString m_currentResponse;             ///< 当前响应
    QStringList m_runtimeToolNames;        ///< 本轮可用工具白名单
    quint64 m_runGeneration;               ///< 运行代际标识，用于屏蔽过期 watchdog
    
    bool m_isRunning;                      ///< 是否正在运行
    mutable QMutex m_mutex;                ///< 互斥锁
};

}

#endif
