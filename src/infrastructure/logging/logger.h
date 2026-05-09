#ifndef CLAWPP_INFRASTRUCTURE_LOGGING_LOGGER_H
#define CLAWPP_INFRASTRUCTURE_LOGGING_LOGGER_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <memory>

namespace clawpp {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

class Logger : public QObject {
    Q_OBJECT

public:
    static Logger& instance();

    void setLevel(LogLevel level);
    void setFileEnabled(bool enabled);

    void debug(const QString& message);
    void info(const QString& message);
    void warning(const QString& message);
    void error(const QString& message);

private:
    Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void initLogFile();
    void log(LogLevel level, const QString& message);
    QString levelToString(LogLevel level) const;

    LogLevel m_level;
    bool m_fileEnabled;
    std::unique_ptr<QFile> m_logFile;
    QMutex m_mutex;
};

#define LOG_DEBUG(msg) clawpp::Logger::instance().debug(msg)
#define LOG_INFO(msg) clawpp::Logger::instance().info(msg)
#define LOG_WARN(msg) clawpp::Logger::instance().warning(msg)
#define LOG_ERROR(msg) clawpp::Logger::instance().error(msg)

}

#endif
