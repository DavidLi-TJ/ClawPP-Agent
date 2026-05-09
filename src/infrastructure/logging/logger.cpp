#include "logger.h"

namespace clawpp {

/**
 * @brief 获取 Logger 单例实例
 * @return Logger 实例引用
 */
Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

/**
 * @brief Logger 构造函数
 * 初始化日志级别和文件开关
 */
Logger::Logger()
    : m_level(LogLevel::Info)
    , m_fileEnabled(false) {
}

/**
 * @brief 设置日志级别
 * @param level 日志级别
 * 
 * 只有级别 >= 设定级别的消息才会被输出
 */
void Logger::setLevel(LogLevel level) {
    m_level = level;
}

/**
 * @brief 设置文件日志开关
 * @param enabled 是否启用文件日志
 * 
 * 启用时会自动初始化日志文件
 */
void Logger::setFileEnabled(bool enabled) {
    m_fileEnabled = enabled;
    if (enabled && !m_logFile) {
        initLogFile();
    }
}

/**
 * @brief 输出 Debug 级别日志
 * @param message 日志消息
 */
void Logger::debug(const QString& message) {
    log(LogLevel::Debug, message);
}

/**
 * @brief 输出 Info 级别日志
 * @param message 日志消息
 */
void Logger::info(const QString& message) {
    log(LogLevel::Info, message);
}

/**
 * @brief 输出 Warning 级别日志
 * @param message 日志消息
 */
void Logger::warning(const QString& message) {
    log(LogLevel::Warning, message);
}

/**
 * @brief 输出 Error 级别日志
 * @param message 日志消息
 */
void Logger::error(const QString& message) {
    log(LogLevel::Error, message);
}

/**
 * @brief 初始化日志文件
 * 
 * 在用户主目录创建 ~/.clawpp/logs/app.log 文件
 */
void Logger::initLogFile() {
    QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString logDir = homeDir + "/.clawpp/logs";

    QDir dir(logDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString logPath = logDir + "/app.log";
    m_logFile = std::make_unique<QFile>(logPath);
    m_logFile->open(QIODevice::WriteOnly | QIODevice::Append);
}

/**
 * @brief 核心日志输出函数
 * @param level 日志级别
 * @param message 日志消息
 * 
 * 同时输出到 stderr 和文件（如果启用）
 * 使用互斥锁保证线程安全
 */
void Logger::log(LogLevel level, const QString& message) {
    if (level < m_level) {
        return;
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString levelStr = levelToString(level);
    QString formatted = QString("[%1] [%2] %3").arg(timestamp, levelStr, message);

    {
        QMutexLocker locker(&m_mutex);

        QTextStream stream(stderr);
        stream << formatted << Qt::endl;

        if (m_fileEnabled && m_logFile && m_logFile->isOpen()) {
            QTextStream fileStream(m_logFile.get());
            fileStream << formatted << Qt::endl;
        }
    }
}

/**
 * @brief 将日志级别转换为字符串
 * @param level 日志级别
 * @return 级别字符串
 */
QString Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error: return "ERROR";
        default: return "UNKNOWN";
    }
}

}
