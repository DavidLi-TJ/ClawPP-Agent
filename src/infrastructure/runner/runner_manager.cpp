#include "runner_manager.h"
#include <QProcess>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDir>
#include <QFile>
#include <QCoreApplication>
#include <QRegularExpression>

namespace clawpp {

namespace {

QString adaptShellCommandForWindows(const QString& command) {
    QString adapted = command;
    adapted.replace(QRegularExpression(QStringLiteral("(?<![&])&&(?![&])")),
                    QStringLiteral("& if errorlevel 1 exit /b %errorlevel% & "));
    return adapted;
}

}

RunnerManager& RunnerManager::instance() {
    static RunnerManager instance;
    return instance;
}

RunnerManager::RunnerManager(QObject* parent) : QObject(parent) {
    // 默认启用子进程隔离（Sandbox）
    m_mode = Mode::Subprocess;
}

void RunnerManager::setMode(Mode mode) {
    m_mode = mode;
}

void RunnerManager::setHttpUrl(const QString& url) {
    m_httpUrl = url;
}

void RunnerManager::setSubprocessPath(const QString& path) {
    m_subprocessPath = path;
}

void RunnerManager::setTimeout(int timeoutMs) {
    m_timeout = timeoutMs;
}

QJsonObject RunnerManager::execute(const QString& action, const QJsonObject& params) {
    switch (m_mode) {
        case Mode::Http:
            return executeHttp(action, params);
        case Mode::Subprocess:
            return executeSubprocess(action, params);
        case Mode::Local:
        default:
            return executeLocal(action, params);
    }
}

QJsonObject RunnerManager::executeLocal(const QString& action, const QJsonObject& params) {
    QJsonObject result;
    
    if (action == "shell") {
        QString command = params["command"].toString();
        QString cwd = params["cwd"].toString(QDir::currentPath());
        
#ifdef Q_OS_WIN
        const QString adapted = adaptShellCommandForWindows(command);
        QString fullCmd = QString("cmd /c \"%1\"").arg(adapted);
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

        if (!process.waitForFinished(m_timeout)) {
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
    
    // Future actions like file_read, file_write, browser_goto
    result["error"] = QString("Unsupported local action: %1").arg(action);
    return result;
}

QJsonObject RunnerManager::executeHttp(const QString& action, const QJsonObject& params) {
    QJsonObject result;
    QNetworkAccessManager manager;
    
    QNetworkRequest request(QUrl(m_httpUrl + "/api/runner/execute"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject payload;
    payload["action"] = action;
    payload["params"] = params;

    QNetworkReply* reply = manager.post(request, QJsonDocument(payload).toJson());
    
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    if (reply->error() != QNetworkReply::NoError) {
        result["error"] = QString("HTTP Runner Error: %1").arg(reply->errorString());
    } else {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            result["error"] = "Invalid JSON response from HTTP Runner.";
        } else {
            result = doc.object();
        }
    }
    reply->deleteLater();
    return result;
}

QJsonObject RunnerManager::executeSubprocess(const QString& action, const QJsonObject& params) {
    QJsonObject result;
    QProcess process;
    
    QString runnerExe = m_subprocessPath;
    if (runnerExe.isEmpty()) {
#ifdef Q_OS_WIN
        runnerExe = QCoreApplication::applicationDirPath() + "/bin/clawrunner_internal.exe";
#else
        runnerExe = QCoreApplication::applicationDirPath() + "/bin/clawrunner_internal";
#endif
    } else if (!runnerExe.contains('/') && !runnerExe.contains('\\')) {
        // Bare filename: resolve relative to application dir, trying bin/ first
        QString base = QCoreApplication::applicationDirPath();
#ifdef Q_OS_WIN
        QString binPath = base + "/bin/" + runnerExe;
        QString localPath = base + "/" + runnerExe;
#else
        QString binPath = base + "/bin/" + runnerExe;
        QString localPath = base + "/" + runnerExe;
#endif
        if (QFile::exists(binPath))
            runnerExe = binPath;
        else if (QFile::exists(localPath))
            runnerExe = localPath;
    }
    
    process.start(runnerExe, QStringList());
    if (!process.waitForStarted(1000)) {
        result["error"] = QString("Runner Subprocess failed to start: %1").arg(runnerExe);
        return result;
    }

    QJsonObject payload;
    payload["action"] = action;
    payload["params"] = params;
    
    process.write(QJsonDocument(payload).toJson(QJsonDocument::Compact) + "\n");
    process.waitForBytesWritten(1000);
    
    if (!process.waitForFinished(m_timeout)) {
        process.kill();
        process.waitForFinished(1000);
        result["error"] = "Runner Subprocess timed out.";
        return result;
    }

    QByteArray output = process.readAllStandardOutput();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(output, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        result["error"] = "Invalid JSON response from Runner Subprocess.";
        result["raw_output"] = QString::fromUtf8(output);
        result["raw_error"] = QString::fromUtf8(process.readAllStandardError());
    } else {
        result = doc.object();
    }
    
    return result;
}

} // namespace clawpp
