#ifndef CLAWPP_TOOL_TOOLS_EDIT_FILE_TOOL_H
#define CLAWPP_TOOL_TOOLS_EDIT_FILE_TOOL_H

#include "tool/i_tool.h"

namespace clawpp {

/**
 * @class EditFileTool
 * @brief 在已有文件中执行文本替换，适合小范围精准修改。
 */
class EditFileTool : public ITool {
    Q_OBJECT

public:
    explicit EditFileTool(QObject* parent = nullptr);
    
    QString name() const override;
    QString description() const override;
    QJsonObject parameters() const override;
    PermissionLevel permissionLevel() const override;
    ToolResult execute(const QJsonObject& args) override;

private:
    /// 执行字符串替换，当前实现默认替换所有匹配项。
    QString applyEdit(const QString& content, const QString& oldStr, const QString& newStr);
};

}

#endif
