#include "write_file_tool.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSaveFile>
#include <QTextStream>
#include <QDateTime>
#include <QRegularExpression>

namespace {

bool pathLooksLikeContent(const QString& path) {
    const QString trimmed = path.trimmed();
    return trimmed.contains('\n')
        || trimmed.startsWith(QLatin1Char('#'))
        || trimmed.length() > 240
        || trimmed.contains(QRegularExpression(QStringLiteral("[<>:\"|?*]")));
}

QString defaultGeneratedPath() {
    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"));
    return QDir::current().filePath(QStringLiteral("generated_outputs/%1.md").arg(stamp));
}

}

namespace clawpp {

WriteFileTool::WriteFileTool(QObject* parent)
    : ITool(parent) {}

QString WriteFileTool::name() const {
    return "write_file";
}

QString WriteFileTool::description() const {
    return "Write content to a file at the given path. Creates the file if it does not exist, overwrites if it does. Creates parent directories if needed.";
}

QJsonObject WriteFileTool::parameters() const {
    QJsonObject params;
    params["type"] = "object";
    
    QJsonObject properties;
    
    QJsonObject path;
    path["type"] = "string";
    path["description"] = "The path where the file should be written";
    properties["path"] = path;
    
    QJsonObject content;
    content["type"] = "string";
    content["description"] = "The content to write to the file";
    properties["content"] = content;
    
    params["properties"] = properties;
    params["required"] = QJsonArray{"path", "content"};
    
    return params;
}

PermissionLevel WriteFileTool::permissionLevel() const {
    return PermissionLevel::Dangerous;
}

ToolResult WriteFileTool::execute(const QJsonObject& args) {
    ToolResult result;
    
    QString path = args["path"].toString().trimmed();
    QString content = args["content"].toString();
    
    if (path.isEmpty()) {
        result.success = false;
        result.content = "Error: path parameter is required";
        return result;
    }

    if (pathLooksLikeContent(path)) {
        if (content.trimmed().isEmpty()) {
            content = path;
            path = defaultGeneratedPath();
        } else {
            result.success = false;
            result.content = QStringLiteral("Error: path looks like document content, not a file path. Use a short path such as generated_outputs/story.md and put the document body in content.");
            return result;
        }
    }
    
    QFileInfo info(path);
    QDir dir = info.absoluteDir();
    
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            result.success = false;
            result.content = QString("Error: Cannot create parent directory: %1").arg(dir.absolutePath());
            return result;
        }
    }
    
    // 使用 QSaveFile 原子落盘，避免进程中断导致目标文件半写入。
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result.success = false;
        result.content = QString("Error: Cannot open file for writing: %1 (%2)").arg(path, file.errorString());
        return result;
    }
    
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << content;

    if (!file.commit()) {
        result.success = false;
        result.content = QString("Error: Failed to commit file: %1 (%2)").arg(path, file.errorString());
        return result;
    }
    
    result.success = true;
    result.content = QString("Successfully wrote %1 bytes to %2").arg(content.size()).arg(path);
    result.metadata["bytes_written"] = content.size();
    result.metadata["path"] = info.absoluteFilePath();
    
    return result;
}

}
