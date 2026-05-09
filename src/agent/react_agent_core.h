#ifndef CLAWPP_AGENT_REACT_AGENT_CORE_H
#define CLAWPP_AGENT_REACT_AGENT_CORE_H

#include <QObject>
#include <QTimer>
#include <QUuid>
#include <QRegularExpression>
#include <QList>
#include <QMap>
#include <QFuture>
#include <QSet>
#include <QHash>
#include "i_agent_core.h"
#include "provider/i_llm_provider.h"
#include "tool/tool_executor.h"
#include "memory/i_memory_system.h"
#include "common/constants.h"

namespace clawpp {

/**
 * @enum ContinueReason
 * @brief Agent 循环继续/终止原因
 * 
 * 7种不同的继续策略，每种对应一种恢复路径：
 * - NormalCompletion: 正常完成，有最终答案
 * - MaxIterationsReached: 达到最大迭代次数
 * - NoToolStreak: 连续多轮无工具调用
 * - ContextOverflow: 上下文窗口超限
 * - ToolExecutionFailure: 工具执行失败
 * - UserStop: 用户主动停止
 * - StreamError: 流式请求错误
 */
enum class ContinueReason {
    NormalCompletion,         ///< 正常完成，生成最终答案
    MaxIterationsReached,     ///< 达到最大迭代次数
    NoToolStreak,             ///< 连续无工具调用轮次过多
    ContextOverflow,          ///< 上下文窗口超限，需压缩
    ToolExecutionFailure,     ///< 工具执行失败，需恢复
    UserStop,                 ///< 用户主动停止
    StreamError               ///< 流式请求失败，需重试
};

struct CorePendingToolCall {
    QString id;
    QString name;
    QString arguments;
};

/**
 * @class ReactAgentCore
 * @brief ReAct 循环核心实现：推理 -> 工具调用 -> 观察 -> 继续迭代。
 */
class ReactAgentCore : public IAgentCore {
    Q_OBJECT

public:
    explicit ReactAgentCore(QObject* parent = nullptr);
    
    void setProvider(ILLMProvider* provider);
    void setMemory(IMemorySystem* memory);
    void setExecutor(ToolExecutor* executor);
    void setProviderConfig(const ProviderConfig& config);
    void setRuntimeToolNames(const QStringList& toolNames);
    void run(const QString& input) override;
    void stop() override;
    void setContext(const MessageList& messages) override;
    void setSystemPrompt(const QString& prompt) override;

signals:
    void conversationUpdated(const MessageList& messages);
    void usageReport(int promptTokens, int completionTokens, int totalTokens);
    void toolCallParsed(const ToolCall& toolCall);           ///< 工具调用解析完成（可能在流式中）
    void toolExecutionStarted(const QString& toolId);        ///< 工具开始预执行
    void toolPreExecutionComplete(const QString& toolId);     ///< 工具预执行完成

private:
    void processIteration();
    MessageList buildIterationMessages() const;
    bool finalizeIfStoppedOrExceeded();
    void handleStreamChunk(const StreamChunk& chunk, quint64 runGeneration);
    void handleStreamCompleted(quint64 runGeneration);
    void handleStreamError(const QString& error, quint64 runGeneration);
    void processThought();
    void executeToolCalls();
    bool shouldTerminateByNoToolStreak() const;
    bool shouldContinueForInterimContent(const QString& text) const;
    bool isLikelyTruncatedAssistantTurn(const QString& text) const;
    bool hasFinalAnswerMarker(const QString& text) const;
    void finalizeResponse(const QString& finalMessage);
    
    // 流式工具预执行
    bool isToolCallComplete(const CorePendingToolCall& pending) const;
    ToolCall createToolCall(const CorePendingToolCall& pending) const;
    void startToolPreExecution();
    void checkPreExecutionResults();
    
    // 四级渐进式压缩
    void compressMessagesForContext(MessageList& messages) const;
    int performLevel1Trim(MessageList& messages) const;
    int performLevel2Dedupe(MessageList& messages) const;
    int performLevel3Fold(MessageList& messages) const;
    void performLevel4Summarize(MessageList& messages) const;
    
    int estimateTokens(const QString& text) const;
    QString buildCompressedSummary(const MessageList& messages, int start, int end) const;
    
    // 7 种继续策略相关方法
    ContinueReason evaluateContinueReason();
    void handleContinueReason(ContinueReason reason);
    bool canRetryAfterError() const;
    void appendRecoveryMessage(const QString& message);
    int estimateContextTokens() const;

    ILLMProvider* m_provider;       // 外部管理，不拥有所有权
    IMemorySystem* m_memory;        // 外部管理，不拥有所有权
    ToolExecutor* m_executor;       // 外部管理，不拥有所有权
    ProviderConfig m_providerConfig;
    MessageList m_context;
    MessageList m_messages;
    QString m_systemPrompt;
    QString m_currentThought;
    QString m_currentContent;
    QStringList m_runtimeToolNames;
    QList<CorePendingToolCall> m_pendingToolCalls;
    QMap<QString, QFuture<ToolResult>> m_preExecutingTools;  // 预执行中的工具 futures
    QMap<QString, ToolResult> m_preExecutedResults;          // 预执行完成结果缓存
    QSet<QString> m_parsedToolIds;                            // 已通知 UI 的工具 ID
    QHash<QString, int> m_toolCallExecutionCounts;            // 工具签名执行次数（防止空转循环）
    quint64 m_runGeneration;
    int m_iteration;
    int m_noToolIterationStreak;
    int m_retryCount;              // 错误重试计数
    int m_lastContextTokens;       // 上次上下文 token 数
    int m_consecutiveToolFailureRounds; // 连续“整轮工具全部失败”次数
    bool m_stopped;
    bool m_hadToolFailure;         // 本轮是否有工具失败
};

}

#endif
