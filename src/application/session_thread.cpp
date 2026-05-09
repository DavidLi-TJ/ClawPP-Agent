#include "session_thread.h"
#include <QObject>

namespace clawpp {

SessionThread::SessionThread(QObject* parent)
    : QThread(parent)
    , m_worker(nullptr) {}

SessionThread::~SessionThread() {
    stop();
}

void SessionThread::setWorker(QObject* worker) {
    m_worker = worker;
    if (m_worker) {
        m_worker->moveToThread(this);
    }
}

void SessionThread::start() {
    if (!isRunning()) {
        QThread::start();
    }
}

void SessionThread::stop() {
    if (isRunning()) {
        quit();
        wait();
    }
}

void SessionThread::run() {
    exec();
}

}
