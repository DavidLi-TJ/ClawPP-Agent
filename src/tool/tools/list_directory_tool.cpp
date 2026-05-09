#include "list_directory_tool.h"
#include <QDir>
#include <QFileInfo>

namespace clawpp {

ListDirectoryTool::ListDirectoryTool(QObject* parent)
    : ITool(parent) {}

QString ListDirectoryTool::name() const {
    return "list_directory";
}

QString ListDirectoryTool::description() const {
    return "List the contents of a directory. Returns a list of files and subdirectories with their metadata.";
}

QJsonObject ListDirectoryTool::parameters() const {
    QJsonObject params;
    params["type"] = "object";
    
    QJsonObject properties;
    
    QJsonObject path;
    path["type"] = "string";
    path["description"] = "The path to the directory to list";
    properties["path"] = path;
    
    params["properties"] = properties;
    params["required"] = QJsonArray{"path"};
    
    return params;
}

PermissionLevel ListDirectoryTool::permissionLevel() const {
    return PermissionLevel::Safe;
}

ToolResult ListDirectoryTool::execute(const QJsonObject& args) {
    ToolResult result;
    
    QString path = args.value("path").toString().trimmed();
    if (path.isEmpty()) {
        path = args.value("directory").toString().trimmed();
    }
    if (path.isEmpty()) {
        path = args.value("dir").toString().trimmed();
    }
    if (path.isEmpty()) {
        path = args.value("folder").toString().trimmed();
    }

    const bool defaultedPath = path.isEmpty();
    if (path.isEmpty()) {
        path = QDir::currentPath();
    }
    
    QDir dir(path);
    if (!dir.exists()) {
        result.success = false;
        result.content = QString("Error: Directory does not exist: %1").arg(path);
        return result;
    }
    
    QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
    
    QJsonArray items;
    QStringList listing;
    
    for (const QFileInfo& info : entries) {
        QJsonObject item;
        item["name"] = info.fileName();
        item["path"] = info.absoluteFilePath();
        item["is_dir"] = info.isDir();
        item["is_file"] = info.isFile();
        item["size"] = info.size();
        item["modified"] = info.lastModified().toString(Qt::ISODate);
        items.append(item);
        
        QString type = info.isDir() ? "[DIR]" : "[FILE]";
        listing.append(QString("%1 %2 (%3 bytes)").arg(type, info.fileName()).arg(info.size()));
    }
    
    result.success = true;
    result.content = listing.isEmpty()
        ? QString("Directory listing for %1:\n[empty directory]").arg(path)
        : QString("Directory listing for %1:\n%2").arg(path, listing.join("\n"));
    result.metadata["items"] = items;
    result.metadata["count"] = entries.size();
    if (defaultedPath) {
        result.metadata["path_defaulted"] = true;
    }
    
    return result;
}

}
