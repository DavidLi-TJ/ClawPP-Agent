#ifndef CLAWPP_INFRASTRUCTURE_DATABASE_CONVERSATION_STORE_H
#define CLAWPP_INFRASTRUCTURE_DATABASE_CONVERSATION_STORE_H

#include <QObject>
#include "common/types.h"

namespace clawpp {

/**
 * @class ConversationStore
 * @brief conversations 表访问层，负责消息明细的持久化与查询。
 */
class ConversationStore : public QObject {
    Q_OBJECT

public:
    explicit ConversationStore(QObject* parent = nullptr);
    
    bool insert(const QString& sessionId, const Message& message);
    MessageList selectBySession(const QString& sessionId);
    MessageList loadMessages(const QString& sessionId);
    void saveMessages(const QString& sessionId, const MessageList& messages);
    void clearSession(const QString& sessionId);
    void deleteOlderThan(const QString& sessionId, qint64 beforeId);
    void updateCompressed(qint64 id, const QString& summary, const QString& originalIds);
    int sumTokens(const QString& sessionId);

private:
    int estimateTokens(const QString& text);
    QString roleToString(MessageRole role) const;
    MessageRole stringToRole(const QString& str) const;
};

}

#endif
