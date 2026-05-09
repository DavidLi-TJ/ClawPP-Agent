#ifndef CLAWPP_TOOL_TOOLS_LIST_DIRECTORY_TOOL_H
#define CLAWPP_TOOL_TOOLS_LIST_DIRECTORY_TOOL_H

#include "tool/i_tool.h"

namespace clawpp {

/**
 * @class ListDirectoryTool
 * @brief 列出目录内容及文件元信息。
 */
class ListDirectoryTool : public ITool {
    Q_OBJECT

public:
    explicit ListDirectoryTool(QObject* parent = nullptr);
    
    QString name() const override;
    QString description() const override;
    QJsonObject parameters() const override;
    PermissionLevel permissionLevel() const override;
    ToolResult execute(const QJsonObject& args) override;
};

}

#endif
