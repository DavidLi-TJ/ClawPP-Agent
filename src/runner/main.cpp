#include <QCoreApplication>
#include <iostream>
#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QProcess>
#include <QDir>
#include <QThread>

QJsonObject handleShell(const QJsonObject& params) {
    QJsonObject result;
    QString command = params["command"].toString();
    QString cwd = params["cwd"].toString(QDir::currentPath());

#ifdef Q_OS_WIN
    QString fullCmd = QString("powershell.exe -NoProfile -NonInteractive -Command \"%1\"").arg(command);
#else
    QString fullCmd = QString("bash -c \"%1\"").arg(command);
#endif

    QProcess process;
    process.setWorkingDirectory(cwd);
    process.startCommand(fullCmd);

    if (!process.waitForStarted(1000)) {
        result["error"] = "Process failed to start.";
        return result;
    }

    if (!process.waitForFinished(300000)) { // Max 5 mins fallback
        process.kill();
        process.waitForFinished(1000);
        result["error"] = "Process execution timed out.";
        result["stdout"] = QString::fromUtf8(process.readAllStandardOutput());
        result["stderr"] = QString::fromUtf8(process.readAllStandardError());
        return result;
    }

    result["stdout"] = QString::fromUtf8(process.readAllStandardOutput());
    result["stderr"] = QString::fromUtf8(process.readAllStandardError());
    result["exit_code"] = process.exitCode();
    return result;
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    // Read single line JSON-RPC from stdin
    std::string line;
    if (!std::getline(std::cin, line)) {
        return 1;
    }

    QByteArray input = QByteArray::fromStdString(line);
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(input, &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        QJsonObject err;
        err["error"] = "Invalid JSON input";
        std::cout << QJsonDocument(err).toJson(QJsonDocument::Compact).toStdString() << std::endl;
        return 1;
    }

    QJsonObject request = doc.object();
    QString action = request["action"].toString();
    QJsonObject params = request["params"].toObject();
    
    QJsonObject response;
    
    if (action == "shell") {
        response = handleShell(params);
    } else {
        response["error"] = QString("Unknown action: %1").arg(action);
    }
    
    // Write out the response as single JSON line to stdout
    std::cout << QJsonDocument(response).toJson(QJsonDocument::Compact).toStdString() << std::endl;
    
    return 0;
}