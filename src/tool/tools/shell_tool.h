#ifndef CLAWPP_TOOL_TOOLS_SHELL_TOOL_H
#define CLAWPP_TOOL_TOOLS_SHELL_TOOL_H

#include "tool/i_tool.h"
#include <QStringList>

namespace clawpp {

/**
 * @class ShellTool
 * @brief Shell 命令执行工具
 * 
 * 提供安全的 shell 命令执行功能
 * 包含命令过滤、路径限制、超时控制等安全机制
 */
class ShellTool : public ITool {
    Q_OBJECT

public:
    explicit ShellTool(QObject* parent = nullptr);
    
    QString name() const override;              ///< 工具名称
    QString description() const override;       ///< 工具描述
    QJsonObject parameters() const override;    ///< 参数定义
    PermissionLevel permissionLevel() const override;  ///< 权限级别
    ToolResult execute(const QJsonObject& args) override;  ///< 执行工具
    
    void setWorkingDirectory(const QString& dir);      ///< 设置工作目录
    void setRestrictToWorkspace(bool restrict);        ///< 限制到工作区
    void setDefaultTimeout(int timeoutMs);             ///< 设置默认超时
    void addBlockedCommand(const QString& pattern);    ///< 添加禁止的命令
    void clearBlockedCommands();                       ///< 清空禁止列表

private:
    bool isCommandBlocked(const QString& command) const;     ///< 检查命令是否被禁止
    bool isPathAllowed(const QString& path) const;           ///< 检查路径是否允许
    QString sanitizeCommand(const QString& command) const;   ///< 清理命令
    QString adaptCommandForPlatform(const QString& command) const; ///< 兼容当前平台 shell 语法

    QString m_workingDirectory;     ///< 工作目录
    bool m_restrictToWorkspace;     ///< 是否限制到工作区
    int m_defaultTimeout;           ///< 默认超时时间
    QStringList m_blockedPatterns;  ///< 禁止的路径模式
    QStringList m_blockedCommands;  ///< 禁止的命令列表
};

}

#endif
