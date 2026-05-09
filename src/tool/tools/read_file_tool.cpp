#include "read_file_tool.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QTextStream>

namespace clawpp {

ReadFileTool::ReadFileTool(QObject* parent)
    : ITool(parent) {}

QString ReadFileTool::name() const {
    return "read_file";
}

QString ReadFileTool::description() const {
    return "Read the contents of a file at the given path. Returns the file content as a string.";
}

QJsonObject ReadFileTool::parameters() const {
    QJsonObject params;
    params["type"] = "object";
    
    QJsonObject properties;
    
    QJsonObject path;
    path["type"] = "string";
    path["description"] = "The absolute or relative path to the file to read";
    properties["path"] = path;
    
    params["properties"] = properties;
    params["required"] = QJsonArray{"path"};
    
    return params;
}

PermissionLevel ReadFileTool::permissionLevel() const {
    return PermissionLevel::Safe;
}

ToolResult ReadFileTool::execute(const QJsonObject& args) {
    ToolResult result;
    
    QString path = args.value("path").toString().trimmed();
    if (path.isEmpty()) {
        path = args.value("file").toString().trimmed();
    }
    if (path.isEmpty()) {
        path = args.value("file_path").toString().trimmed();
    }

    bool autoSelected = false;
    if (path.isEmpty()) {
        QString directory = args.value("directory").toString().trimmed();
        if (directory.isEmpty()) {
            directory = args.value("dir").toString().trimmed();
        }
        if (directory.isEmpty()) {
            directory = QDir::currentPath();
        }

        QDir dir(directory);
        if (dir.exists()) {
            const QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
            if (!files.isEmpty()) {
                static const QSet<QString> preferredSuffixes = {
                    QStringLiteral("md"), QStringLiteral("txt"), QStringLiteral("json"),
                    QStringLiteral("yml"), QStringLiteral("yaml"), QStringLiteral("xml"),
                    QStringLiteral("ini"), QStringLiteral("toml"), QStringLiteral("cpp"),
                    QStringLiteral("cc"), QStringLiteral("c"), QStringLiteral("h"),
                    QStringLiteral("hpp"), QStringLiteral("py"), QStringLiteral("js"),
                    QStringLiteral("ts"), QStringLiteral("tsx"), QStringLiteral("qml"),
                    QStringLiteral("cmake"), QStringLiteral("bat"), QStringLiteral("sh"),
                    QStringLiteral("log"), QStringLiteral("csv")
                };

                for (const QFileInfo& info : files) {
                    if (preferredSuffixes.contains(info.suffix().toLower())) {
                        path = info.absoluteFilePath();
                        break;
                    }
                }
                if (path.isEmpty()) {
                    path = files.first().absoluteFilePath();
                }
                autoSelected = !path.isEmpty();
            }
        }
    }

    if (path.isEmpty()) {
        result.success = false;
        result.content = "Error: path parameter is required";
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
    result.content = in.readAll();
    result.success = true;
    file.close();
    
    QFileInfo info(path);
    result.metadata["size"] = info.size();
    result.metadata["path"] = info.absoluteFilePath();
    result.metadata["lines"] = result.content.isEmpty() ? 0 : result.content.count('\n') + 1;
    if (autoSelected) {
        result.metadata["path_auto_selected"] = true;
    }
    
    return result;
}

}
