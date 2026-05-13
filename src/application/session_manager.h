#ifndef CLAWPP_APPLICATION_SESSION_MANAGER_H
#define CLAWPP_APPLICATION_SESSION_MANAGER_H

#include <QObject>
#include <QUuid>
#include <QMutex>
#include "common/types.h"

namespace clawpp {

class SessionStore;
class ConversationStore;
class AgentService;
class IMemorySystem;

/**
 * @class SessionManager
 * @brief 统一管理会话生命周期、持久化与当前上下文同步。
 *
 * 该类负责协调：
 * - SessionStore（会话元数据）
 * - ConversationStore（消息存储）
 * - AgentService（运行态）
 * - IMemorySystem（上下文与长期记忆）
 */
class SessionManager : public QObject {
    Q_OBJECT

public:
    explicit SessionManager(QObject* parent = nullptr);
    
    void setAgentService(AgentService* service);
    void setMemorySystem(IMemorySystem* memorySystem);
    
    QString createSession(const QString& name = QString());
    QString duplicateSession(const QString& id);
    void deleteSession(const QString& id);
    void switchSession(const QString& id);
    Session currentSession() const;
    QList<Session> listSessions() const;
    void renameSession(const QString& id, const QString& name);
    void archiveSession(const QString& id);
    void setPinned(const QString& id, bool pinned);
    void saveCurrentSession();
    void updateMessages(const MessageList& messages);
    void addMessage(const Message& message);
    bool exportSession(const QString& id, const QString& filePath, QString* errorMessage = nullptr) const;
    QString importSession(const QString& filePath, QString* errorMessage = nullptr);
    QString currentSessionId() const;
    bool hasCurrentSession() const;

signals:
    void sessionCreated(const QString& id, const QString& name);
    void sessionDeleted(const QString& id);
    void sessionSwitched(const QString& id);
    void sessionRenamed(const QString& id, const QString& name);
    void sessionArchived(const QString& id);
    void errorOccurred(const QString& error);

private:
    MessageList sanitizeMessages(const MessageList& messages) const;
    QString generateDefaultName() const;
    QString renderMarkdown(const Session& session, const MessageList& messages) const;
    QString renderPlainText(const Session& session, const MessageList& messages) const;
    QDateTime lastAssistantMessageTime(const MessageList& messages) const;
    
    SessionStore* m_sessionStore;           // Qt parent 管理
    ConversationStore* m_conversationStore; // Qt parent 管理
    AgentService* m_agentService;           // 外部管理，不拥有所有权
    IMemorySystem* m_memorySystem;          // 外部管理，不拥有所有权
    Session m_currentSession;
    mutable QMutex m_mutex;
};

}

#endif
