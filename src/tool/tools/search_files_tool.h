#ifndef CLAWPP_TOOL_TOOLS_SEARCH_FILES_TOOL_H
#define CLAWPP_TOOL_TOOLS_SEARCH_FILES_TOOL_H

#include "tool/i_tool.h"

namespace clawpp {

/**
 * @class SearchFilesTool
 * @brief 按 glob 模式搜索文件，支持递归遍历。
 */
class SearchFilesTool : public ITool {
    Q_OBJECT

public:
    explicit SearchFilesTool(QObject* parent = nullptr);
    
    QString name() const override;
    QString description() const override;
    QJsonObject parameters() const override;
    PermissionLevel permissionLevel() const override;
    ToolResult execute(const QJsonObject& args) override;

private:
    /// 递归实现：将命中文件追加到 results 与 listing。
    void searchRecursive(const QString& path, const QString& pattern, bool recursive, QJsonArray& results, QStringList& listing);
};

}

#endif
