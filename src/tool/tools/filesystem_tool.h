#ifndef CLAWPP_TOOL_TOOLS_FILESYSTEM_TOOL_H
#define CLAWPP_TOOL_TOOLS_FILESYSTEM_TOOL_H

#include "tool/i_tool.h"

namespace clawpp {

/**
 * @class FilesystemTool
 * @brief 文件系统工具
 * 
 * 提供文件读写、目录操作等文件系统功能
 * 支持的安全操作：read, write, list, mkdir, delete, copy, move, search
 */
class FilesystemTool : public ITool {
    Q_OBJECT

public:
    explicit FilesystemTool(QObject* parent = nullptr);
    
    QString name() const override;              ///< 工具名称
    QString description() const override;       ///< 工具描述
    QJsonObject parameters() const override;    ///< 参数定义
    PermissionLevel permissionLevel() const override;  ///< 权限级别
    ToolResult execute(const QJsonObject& args) override;  ///< 执行工具

private:
    ToolResult readFile(const QString& path);           ///< 读取文件
    ToolResult writeFile(const QString& path, const QString& content);  ///< 写入文件
    ToolResult listDirectory(const QString& path);      ///< 列出目录
    ToolResult createDirectory(const QString& path);    ///< 创建目录
    ToolResult deleteFile(const QString& path);         ///< 删除文件
    ToolResult copyFile(const QString& source, const QString& destination);  ///< 复制文件
    ToolResult moveFile(const QString& source, const QString& destination);  ///< 移动文件
    ToolResult searchFiles(const QString& path, const QString& pattern, bool recursive);  ///< 搜索文件
};

}

#endif
