#ifndef CLAWPP_TOOL_TOOL_EXECUTOR_H
#define CLAWPP_TOOL_TOOL_EXECUTOR_H

#include <QObject>
#include <functional>
#include "common/types.h"

namespace clawpp {

class ToolRegistry;
class PermissionManager;
class ILLMProvider;

/**
 * @class ToolExecutor
 * @brief 工具执行入口，封装权限校验、异常防护与子代理执行。
 *
 * 注意：当前 NeedConfirmation 走同步等待（QEventLoop），
 * 后续可演进为全异步状态机以避免潜在阻塞。
 */
class ToolExecutor : public QObject {
    Q_OBJECT

public:
    explicit ToolExecutor(ToolRegistry* registry = nullptr,
                          PermissionManager* permissionManager = nullptr,
                          QObject* parent = nullptr);
    
    void setRegistry(ToolRegistry* registry);         ///< 设置工具注册表
    void setPermissionManager(PermissionManager* manager);  ///< 设置权限管理器
    void setProvider(ILLMProvider* provider);
    void setProviderConfig(const ProviderConfig& config);
    void setSessionId(const QString& sessionId);
    QString sessionId() const;
    void setAutoApproveConfirmations(bool enabled);
    
    /**
     * @brief 执行工具调用
     * @param toolCall 工具调用信息
     * @return 执行结果
     */
    ToolResult execute(const ToolCall& toolCall);
    
    /**
     * @brief 批量执行工具调用（支持并行）
     * @param toolCalls 工具调用列表
     * @return 执行结果列表（顺序与输入一致）
     * 
     * 自动将 Safe 工具并行执行，Risky 工具串行执行
     */
    QVector<ToolResult> executeBatch(const QVector<ToolCall>& toolCalls);

signals:
    void permissionRequest(const ToolCall& toolCall, std::function<void(bool)> callback);  ///< 权限请求信号
    void toolCompleted(const ToolCall& toolCall, const ToolResult& result);                  ///< 工具完成信号

private:
    ToolResult executeSubagent(const ToolCall& toolCall);
    QString buildSubagentPrompt(const QJsonObject& args) const;
    void applyOutputLimit(ToolResult& result, const ToolCall& toolCall) const;
    QString buildOutputSummary(const QString& content, int maxChars) const;

    ToolRegistry* m_registry;              // 外部管理，不拥有所有权
    PermissionManager* m_permissionManager;// 外部管理，不拥有所有权
    ILLMProvider* m_provider;               // 外部管理，不拥有所有权
    ProviderConfig m_providerConfig;
    QString m_sessionId;
    bool m_autoApproveConfirmations;
};

}

#endif
