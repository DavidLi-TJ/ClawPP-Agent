#ifndef CLAWPP_INFRASTRUCTURE_RUNNER_MANAGER_H
#define CLAWPP_INFRASTRUCTURE_RUNNER_MANAGER_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QEventLoop>

namespace clawpp {

class RunnerManager : public QObject {
    Q_OBJECT

public:
    enum class Mode {
        Local,          // 直连模式（当前默认，无隔离）
        Http,           // 独立 HTTP API Server 模式（低权后台）
        Subprocess      // 动态子进程模式（runas / 降权启动）
    };

    static RunnerManager& instance();

    void setMode(Mode mode);
    void setHttpUrl(const QString& url);
    void setSubprocessPath(const QString& path);
    void setTimeout(int timeoutMs);

    // 核心执行入口
    QJsonObject execute(const QString& action, const QJsonObject& params);

private:
    explicit RunnerManager(QObject* parent = nullptr);
    ~RunnerManager() override = default;

    QJsonObject executeLocal(const QString& action, const QJsonObject& params);
    QJsonObject executeHttp(const QString& action, const QJsonObject& params);
    QJsonObject executeSubprocess(const QString& action, const QJsonObject& params);

    Mode m_mode = Mode::Local;
    QString m_httpUrl = "http://127.0.0.1:8081";
    QString m_subprocessPath = "clawrunner_internal.exe";
    int m_timeout = 30000;
};

} // namespace clawpp

#endif // CLAWPP_INFRASTRUCTURE_RUNNER_MANAGER_H
