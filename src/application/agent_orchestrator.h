#ifndef CLAWPP_APPLICATION_AGENT_ORCHESTRATOR_H
#define CLAWPP_APPLICATION_AGENT_ORCHESTRATOR_H

#include <QObject>
#include <QPointer>

namespace clawpp {

class AgentService;

class AgentOrchestrator : public QObject {
    Q_OBJECT

public:
    explicit AgentOrchestrator(QObject* parent = nullptr);

    void setAgentService(AgentService* service);
    void sendUserInput(const QString& input);
    void stopActiveRun();

    bool isRunning() const;
    QString activeRequestId() const;

signals:
    void runStarted(const QString& requestId);
    void runFinished(const QString& requestId, const QString& status);
    void inputRejected(const QString& reason);

private:
    void bindServiceSignals();
    QString createRequestId() const;

    QPointer<AgentService> m_agentService;
    QString m_activeRequestId;
    bool m_running;
};

} // namespace clawpp

#endif
