#include "search_files_tool.h"
#include <QDir>
#include <QFileInfo>

namespace clawpp {

SearchFilesTool::SearchFilesTool(QObject* parent)
    : ITool(parent) {}

QString SearchFilesTool::name() const {
    return "search_files";
}

QString SearchFilesTool::description() const {
    return "Search for files matching a pattern in a directory. Supports glob patterns like *.cpp or *.h";
}

QJsonObject SearchFilesTool::parameters() const {
    QJsonObject params;
    params["type"] = "object";
    
    QJsonObject properties;
    
    QJsonObject path;
    path["type"] = "string";
    path["description"] = "The directory to search in";
    properties["path"] = path;
    
    QJsonObject pattern;
    pattern["type"] = "string";
    pattern["description"] = "Glob pattern to match files (e.g., *.cpp, *.h, *.txt)";
    properties["pattern"] = pattern;
    
    QJsonObject recursive;
    recursive["type"] = "boolean";
    recursive["description"] = "Whether to search recursively in subdirectories";
    properties["recursive"] = recursive;
    
    params["properties"] = properties;
    params["required"] = QJsonArray{"path", "pattern"};
    
    return params;
}

PermissionLevel SearchFilesTool::permissionLevel() const {
    return PermissionLevel::Safe;
}

ToolResult SearchFilesTool::execute(const QJsonObject& args) {
    ToolResult result;
    
    QString path = args["path"].toString();
    QString pattern = args["pattern"].toString();
    bool recursive = args["recursive"].toBool(true);
    
    if (path.isEmpty()) {
        result.success = false;
        result.content = "Error: path parameter is required";
        return result;
    }
    
    if (pattern.isEmpty()) {
        result.success = false;
        result.content = "Error: pattern parameter is required";
        return result;
    }
    
    QDir dir(path);
    if (!dir.exists()) {
        result.success = false;
        result.content = QString("Error: Directory does not exist: %1").arg(path);
        return result;
    }
    
    QJsonArray items;
    QStringList listing;
    
    searchRecursive(path, pattern, recursive, items, listing);
    
    result.success = true;
    result.content = QString("Found %1 files matching '%2' in %3:\n%4")
        .arg(items.size()).arg(pattern, path, listing.join("\n"));
    result.metadata["items"] = items;
    result.metadata["count"] = items.size();
    
    return result;
}

void SearchFilesTool::searchRecursive(const QString& path, const QString& pattern, bool recursive, QJsonArray& results, QStringList& listing) {
    QDir dir(path);
    
    QStringList nameFilters;
    nameFilters << pattern;
    
    QFileInfoList fileEntries = dir.entryInfoList(nameFilters, QDir::Files | QDir::NoDotAndDotDot);
    
    for (const QFileInfo& info : fileEntries) {
        QJsonObject item;
        item["name"] = info.fileName();
        item["path"] = info.absoluteFilePath();
        item["size"] = info.size();
        item["modified"] = info.lastModified().toString(Qt::ISODate);
        results.append(item);
        listing.append(info.absoluteFilePath());
    }
    
    if (recursive) {
        QFileInfoList dirEntries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo& subDir : dirEntries) {
            // 避免符号链接目录造成环形递归。
            if (subDir.isSymLink()) {
                continue;
            }
            searchRecursive(subDir.absoluteFilePath(), pattern, true, results, listing);
        }
    }
}

}
