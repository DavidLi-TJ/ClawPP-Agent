#ifndef CLAWPP_TOOL_TOOLS_SYSTEM_TOOL_H
#define CLAWPP_TOOL_TOOLS_SYSTEM_TOOL_H

#include "tool/i_tool.h"

namespace clawpp {

/**
 * @class SystemTool
 * @brief 提供系统信息、时间和环境变量相关操作。
 */
class SystemTool : public ITool {
    Q_OBJECT

public:
    explicit SystemTool(QObject* parent = nullptr);
    
    QString name() const override;
    QString description() const override;
    QJsonObject parameters() const override;
    PermissionLevel permissionLevel() const override;
    ToolResult execute(const QJsonObject& args) override;

private:
    ToolResult getEnv(const QString& name);
    ToolResult setEnv(const QString& name, const QString& value);
    ToolResult getCurrentTime(const QString& format);
    ToolResult getSystemInfo();
};

}

#endif
