#include "shell_tool.h"
#include "infrastructure/runner/runner_manager.h"
#include <QFileInfo>
#include <QDir>
#include <QJsonArray>
#include <QRegularExpression>

namespace clawpp {

ShellTool::ShellTool(QObject* parent)
    : ITool(parent)
    , m_workingDirectory(QDir::currentPath())
    , m_restrictToWorkspace(true)
    , m_defaultTimeout(30000) {
    m_blockedPatterns = {
        "rm -rf /",
        "rm -rf /*",
        "rm -rf ~",
        "rm -rf ~/*",
        "mkfs",
        "dd if=",
        "format ",
        "> /dev/sd",
        "> /dev/hd",
        ":(){ :|:& };:",
        "chmod -R 777 /",
        "chown -R",
        "shutdown",
        "reboot",
        "init 0",
        "init 6",
        "halt",
        "poweroff",
    };
    
#ifdef Q_OS_WIN
    m_blockedCommands = {
        "format",
        "del /s /q",
        "rmdir /s /q",
        "shutdown",
        "restart",
        "taskkill /f /im",
        "reg delete",
        "bcdedit",
        "diskpart",
        "wmic",
    };
#endif
}

QString ShellTool::name() const {
    return "shell";
}

QString ShellTool::description() const {
    return "Execute shell commands with timeout and security controls. Commands are validated against a blacklist for safety.";
}

QJsonObject ShellTool::parameters() const {
    QJsonObject params;
    params["type"] = "object";
    
    QJsonObject properties;
    
    QJsonObject command;
    command["type"] = "string";
    command["description"] = "The shell command to execute";
    properties["command"] = command;
    
    QJsonObject workingDir;
    workingDir["type"] = "string";
    workingDir["description"] = "Working directory for the command (optional, defaults to workspace)";
    properties["working_dir"] = workingDir;
    
    QJsonObject timeout;
    timeout["type"] = "integer";
    timeout["description"] = "Timeout in milliseconds (default: 30000, max: 300000)";
    timeout["default"] = 30000;
    properties["timeout"] = timeout;
    
    QJsonObject shell;
    shell["type"] = "boolean";
    shell["description"] = "Use shell to execute command (default: true)";
    shell["default"] = true;
    properties["use_shell"] = shell;
    
    params["properties"] = properties;
    params["required"] = QJsonArray{"command"};
    
    return params;
}

PermissionLevel ShellTool::permissionLevel() const {
    return PermissionLevel::Dangerous;
}

void ShellTool::setWorkingDirectory(const QString& dir) {
    m_workingDirectory = dir;
}

void ShellTool::setRestrictToWorkspace(bool restrict) {
    m_restrictToWorkspace = restrict;
}

void ShellTool::setDefaultTimeout(int timeoutMs) {
    m_defaultTimeout = qBound(1000, timeoutMs, 300000);
}

void ShellTool::addBlockedCommand(const QString& pattern) {
    m_blockedPatterns.append(pattern.toLower());
}

void ShellTool::clearBlockedCommands() {
    m_blockedPatterns.clear();
    m_blockedCommands.clear();
}

bool ShellTool::isCommandBlocked(const QString& command) const {
    QString lowerCmd = command.toLower();
    
    for (const QString& pattern : m_blockedPatterns) {
        if (lowerCmd.contains(pattern.toLower())) {
            return true;
        }
    }
    
    for (const QString& blocked : m_blockedCommands) {
        if (lowerCmd.startsWith(blocked.toLower())) {
            return true;
        }
    }
    
    return false;
}

bool ShellTool::isPathAllowed(const QString& path) const {
    if (!m_restrictToWorkspace || m_workingDirectory.isEmpty()) {
        return true;
    }
    
    QFileInfo info(path);
    QString absolutePath = info.absoluteFilePath();
    QString absoluteWorkspace = QDir(m_workingDirectory).absolutePath();

    // 使用相对路径判断是否越界，避免前缀比较误判（如 C:/ws 与 C:/ws_tmp）。
    const QString relative = QDir(absoluteWorkspace).relativeFilePath(absolutePath);
    return !relative.startsWith("..") && !QDir::isAbsolutePath(relative);
}

QString ShellTool::sanitizeCommand(const QString& command) const {
    QString result = command.trimmed();
    
    result.remove(QRegularExpression("\x1b\\[[0-9;]*[a-zA-Z]"));
    
    return result;
}

ToolResult ShellTool::execute(const QJsonObject& args) {
    ToolResult result;
    
    QString command = args["command"].toString();
    if (command.isEmpty()) {
        result.success = false;
        result.content = "Error: command is required";
        return result;
    }
    
    command = sanitizeCommand(command);
    
    if (isCommandBlocked(command)) {
        result.success = false;
        result.content = QString("Error: Command blocked for safety: %1").arg(command);
        result.metadata["blocked"] = true;
        return result;
    }
    
    QString workingDir = args["working_dir"].toString();
    if (workingDir.isEmpty()) {
        workingDir = m_workingDirectory;
    }
    
    if (!workingDir.isEmpty() && m_restrictToWorkspace) {
        if (!isPathAllowed(workingDir)) {
            result.success = false;
            result.content = QString("Error: Working directory outside allowed workspace: %1").arg(workingDir);
            result.metadata["blocked"] = true;
            return result;
        }
    }
    
    int timeoutMs = args["timeout"].toInt(m_defaultTimeout);
    timeoutMs = qBound(1000, timeoutMs, 300000);
    
    QJsonObject runnerParams;
    runnerParams["command"] = command;
    if (!workingDir.isEmpty()) {
        runnerParams["cwd"] = workingDir;
    }
    
    RunnerManager::instance().setTimeout(timeoutMs);
    QJsonObject runnerRes = RunnerManager::instance().execute("shell", runnerParams);
    
    if (runnerRes.contains("error")) {
        result.success = false;
        result.content = runnerRes["error"].toString();
        if (runnerRes.contains("stdout")) {
            result.content += "\nStdout: " + runnerRes["stdout"].toString();
        }
        if (runnerRes.contains("stderr")) {
            result.content += "\nStderr: " + runnerRes["stderr"].toString();
        }
        return result;
    }
    
    QString stdoutStr = runnerRes["stdout"].toString();
    QString stderrStr = runnerRes["stderr"].toString();
    int exitCode = runnerRes["exit_code"].toInt(0);
    
    result.success = (exitCode == 0);
    result.content = stdoutStr;
    if (!stderrStr.isEmpty()) {
        if (!result.content.isEmpty()) {
            result.content += "\n";
        }
        result.content += stderrStr;
    }
    
    result.metadata["exit_code"] = exitCode;
    result.metadata["command"] = command;
    result.metadata["timeout"] = timeoutMs;
    result.metadata["working_dir"] = workingDir;
    
    if (!result.success) {
        result.content = QString("Command failed with exit code %1:\n%2").arg(exitCode).arg(result.content);
    }
    
    return result;
}

}
