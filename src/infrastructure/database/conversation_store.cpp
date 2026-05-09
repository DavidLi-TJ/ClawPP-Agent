#include "conversation_store.h"
#include "database_manager.h"
#include "infrastructure/logging/logger.h"
#include <QSqlQuery>
#include <QSqlError>

namespace clawpp {

namespace {

bool ensureDatabaseReady(const QString& operationName) {
    auto& manager = DatabaseManager::instance();
    QSqlDatabase& db = manager.database();
    if (db.isValid() && db.isOpen()) {
        return true;
    }

    if (manager.initialize()) {
        return true;
    }

    LOG_ERROR(QStringLiteral("ConversationStore operation failed to acquire database connection (%1)")
                  .arg(operationName));
    return false;
}

}

ConversationStore::ConversationStore(QObject* parent)
    : QObject(parent) {}

bool ConversationStore::insert(const QString& sessionId, const Message& message) {
    if (!ensureDatabaseReady(QStringLiteral("insert"))) {
        return false;
    }

    const QString safeContent = message.content.isNull()
        ? QStringLiteral("")
        : message.content;

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(R"(
        INSERT INTO conversations (session_id, role, content, token_count, created_at)
        VALUES (?, ?, ?, ?, ?)
    )");
    
    query.addBindValue(sessionId);
    query.addBindValue(roleToString(message.role));
    query.addBindValue(safeContent);
    query.addBindValue(estimateTokens(safeContent));
    query.addBindValue(message.timestamp.isValid() ? message.timestamp : QDateTime::currentDateTime());

    if (!query.exec()) {
        LOG_ERROR(QStringLiteral("ConversationStore insert failed (session=%1): %2")
                      .arg(sessionId, query.lastError().text()));
        return false;
    }

    return true;
}

MessageList ConversationStore::selectBySession(const QString& sessionId) {
    return loadMessages(sessionId);
}

MessageList ConversationStore::loadMessages(const QString& sessionId) {
    MessageList messages;

    if (!ensureDatabaseReady(QStringLiteral("loadMessages"))) {
        return messages;
    }
    
    QSqlQuery query(DatabaseManager::instance().database());
    // 先按 created_at，再按自增 id，确保同秒写入仍保持稳定顺序。
    query.prepare("SELECT role, content, created_at FROM conversations WHERE session_id = ? ORDER BY created_at ASC, id ASC");
    query.addBindValue(sessionId);

    if (!query.exec()) {
        LOG_ERROR(QStringLiteral("ConversationStore loadMessages failed (session=%1): %2")
                      .arg(sessionId, query.lastError().text()));
        return messages;
    }

    while (query.next()) {
        QString roleStr = query.value(0).toString();
        QString content = query.value(1).toString();
        QDateTime createdAt = query.value(2).toDateTime();

        MessageRole role = stringToRole(roleStr);
        Message message(role, content);
        if (createdAt.isValid()) {
            message.timestamp = createdAt;
        }
        messages.append(message);
    }

    return messages;
}

void ConversationStore::saveMessages(const QString& sessionId, const MessageList& messages) {
    if (!ensureDatabaseReady(QStringLiteral("saveMessages"))) {
        return;
    }

    auto& db = DatabaseManager::instance().database();
    const bool transactionStarted = db.transaction();

    if (!transactionStarted) {
        LOG_ERROR(QStringLiteral("ConversationStore failed to start transaction (session=%1): %2")
                      .arg(sessionId, db.lastError().text()));
        return;
    }

    bool insertFailed = false;

    for (const Message& msg : messages) {
        if (!insert(sessionId, msg)) {
            insertFailed = true;
            break;
        }
    }

    if (insertFailed || !db.commit()) {
        if (!insertFailed) {
            LOG_ERROR(QStringLiteral("ConversationStore commit failed (session=%1): %2")
                          .arg(sessionId, db.lastError().text()));
        }
        if (!db.rollback()) {
            LOG_ERROR(QStringLiteral("ConversationStore rollback failed (session=%1): %2")
                          .arg(sessionId, db.lastError().text()));
        }
    }
}

void ConversationStore::clearSession(const QString& sessionId) {
    if (!ensureDatabaseReady(QStringLiteral("clearSession"))) {
        return;
    }

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("DELETE FROM conversations WHERE session_id = ?");
    query.addBindValue(sessionId);
    if (!query.exec()) {
        LOG_ERROR(QStringLiteral("ConversationStore clearSession failed (session=%1): %2")
                      .arg(sessionId, query.lastError().text()));
    }
}

void ConversationStore::deleteOlderThan(const QString& sessionId, qint64 beforeId) {
    if (!ensureDatabaseReady(QStringLiteral("deleteOlderThan"))) {
        return;
    }

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("DELETE FROM conversations WHERE session_id = ? AND id < ?");
    query.addBindValue(sessionId);
    query.addBindValue(beforeId);
    if (!query.exec()) {
        LOG_ERROR(QStringLiteral("ConversationStore deleteOlderThan failed (session=%1, beforeId=%2): %3")
                      .arg(sessionId)
                      .arg(beforeId)
                      .arg(query.lastError().text()));
    }
}

void ConversationStore::updateCompressed(qint64 id, const QString& summary, const QString& originalIds) {
    if (!ensureDatabaseReady(QStringLiteral("updateCompressed"))) {
        return;
    }

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(R"(
        UPDATE conversations 
        SET content = ?, is_compressed = 1, original_ids = ?
        WHERE id = ?
    )");
    query.addBindValue(summary);
    query.addBindValue(originalIds);
    query.addBindValue(id);
    if (!query.exec()) {
        LOG_ERROR(QStringLiteral("ConversationStore updateCompressed failed (id=%1): %2")
                      .arg(id)
                      .arg(query.lastError().text()));
    }
}

int ConversationStore::sumTokens(const QString& sessionId) {
    if (!ensureDatabaseReady(QStringLiteral("sumTokens"))) {
        return 0;
    }

    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT SUM(token_count) FROM conversations WHERE session_id = ?");
    query.addBindValue(sessionId);

    if (!query.exec()) {
        LOG_ERROR(QStringLiteral("ConversationStore sumTokens query failed (session=%1): %2")
                      .arg(sessionId, query.lastError().text()));
        return 0;
    }

    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int ConversationStore::estimateTokens(const QString& text) {
    int tokens = 0;
    for (const QChar& c : text) {
        if (c.script() == QChar::Script_Han) {
            tokens += 1;
        } else {
            tokens += (c.isSpace() || c.isPunct()) ? 0 : 1;
        }
    }
    return tokens / 3 + 1;
}

QString ConversationStore::roleToString(MessageRole role) const {
    switch (role) {
        case MessageRole::User: return "user";
        case MessageRole::Assistant: return "assistant";
        case MessageRole::System: return "system";
        case MessageRole::Tool: return "tool";
        default: return "user";
    }
}

MessageRole ConversationStore::stringToRole(const QString& str) const {
    if (str == "user") return MessageRole::User;
    if (str == "assistant") return MessageRole::Assistant;
    if (str == "system") return MessageRole::System;
    if (str == "tool") return MessageRole::Tool;
    return MessageRole::User;
}

}
