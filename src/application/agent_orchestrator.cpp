#include "agent_orchestrator.h"

#include <QDateTime>

#include "application/agent_service.h"

namespace clawpp {

AgentOrchestrator::AgentOrchestrator(QObject* parent)
    : QObject(parent)
    , m_agentService(nullptr)
    , m_running(false) {}

void AgentOrchestrator::setAgentService(AgentService* service) {
    if (m_agentService == service) {
        return;
    }
    if (m_agentService) {
        disconnect(m_agentService, nullptr, this, nullptr);
    }
    m_agentService = service;
    bindServiceSignals();
}

void AgentOrchestrator::sendUserInput(const QString& input) {
    const QString trimmed = input.trimmed();
    if (trimmed.isEmpty()) {
        emit inputRejected(QStringLiteral("输入内容为空"));
        return;
    }
    if (!m_agentService) {
        emit inputRejected(QStringLiteral("AgentService 未初始化"));
        return;
    }
    if (m_running) {
        if (!m_agentService->isRunning()) {
            m_running = false;
            m_activeRequestId.clear();
        } else {
        emit inputRejected(QStringLiteral("上一条回复仍在生成，请稍后重试"));
        return;
        }
    }

    m_activeRequestId = createRequestId();
    m_running = true;
    emit runStarted(m_activeRequestId);
    m_agentService->sendMessage(trimmed);
}

void AgentOrchestrator::stopActiveRun() {
    if (!m_running || !m_agentService) {
        return;
    }
    m_agentService->stopGeneration();
}

bool AgentOrchestrator::isRunning() const {
    return m_running;
}

QString AgentOrchestrator::activeRequestId() const {
    return m_activeRequestId;
}

void AgentOrchestrator::bindServiceSignals() {
    if (!m_agentService) {
        return;
    }

    connect(m_agentService, &AgentService::responseComplete, this, [this](const QString&) {
        if (!m_running) {
            return;
        }
        const QString requestId = m_activeRequestId;
        m_running = false;
        m_activeRequestId.clear();
        emit runFinished(requestId, QStringLiteral("completed"));
    }, Qt::UniqueConnection);

    connect(m_agentService, &AgentService::errorOccurred, this, [this](const QString&) {
        if (!m_running) {
            return;
        }
        const QString requestId = m_activeRequestId;
        m_running = false;
        m_activeRequestId.clear();
        emit runFinished(requestId, QStringLiteral("error"));
    }, Qt::UniqueConnection);

    connect(m_agentService, &AgentService::generationStopped, this, [this]() {
        if (!m_running) {
            return;
        }
        const QString requestId = m_activeRequestId;
        m_running = false;
        m_activeRequestId.clear();
        emit runFinished(requestId, QStringLiteral("stopped"));
    }, Qt::UniqueConnection);
}

QString AgentOrchestrator::createRequestId() const {
    return QStringLiteral("req-%1").arg(QDateTime::currentMSecsSinceEpoch());
}

} // namespace clawpp
