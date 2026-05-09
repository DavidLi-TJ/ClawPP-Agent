#ifndef CLAWPP_TOOL_TOOLS_SUBAGENT_TOOL_H
#define CLAWPP_TOOL_TOOLS_SUBAGENT_TOOL_H

#include "tool/i_tool.h"

namespace clawpp {

/**
 * @class SubagentTool
 * @brief 子代理声明工具，实际执行由 ToolExecutor 接管。
 */
class SubagentTool : public ITool {
    Q_OBJECT

public:
    explicit SubagentTool(QObject* parent = nullptr);
    
    QString name() const override;
    QString description() const override;
    QJsonObject parameters() const override;
    PermissionLevel permissionLevel() const override;
    bool canRunInParallel() const override;
    ToolResult execute(const QJsonObject& args) override;
};

}

#endif
