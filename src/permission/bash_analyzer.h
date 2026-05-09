#ifndef CLAWPP_PERMISSION_BASH_ANALYZER_H
#define CLAWPP_PERMISSION_BASH_ANALYZER_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QRegularExpression>

namespace clawpp {

/**
 * @struct SafetyIssue
 * @brief Bash 命令安全问题
 */
struct SafetyIssue {
    QString type;        ///< 问题类型：dangerous_command, command_injection, sensitive_path, etc
    QString severity;    ///< 严重级别：critical, warning, info
    QString message;     ///< 问题描述
    QString details;     ///< 详细信息
};

/**
 * @class BashAnalyzer
 * @brief Bash 命令安全分析器
 * 
 * 实现轻量版安全检查（不使用完整 AST，基于规则匹配）：
 * - 危险命令检测
 * - 命令注入模式识别
 * - 敏感路径检测
 * - Glob 炸弹检测
 * - 重定向危险检测
 */
class BashAnalyzer {
public:
    BashAnalyzer();
    
    /**
     * @brief 分析 Bash 命令的安全性
     * @param command 待分析的命令字符串
     * @return 安全问题列表
     */
    QVector<SafetyIssue> analyze(const QString& command) const;
    
    /**
     * @brief 检查命令是否包含危险操作
     * @param command 命令字符串
     * @return 是否危险
     */
    bool isDangerous(const QString& command) const;
    
    /**
     * @brief 获取命令的风险等级
     * @param command 命令字符串
     * @return 风险等级：0=safe, 1=low, 2=medium, 3=high, 4=critical
     */
    int getRiskLevel(const QString& command) const;

private:
    // 检查各类安全问题
    void checkDangerousCommands(const QString& command, QVector<SafetyIssue>& issues) const;
    void checkCommandInjection(const QString& command, QVector<SafetyIssue>& issues) const;
    void checkSensitivePaths(const QString& command, QVector<SafetyIssue>& issues) const;
    void checkGlobPatterns(const QString& command, QVector<SafetyIssue>& issues) const;
    void checkRedirections(const QString& command, QVector<SafetyIssue>& issues) const;
    void checkBackgroundProcesses(const QString& command, QVector<SafetyIssue>& issues) const;
    void checkNetworkOperations(const QString& command, QVector<SafetyIssue>& issues) const;
    void checkPrivilegeEscalation(const QString& command, QVector<SafetyIssue>& issues) const;
    
    // 工具方法
    bool containsAny(const QString& text, const QStringList& patterns) const;
    bool matchesPattern(const QString& text, const QString& pattern) const;
    
    // 危险模式定义
    QStringList m_criticalCommands;
    QStringList m_sensitivePaths;
    QStringList m_dangerousRedirects;
    QRegularExpression m_injectionPattern;
    QRegularExpression m_globBombPattern;
    QRegularExpression m_backgroundPattern;
};

}

#endif
