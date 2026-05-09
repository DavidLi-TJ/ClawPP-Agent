#include "edit_file_tool.h"
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QTextStream>

namespace clawpp {

EditFileTool::EditFileTool(QObject* parent)
    : ITool(parent) {}

QString EditFileTool::name() const {
    return "edit_file";
}

QString EditFileTool::description() const {
    return "Edit a file by replacing specific text. The file must exist. Use this for targeted modifications instead of rewriting entire files.";
}

QJsonObject EditFileTool::parameters() const {
    QJsonObject params;
    params["type"] = "object";
    
    QJsonObject properties;
    
    QJsonObject path;
    path["type"] = "string";
    path["description"] = "The path to the file to edit";
    properties["path"] = path;
    
    QJsonObject oldStr;
    oldStr["type"] = "string";
    oldStr["description"] = "The text to search for and replace. Must match exactly.";
    properties["old_str"] = oldStr;
    
    QJsonObject newStr;
    newStr["type"] = "string";
    newStr["description"] = "The text to replace the old_str with";
    properties["new_str"] = newStr;
    
    params["properties"] = properties;
    params["required"] = QJsonArray{"path", "old_str", "new_str"};
    
    return params;
}

PermissionLevel EditFileTool::permissionLevel() const {
    return PermissionLevel::Dangerous;
}

ToolResult EditFileTool::execute(const QJsonObject& args) {
    ToolResult result;
    
    QString path = args["path"].toString();
    QString oldStr = args["old_str"].toString();
    QString newStr = args["new_str"].toString();
    
    if (path.isEmpty()) {
        result.success = false;
        result.content = "Error: path parameter is required";
        return result;
    }
    
    if (oldStr.isEmpty()) {
        result.success = false;
        result.content = "Error: old_str parameter is required";
        return result;
    }
    
    QFile file(path);
    if (!file.exists()) {
        result.success = false;
        result.content = QString("Error: File does not exist: %1").arg(path);
        return result;
    }
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.success = false;
        result.content = QString("Error: Cannot open file: %1 (%2)").arg(path, file.errorString());
        return result;
    }
    
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QString content = in.readAll();
    file.close();

    const int occurrences = content.count(oldStr);
    if (occurrences <= 0) {
        result.success = false;
        result.content = QString("Error: old_str not found in file: %1").arg(path);
        return result;
    }

    if (oldStr == newStr) {
        result.success = true;
        result.content = QString("No change needed for %1 (old_str equals new_str)").arg(path);
        result.metadata["path"] = QFileInfo(path).absoluteFilePath();
        result.metadata["replacements"] = 0;
        return result;
    }
    
    QString edited = applyEdit(content, oldStr, newStr);

    // 原子写入，避免目标文件被部分覆盖。
    QSaveFile saveFile(path);
    if (!saveFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result.success = false;
        result.content = QString("Error: Cannot open file for writing: %1 (%2)").arg(path, saveFile.errorString());
        return result;
    }

    QTextStream out(&saveFile);
    out.setEncoding(QStringConverter::Utf8);
    out << edited;

    if (!saveFile.commit()) {
        result.success = false;
        result.content = QString("Error: Failed to commit edited file: %1 (%2)").arg(path, saveFile.errorString());
        return result;
    }
    
    result.success = true;
    result.content = QString("Successfully edited %1").arg(path);
    result.metadata["path"] = QFileInfo(path).absoluteFilePath();
    result.metadata["replacements"] = occurrences;
    
    return result;
}

QString EditFileTool::applyEdit(const QString& content, const QString& oldStr, const QString& newStr) {
    QString result = content;
    return result.replace(oldStr, newStr);
}

}
