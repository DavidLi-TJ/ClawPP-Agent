#include "bash_analyzer.h"
#include <QDebug>

namespace clawpp {

BashAnalyzer::BashAnalyzer() {
    // 初始化危险命令列表（23项安全检查的核心）
    m_criticalCommands = {
        "rm -rf /",
        "rm -rf /*",
        "rm -rf ~/*",
        "dd if=/dev/zero",
        "dd if=/dev/random",
        "mkfs",
        "mkfs.ext",
        "format",
        "fdisk",
        "parted",
        "> /dev/sda",
        "> /dev/sd",
        ":(){ :|:& };:",  // Fork bomb
        "chmod -R 777 /",
        "chown -R",
        "kill -9 -1",
        "pkill -9",
        "killall",
        "shutdown",
        "reboot",
        "poweroff",
        "halt"
    };
    
    // 敏感系统路径
    m_sensitivePaths = {
        "/",
        "/etc",
        "/usr",
        "/bin",
        "/sbin",
        "/boot",
        "/sys",
        "/proc",
        "/dev",
        "/root",
        "/var/log",
        "/lib",
        "/opt"
    };
    
    // 危险重定向目标
    m_dangerousRedirects = {
        "/dev/sda",
        "/dev/sdb",
        "/dev/sd",
        "/dev/null",
        "/etc/passwd",
        "/etc/shadow",
        "/etc/sudoers",
        "/boot/grub/grub.cfg"
    };
    
    // 命令注入模式（正则）
    m_injectionPattern = QRegularExpression(
        R"([\$`]\(.*\)|[;&|].*|<\(.*\)|>\(.*\))",
        QRegularExpression::CaseInsensitiveOption
    );
    
    // Glob 炸弹模式
    m_globBombPattern = QRegularExpression(
        R"(\*{2,}|(\*/){3,}|/{2,}\*)",
        QRegularExpression::CaseInsensitiveOption
    );
    
    // 后台进程模式
    m_backgroundPattern = QRegularExpression(
        R"(\s&\s*$|\bnohup\b|\bdisown\b)",
        QRegularExpression::CaseInsensitiveOption
    );
}

QVector<SafetyIssue> BashAnalyzer::analyze(const QString& command) const {
    QVector<SafetyIssue> issues;
    
    if (command.trimmed().isEmpty()) {
        return issues;
    }
    
    // 执行所有安全检查
    checkDangerousCommands(command, issues);
    checkCommandInjection(command, issues);
    checkSensitivePaths(command, issues);
    checkGlobPatterns(command, issues);
    checkRedirections(command, issues);
    checkBackgroundProcesses(command, issues);
    checkNetworkOperations(command, issues);
    checkPrivilegeEscalation(command, issues);
    
    return issues;
}

bool BashAnalyzer::isDangerous(const QString& command) const {
    QVector<SafetyIssue> issues = analyze(command);
    
    // 检查是否有 critical 级别问题
    for (const SafetyIssue& issue : issues) {
        if (issue.severity == "critical") {
            return true;
        }
    }
    
    return false;
}

int BashAnalyzer::getRiskLevel(const QString& command) const {
    QVector<SafetyIssue> issues = analyze(command);
    
    if (issues.isEmpty()) {
        return 0;  // safe
    }
    
    int maxLevel = 0;
    for (const SafetyIssue& issue : issues) {
        if (issue.severity == "critical") {
            return 4;  // critical - immediate return
        } else if (issue.severity == "warning") {
            maxLevel = qMax(maxLevel, 2);  // medium
        } else if (issue.severity == "info") {
            maxLevel = qMax(maxLevel, 1);  // low
        }
    }
    
    return maxLevel;
}

void BashAnalyzer::checkDangerousCommands(const QString& command, QVector<SafetyIssue>& issues) const {
    for (const QString& dangerous : m_criticalCommands) {
        if (command.contains(dangerous, Qt::CaseInsensitive)) {
            issues.append({
                .type = "dangerous_command",
                .severity = "critical",
                .message = QString("检测到危险命令: %1").arg(dangerous),
                .details = "此命令可能导致系统损坏或数据丢失"
            });
        }
    }
}

void BashAnalyzer::checkCommandInjection(const QString& command, QVector<SafetyIssue>& issues) const {
    QRegularExpressionMatch match = m_injectionPattern.match(command);
    if (match.hasMatch()) {
        QString matchedPattern = match.captured(0);
        issues.append({
            .type = "command_injection",
            .severity = "warning",
            .message = "检测到命令链接或替换模式",
            .details = QString("发现模式: %1 - 可能导致意外命令执行").arg(matchedPattern)
        });
    }
    
    // 检查环境变量引用（可能泄露）
    if (command.contains(QRegularExpression(R"(\$\{?[A-Z_][A-Z0-9_]*\}?)"))) {
        issues.append({
            .type = "env_var_access",
            .severity = "info",
            .message = "命令包含环境变量引用",
            .details = "确保环境变量值可信"
        });
    }
}

void BashAnalyzer::checkSensitivePaths(const QString& command, QVector<SafetyIssue>& issues) const {
    for (const QString& path : m_sensitivePaths) {
        // 使用词边界匹配，避免误报（如 /etc/config 不触发 /etc）
        QRegularExpression pathPattern(QString(R"(\b%1\b)").arg(QRegularExpression::escape(path)));
        if (command.contains(pathPattern)) {
            // 检查是否是写操作
            bool isWrite = command.contains(QRegularExpression(R"(\b(rm|rmdir|mv|cp|>|>>|tee)\b)"));
            
            issues.append({
                .type = "sensitive_path",
                .severity = isWrite ? "warning" : "info",
                .message = QString("操作敏感系统路径: %1").arg(path),
                .details = isWrite ? "写操作可能影响系统稳定性" : "读取系统目录"
            });
        }
    }
}

void BashAnalyzer::checkGlobPatterns(const QString& command, QVector<SafetyIssue>& issues) const {
    QRegularExpressionMatch match = m_globBombPattern.match(command);
    if (match.hasMatch()) {
        issues.append({
            .type = "glob_bomb",
            .severity = "warning",
            .message = "检测到危险 Glob 模式",
            .details = QString("模式 '%1' 可能匹配大量文件，导致系统卡顿").arg(match.captured(0))
        });
    }
}

void BashAnalyzer::checkRedirections(const QString& command, QVector<SafetyIssue>& issues) const {
    for (const QString& target : m_dangerousRedirects) {
        if (command.contains(QRegularExpression(QString(R"([>|]\s*%1)").arg(QRegularExpression::escape(target))))) {
            issues.append({
                .type = "dangerous_redirect",
                .severity = "critical",
                .message = QString("危险重定向目标: %1").arg(target),
                .details = "向此目标写入可能导致系统损坏"
            });
        }
    }
}

void BashAnalyzer::checkBackgroundProcesses(const QString& command, QVector<SafetyIssue>& issues) const {
    QRegularExpressionMatch match = m_backgroundPattern.match(command);
    if (match.hasMatch()) {
        issues.append({
            .type = "background_process",
            .severity = "info",
            .message = "命令将在后台运行",
            .details = "后台进程可能持续占用资源"
        });
    }
}

void BashAnalyzer::checkNetworkOperations(const QString& command, QVector<SafetyIssue>& issues) const {
    QStringList networkCommands = {"curl", "wget", "nc", "netcat", "telnet", "ssh", "scp", "rsync"};
    
    for (const QString& netCmd : networkCommands) {
        if (command.contains(QRegularExpression(QString(R"(\b%1\b)").arg(netCmd)))) {
            issues.append({
                .type = "network_operation",
                .severity = "info",
                .message = QString("网络操作: %1").arg(netCmd),
                .details = "命令将进行网络通信"
            });
            break;  // 只报告一次
        }
    }
}

void BashAnalyzer::checkPrivilegeEscalation(const QString& command, QVector<SafetyIssue>& issues) const {
    QStringList sudoCommands = {"sudo", "su", "pkexec"};
    
    for (const QString& sudoCmd : sudoCommands) {
        if (command.contains(QRegularExpression(QString(R"(\b%1\b)").arg(sudoCmd)))) {
            issues.append({
                .type = "privilege_escalation",
                .severity = "warning",
                .message = QString("权限提升: %1").arg(sudoCmd),
                .details = "命令尝试提升执行权限"
            });
        }
    }
}

bool BashAnalyzer::containsAny(const QString& text, const QStringList& patterns) const {
    for (const QString& pattern : patterns) {
        if (text.contains(pattern, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool BashAnalyzer::matchesPattern(const QString& text, const QString& pattern) const {
    QRegularExpression re(pattern, QRegularExpression::CaseInsensitiveOption);
    return re.match(text).hasMatch();
}

}
