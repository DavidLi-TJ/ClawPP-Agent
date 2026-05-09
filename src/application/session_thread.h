#ifndef CLAWPP_APPLICATION_SESSION_THREAD_H
#define CLAWPP_APPLICATION_SESSION_THREAD_H

#include <QThread>
#include <QObject>

namespace clawpp {

class SessionThread : public QThread {
    Q_OBJECT

public:
    explicit SessionThread(QObject* parent = nullptr);
    ~SessionThread();
    
    void setWorker(QObject* worker);
    void start();
    void stop();

protected:
    void run() override;

private:
    QObject* m_worker;
};

}

#endif
