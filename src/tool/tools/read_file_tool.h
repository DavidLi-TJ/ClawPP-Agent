#ifndef CLAWPP_TOOL_TOOLS_READ_FILE_TOOL_H
#define CLAWPP_TOOL_TOOLS_READ_FILE_TOOL_H

#include "tool/i_tool.h"

namespace clawpp {

/**
 * @class ReadFileTool
 * @brief 读取文本文件内容并返回基础元数据。
 */
class ReadFileTool : public ITool {
    Q_OBJECT

public:
    explicit ReadFileTool(QObject* parent = nullptr);
    
    QString name() const override;
    QString description() const override;
    QJsonObject parameters() const override;
    PermissionLevel permissionLevel() const override;
    ToolResult execute(const QJsonObject& args) override;
};

}

#endif
