#include "session_manager.h"
#include "infrastructure/database/session_store.h"
#include "infrastructure/database/conversation_store.h"
#include "agent_service.h"
#include "memory/i_memory_system.h"
#include "common/constants.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTextStream>

namespace clawpp {

namespace {
bool isTransientEmptyAssistantMessage(const Message& message) {
    return message.role == MessageRole::Assistant
        && message.content.trimmed().isEmpty()
        && message.toolCalls.isEmpty();
}

MessageList normalizeStoredMessages(const MessageList& messages) {
    MessageList sanitized;
    sanitized.reserve(messages.size());

    MessageList pendingGroup;
    QSet<QString> pendingToolIds;
    QSet<QString> resolvedToolIds;

    auto flushPendingGroup = [&](bool keep) mutable {
        if (keep) {
            sanitized.append(pendingGroup);
        }
        pendingGroup.clear();
        pendingToolIds.clear();
        resolvedToolIds.clear();
    };

    for (const Message& rawMessage : messages) {
        Message message = rawMessage;
        bool reprocessCurrent = false;

        do {
            reprocessCurrent = false;

            if (pendingToolIds.isEmpty()) {
                if (isTransientEmptyAssistantMessage(message)) {
                    break;
                }

                if (message.role == MessageRole::Tool) {
                    break;
                }

                if (message.role == MessageRole::Assistant && !message.toolCalls.isEmpty()) {
                    Message toolCallMessage = message;
                    toolCallMessage.toolCalls.clear();

                    QSet<QString> nextPendingIds;
                    for (const ToolCallData& call : message.toolCalls) {
                        const QString callId = call.id.trimmed();
                        if (callId.isEmpty() || nextPendingIds.contains(callId)) {
                            continue;
                        }
                        toolCallMessage.toolCalls.append(call);
                        nextPendingIds.insert(callId);
                    }

                    if (toolCallMessage.toolCalls.isEmpty()) {
                        if (!toolCallMessage.content.trimmed().isEmpty()) {
                            sanitized.append(toolCallMessage);
                        }
                        break;
                    }

                    pendingGroup.clear();
                    pendingGroup.append(toolCallMessage);
                    pendingToolIds = nextPendingIds;
                    resolvedToolIds.clear();
                    break;
                }

                if (!sanitized.isEmpty()
                    && sanitized.last().role == MessageRole::Assistant
                    && message.role == MessageRole::Assistant
                    && sanitized.last().toolCalls.isEmpty()
                    && message.toolCalls.isEmpty()
                    && !message.content.trimmed().isEmpty()
                    && sanitized.last().content.trimmed() == message.content.trimmed()) {
                    break;
                }

                sanitized.append(message);
                break;
            }

            if (message.role == MessageRole::Tool) {
                const QString toolCallId = message.toolCallId.trimmed();
                if (toolCallId.isEmpty()
                    || !pendingToolIds.contains(toolCallId)
                    || resolvedToolIds.contains(toolCallId)) {
                    break;
                }

                pendingGroup.append(message);
                resolvedToolIds.insert(toolCallId);
                if (resolvedToolIds.size() == pendingToolIds.size()) {
                    flushPendingGroup(true);
                }
                break;
            }

            flushPendingGroup(false);
            reprocessCurrent = true;
        } while (reprocessCurrent);
    }

    if (!pendingToolIds.isEmpty()) {
        flushPendingGroup(false);
    }

    return sanitized;
}
}

SessionManager::SessionManager(QObject* parent)
    : QObject(parent)
    , m_sessionStore(new SessionStore(this))
    , m_conversationStore(new ConversationStore(this))
    , m_agentService(nullptr)
    , m_memorySystem(nullptr) {}

void SessionManager::setAgentService(AgentService* service) {
    m_agentService = service;
}

void SessionManager::setMemorySystem(IMemorySystem* memorySystem) {
    m_memorySystem = memorySystem;
}

QString SessionManager::createSession(const QString& name) {
    QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    QString sessionName = name.isEmpty() ? generateDefaultName() : name;
    sessionName = m_sessionStore->findUniqueName(sessionName);
    
    Session session;
    session.id = id;
    session.name = sessionName;
    session.createdAt = QDateTime::currentDateTime();
    session.updatedAt = QDateTime::currentDateTime();
    session.status = Session::Active;
    
    m_sessionStore->save(session);
    
    emit sessionCreated(id, session.name);
    
    return id;
}

QString SessionManager::duplicateSession(const QString& id) {
    Session sourceSession = m_sessionStore->load(id);
    if (sourceSession.id.isEmpty()) {
        emit errorOccurred(QString("Session not found: %1").arg(id));
        return QString();
    }

    MessageList messages = m_conversationStore->loadMessages(id);
    if (messages.isEmpty() && m_currentSession.id == id) {
        messages = m_currentSession.messages;
    }

    Session session;
    session.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString baseName = sourceSession.name.isEmpty() ? generateDefaultName() : sourceSession.name;
    session.name = m_sessionStore->findUniqueName(
        QStringLiteral("%1 %2").arg(baseName, QString::fromUtf8(constants::SESSION_COPY_SUFFIX)));
    session.createdAt = QDateTime::currentDateTime();
    session.updatedAt = session.createdAt;
    session.status = Session::Active;
    session.isPinned = false;
    session.messages = messages;

    m_sessionStore->save(session);
    m_conversationStore->saveMessages(session.id, messages);

    emit sessionCreated(session.id, session.name);
    return session.id;
}

void SessionManager::deleteSession(const QString& id) {
    if (m_currentSession.id == id) {
        if (m_agentService) {
            m_agentService->endSession();
        }
        if (m_memorySystem) {
            m_memorySystem->setSessionContext(QString(), QString());
            m_memorySystem->setContext(MessageList());
        }
        m_currentSession = Session();
    }
    
    m_sessionStore->remove(id);
    m_conversationStore->clearSession(id);
    
    emit sessionDeleted(id);
}

void SessionManager::switchSession(const QString& id) {
    Session session = m_sessionStore->load(id);
    
    if (session.id.isEmpty()) {
        emit errorOccurred(QString("Session not found: %1").arg(id));
        return;
    }
    
    if (m_agentService && m_currentSession.id == id) {
        return;
    }
    
    // 切换会话前先落盘当前状态，避免切换导致历史丢失。
    if (m_agentService && !m_currentSession.id.isEmpty()) {
        saveCurrentSession();
        m_agentService->endSession();
    }
    
    m_currentSession = session;
    m_currentSession.messages = m_conversationStore->loadMessages(id);

    if (m_memorySystem) {
        m_memorySystem->setSessionContext(id, session.name);
        m_memorySystem->setContext(m_currentSession.messages);
    }
    
    if (m_agentService) {
        m_agentService->startSession(id, session.name);
        m_agentService->setHistory(m_currentSession.messages);
    }
    
    emit sessionSwitched(id);
}

Session SessionManager::currentSession() const {
    return m_currentSession;
}

QList<Session> SessionManager::listSessions() const {
    return m_sessionStore->listAll();
}

void SessionManager::renameSession(const QString& id, const QString& name) {
    Session session = m_sessionStore->load(id);
    
    if (session.id.isEmpty()) {
        emit errorOccurred(QString("Session not found: %1").arg(id));
        return;
    }
    
    session.name = name;
    
    m_sessionStore->save(session);
    
    if (m_currentSession.id == id) {
        m_currentSession.name = name;
        if (m_memorySystem) {
            m_memorySystem->setSessionContext(id, name);
        }
    }
    
    emit sessionRenamed(id, name);
}

void SessionManager::archiveSession(const QString& id) {
    Session session = m_sessionStore->load(id);
    
    if (session.id.isEmpty()) {
        return;
    }
    
    session.status = Session::Archived;
    
    m_sessionStore->save(session);
    
    if (m_currentSession.id == id) {
        m_currentSession.status = Session::Archived;
        if (m_memorySystem) {
            m_memorySystem->setSessionContext(id, m_currentSession.name);
        }
    }
    
    emit sessionArchived(id);
}

void SessionManager::setPinned(const QString& id, bool pinned) {
    Session session = m_sessionStore->load(id);

    if (session.id.isEmpty()) {
        emit errorOccurred(QString("Session not found: %1").arg(id));
        return;
    }

    session.isPinned = pinned;
    m_sessionStore->save(session);

    if (m_currentSession.id == id) {
        m_currentSession.isPinned = pinned;
    }
}

void SessionManager::saveCurrentSession() {
    if (m_currentSession.id.isEmpty()) {
        return;
    }

    const QDateTime lastAssistantAt = lastAssistantMessageTime(m_currentSession.messages);
    if (lastAssistantAt.isValid()) {
        m_currentSession.updatedAt = lastAssistantAt;
    } else if (!m_currentSession.updatedAt.isValid()) {
        m_currentSession.updatedAt = m_currentSession.createdAt.isValid()
            ? m_currentSession.createdAt
            : QDateTime::currentDateTime();
    }

    m_sessionStore->save(m_currentSession);
}

void SessionManager::updateMessages(const MessageList& messages) {
    if (m_currentSession.id.isEmpty()) {
        return;
    }

    const MessageList sanitizedMessages = sanitizeMessages(messages);

    m_currentSession.messages = sanitizedMessages;
    const QDateTime lastAssistantAt = lastAssistantMessageTime(sanitizedMessages);
    if (lastAssistantAt.isValid()) {
        m_currentSession.updatedAt = lastAssistantAt;
    } else if (!m_currentSession.createdAt.isValid()) {
        m_currentSession.updatedAt = QDateTime::currentDateTime();
    } else if (!m_currentSession.updatedAt.isValid()) {
        m_currentSession.updatedAt = m_currentSession.createdAt;
    }
    
    m_sessionStore->save(m_currentSession);
    
    // 采用“先清后写”的覆盖策略，确保 DB 与内存历史一致。
    m_conversationStore->clearSession(m_currentSession.id);
    m_conversationStore->saveMessages(m_currentSession.id, sanitizedMessages);

    if (m_memorySystem) {
        m_memorySystem->setSessionContext(m_currentSession.id, m_currentSession.name);
        m_memorySystem->setContext(sanitizedMessages);
    }

    // 外部覆盖会话消息时，同步更新 AgentService 内部历史，避免上下文残留。
    if (m_agentService && !m_agentService->isRunning()) {
        m_agentService->setHistory(sanitizedMessages);
    }
}

void SessionManager::addMessage(const Message& message) {
    m_currentSession.messages.append(message);
    if (message.role == MessageRole::Assistant) {
        m_currentSession.updatedAt = message.timestamp.isValid()
            ? message.timestamp
            : QDateTime::currentDateTime();
    }

    if (m_memorySystem) {
        m_memorySystem->setSessionContext(m_currentSession.id, m_currentSession.name);
        m_memorySystem->addMessage(message);
    }
}

MessageList SessionManager::sanitizeMessages(const MessageList& messages) const {
    return normalizeStoredMessages(messages);
}

bool SessionManager::exportSession(const QString& id, const QString& filePath, QString* errorMessage) const {
    Session session = m_sessionStore->load(id);
    if (session.id.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QString("Session not found: %1").arg(id);
        }
        return false;
    }

    MessageList messages = m_conversationStore->loadMessages(id);
    if (messages.isEmpty() && m_currentSession.id == id) {
        messages = m_currentSession.messages;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = QString("Failed to open file for writing: %1").arg(filePath);
        }
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    const QString suffix = QFileInfo(filePath).suffix().toLower();
    if (suffix == QStringLiteral("md") || suffix == QStringLiteral("markdown")) {
        out << renderMarkdown(session, messages);
    } else if (suffix == QStringLiteral("txt")) {
        out << renderPlainText(session, messages);
    } else {
        QJsonObject root;
        root[QStringLiteral("session_id")] = session.id;
        root[QStringLiteral("name")] = session.name;
        root[QStringLiteral("status")] = session.status == Session::Archived ? "archived" : session.status == Session::Paused ? "paused" : "active";
        root[QStringLiteral("created_at")] = session.createdAt.toString(Qt::ISODate);
        root[QStringLiteral("updated_at")] = session.updatedAt.toString(Qt::ISODate);

        QJsonArray messagesArray;
        for (const Message& message : messages) {
            messagesArray.append(message.toJson());
        }
        root[QStringLiteral("messages")] = messagesArray;

        out << QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
    }

    file.close();
    return true;
}

QString SessionManager::importSession(const QString& filePath, QString* errorMessage) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = QString("Failed to open file for reading: %1").arg(filePath);
        }
        return QString();
    }

    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorMessage) {
            *errorMessage = "Invalid JSON format. Cannot import.";
        }
        return QString();
    }

    QJsonObject root = doc.object();
    QString importedName = root.value(QStringLiteral("name")).toString(QStringLiteral("Imported Session"));
    
    Session session;
    session.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    session.name = m_sessionStore->findUniqueName(importedName);
    session.createdAt = QDateTime::currentDateTime();
    session.updatedAt = QDateTime::currentDateTime();
    session.status = Session::Active;
    session.isPinned = false;

    MessageList messages;
    QJsonArray messagesArray = root.value(QStringLiteral("messages")).toArray();
    for (const QJsonValue& val : messagesArray) {
        if (val.isObject()) {
            messages.append(Message::fromJson(val.toObject()));
        }
    }
    session.messages = messages;

    m_sessionStore->save(session);
    m_conversationStore->saveMessages(session.id, messages);

    emit sessionCreated(session.id, session.name);
    return session.id;
}

QString SessionManager::renderMarkdown(const Session& session, const MessageList& messages) const {
    QStringList lines;
    lines.append(QString("# Session: %1").arg(session.name));
    lines.append(QString("- Session ID: %1").arg(session.id));
    lines.append(QString("- Status: %1").arg(session.status == Session::Archived ? "archived" : session.status == Session::Paused ? "paused" : "active"));
    lines.append(QString("- Created: %1").arg(session.createdAt.toString("yyyy-MM-dd HH:mm:ss")));
    lines.append(QString("- Updated: %1").arg(session.updatedAt.toString("yyyy-MM-dd HH:mm:ss")));
    lines.append(QString());
    lines.append("## Messages");

    for (const Message& message : messages) {
        QString role;
        switch (message.role) {
            case MessageRole::User: role = "User"; break;
            case MessageRole::Assistant: role = "Assistant"; break;
            case MessageRole::System: role = "System"; break;
            case MessageRole::Tool: role = "Tool"; break;
        }

        lines.append(QString("### %1").arg(role));
        if (!message.name.isEmpty()) {
            lines.append(QString("- Name: %1").arg(message.name));
        }
        if (!message.toolCallId.isEmpty()) {
            lines.append(QString("- Tool Call ID: %1").arg(message.toolCallId));
        }
        lines.append(QString());
        lines.append(message.content);
        lines.append(QString());
    }

    return lines.join("\n");
}

QString SessionManager::renderPlainText(const Session& session, const MessageList& messages) const {
    QStringList lines;
    lines.append(QString("Session: %1").arg(session.name));
    lines.append(QString("Session ID: %1").arg(session.id));
    lines.append(QString("Status: %1").arg(session.status == Session::Archived ? "archived" : session.status == Session::Paused ? "paused" : "active"));
    lines.append(QString());

    for (const Message& message : messages) {
        QString role;
        switch (message.role) {
            case MessageRole::User: role = "USER"; break;
            case MessageRole::Assistant: role = "ASSISTANT"; break;
            case MessageRole::System: role = "SYSTEM"; break;
            case MessageRole::Tool: role = "TOOL"; break;
        }

        lines.append(QString("[%1] %2").arg(message.timestamp.toString("yyyy-MM-dd HH:mm:ss"), role));
        lines.append(message.content);
        lines.append(QString());
    }

    return lines.join("\n");
}

QString SessionManager::currentSessionId() const {
    return m_currentSession.id;
}

bool SessionManager::hasCurrentSession() const {
    return !m_currentSession.id.isEmpty();
}

QString SessionManager::generateDefaultName() const {
    return QString("New Chat %1").arg(
        QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"));
}

QDateTime SessionManager::lastAssistantMessageTime(const MessageList& messages) const {
    for (auto it = messages.crbegin(); it != messages.crend(); ++it) {
        if (it->role == MessageRole::Assistant && it->timestamp.isValid()) {
            return it->timestamp;
        }
    }
    return {};
}

}
