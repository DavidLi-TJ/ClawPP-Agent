#ifndef CLAWPP_TOOL_TOOLS_WRITE_FILE_TOOL_H
#define CLAWPP_TOOL_TOOLS_WRITE_FILE_TOOL_H

#include "tool/i_tool.h"

namespace clawpp {

/**
 * @class WriteFileTool
 * @brief 负责将文本内容写入指定文件路径。
 */
class WriteFileTool : public ITool {
    Q_OBJECT

public:
    explicit WriteFileTool(QObject* parent = nullptr);

    QString name() const override;
    QString description() const override;
    QJsonObject parameters() const override;
    PermissionLevel permissionLevel() const override;
    ToolResult execute(const QJsonObject& args) override;
};

}

#endif
