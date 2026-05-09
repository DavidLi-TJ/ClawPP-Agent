#include "filesystem_tool.h"
#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QTextStream>

namespace clawpp {

FilesystemTool::FilesystemTool(QObject* parent)
    : ITool(parent) {}

QString FilesystemTool::name() const {
    return "filesystem";
}

QString FilesystemTool::description() const {
    return "File system operations: read_file, write_file, list_directory, create_directory, delete_file, copy_file, move_file, search_files";
}

QJsonObject FilesystemTool::parameters() const {
    QJsonObject params;
    params["type"] = "object";
    
    QJsonObject properties;
    
    QJsonObject operation;
    operation["type"] = "string";
    operation["enum"] = QJsonArray{
        "read_file", "write_file", "list_directory", 
        "create_directory", "delete_file", "copy_file", 
        "move_file", "search_files"
    };
    operation["description"] = "The operation to perform";
    properties["operation"] = operation;
    
    QJsonObject path;
    path["type"] = "string";
    path["description"] = "File or directory path";
    properties["path"] = path;
    
    QJsonObject content;
    content["type"] = "string";
    content["description"] = "Content to write (for write_file)";
    properties["content"] = content;
    
    QJsonObject destination;
    destination["type"] = "string";
    destination["description"] = "Destination path (for copy_file, move_file)";
    properties["destination"] = destination;
    
    QJsonObject pattern;
    pattern["type"] = "string";
    pattern["description"] = "Search pattern (for search_files)";
    properties["pattern"] = pattern;
    
    QJsonObject recursive;
    recursive["type"] = "boolean";
    recursive["description"] = "Search recursively (for search_files)";
    properties["recursive"] = recursive;
    
    params["properties"] = properties;
    params["required"] = QJsonArray{"operation", "path"};
    
    return params;
}

PermissionLevel FilesystemTool::permissionLevel() const {
    return PermissionLevel::Moderate;
}

ToolResult FilesystemTool::execute(const QJsonObject& args) {
    ToolResult result;
    result.toolCallId = "";
    
    QString operation = args["operation"].toString();
    QString path = args["path"].toString();
    
    if (operation.isEmpty() || path.isEmpty()) {
        result.success = false;
        result.content = "Error: operation and path are required";
        return result;
    }
    
    if (operation == "read_file") {
        return readFile(path);
    } else if (operation == "write_file") {
        QString content = args["content"].toString();
        return writeFile(path, content);
    } else if (operation == "list_directory") {
        return listDirectory(path);
    } else if (operation == "create_directory") {
        return createDirectory(path);
    } else if (operation == "delete_file") {
        return deleteFile(path);
    } else if (operation == "copy_file") {
        QString destination = args["destination"].toString();
        return copyFile(path, destination);
    } else if (operation == "move_file") {
        QString destination = args["destination"].toString();
        return moveFile(path, destination);
    } else if (operation == "search_files") {
        QString pattern = args["pattern"].toString("*");
        bool recursive = args["recursive"].toBool(true);
        return searchFiles(path, pattern, recursive);
    } else {
        result.success = false;
        result.content = QString("Error: Unknown operation: %1").arg(operation);
        return result;
    }
}

ToolResult FilesystemTool::readFile(const QString& path) {
    ToolResult result;
    
    QFile file(path);
    if (!file.exists()) {
        result.success = false;
        result.content = QString("Error: File does not exist: %1").arg(path);
        return result;
    }
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.success = false;
        result.content = QString("Error: Cannot open file: %1").arg(file.errorString());
        return result;
    }
    
    QTextStream in(&file);
    result.content = in.readAll();
    result.success = true;
    file.close();
    
    result.metadata["size"] = file.size();
    result.metadata["path"] = path;
    
    return result;
}

ToolResult FilesystemTool::writeFile(const QString& path, const QString& content) {
    ToolResult result;

    QFileInfo fileInfo(path);
    QDir parentDir = fileInfo.dir();
    if (!parentDir.exists() && !parentDir.mkpath(".")) {
        result.success = false;
        result.content = QString("Error: Cannot create parent directory: %1").arg(parentDir.absolutePath());
        return result;
    }
    
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result.success = false;
        result.content = QString("Error: Cannot open file for writing: %1").arg(file.errorString());
        return result;
    }
    
    QTextStream out(&file);
    out << content;
    file.close();
    
    result.success = true;
    result.content = QString("Successfully wrote %1 bytes to %2").arg(content.size()).arg(path);
    result.metadata["bytes_written"] = content.size();
    result.metadata["path"] = path;
    
    return result;
}

ToolResult FilesystemTool::listDirectory(const QString& path) {
    ToolResult result;
    
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
    
    return result;
}

ToolResult FilesystemTool::createDirectory(const QString& path) {
    ToolResult result;
    
    QDir dir;
    if (dir.mkpath(path)) {
        result.success = true;
        result.content = QString("Successfully created directory: %1").arg(path);
        result.metadata["path"] = path;
    } else {
        result.success = false;
        result.content = QString("Error: Failed to create directory: %1").arg(path);
    }
    
    return result;
}

ToolResult FilesystemTool::deleteFile(const QString& path) {
    ToolResult result;
    
    QFileInfo info(path);
    bool success = false;
    
    if (info.isDir()) {
        QDir dir(path);
        success = dir.removeRecursively();
    } else {
        QFile file(path);
        success = file.remove();
    }
    
    if (success) {
        result.success = true;
        result.content = QString("Successfully deleted: %1").arg(path);
        result.metadata["path"] = path;
    } else {
        result.success = false;
        result.content = QString("Error: Failed to delete: %1").arg(path);
    }
    
    return result;
}

ToolResult FilesystemTool::copyFile(const QString& source, const QString& destination) {
    ToolResult result;
    
    if (QFile::copy(source, destination)) {
        result.success = true;
        result.content = QString("Successfully copied %1 to %2").arg(source, destination);
        result.metadata["source"] = source;
        result.metadata["destination"] = destination;
    } else {
        result.success = false;
        result.content = QString("Error: Failed to copy %1 to %2").arg(source, destination);
    }
    
    return result;
}

ToolResult FilesystemTool::moveFile(const QString& source, const QString& destination) {
    ToolResult result;
    
    if (QFile::rename(source, destination)) {
        result.success = true;
        result.content = QString("Successfully moved %1 to %2").arg(source, destination);
        result.metadata["source"] = source;
        result.metadata["destination"] = destination;
    } else {
        result.success = false;
        result.content = QString("Error: Failed to move %1 to %2").arg(source, destination);
    }
    
    return result;
}

ToolResult FilesystemTool::searchFiles(const QString& path, const QString& pattern, bool recursive) {
    ToolResult result;
    
    QDir dir(path);
    if (!dir.exists()) {
        result.success = false;
        result.content = QString("Error: Directory does not exist: %1").arg(path);
        return result;
    }
    
    QJsonArray items;
    QStringList listing;

    // QDirIterator 才能实现真正递归；entryInfoList 只会列出当前目录。
    QDirIterator::IteratorFlags flags = recursive
        ? QDirIterator::Subdirectories
        : QDirIterator::NoIteratorFlags;
    QDirIterator iterator(path, QStringList() << pattern, QDir::Files | QDir::NoDotAndDotDot, flags);

    int count = 0;
    while (iterator.hasNext()) {
        iterator.next();
        const QFileInfo info = iterator.fileInfo();
        QJsonObject item;
        item["name"] = info.fileName();
        item["path"] = info.absoluteFilePath();
        item["size"] = info.size();
        items.append(item);

        listing.append(info.absoluteFilePath());
        ++count;
    }

    result.success = true;
    result.content = QString("Found %1 files matching '%2' in %3:\n%4")
        .arg(count).arg(pattern, path, listing.join("\n"));
    result.metadata["items"] = items;
    result.metadata["count"] = count;
    
    return result;
}

}
