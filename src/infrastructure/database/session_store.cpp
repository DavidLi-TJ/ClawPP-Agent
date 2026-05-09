#include "session_store.h"
#include "database_manager.h"
#include <QJsonDocument>

namespace clawpp {

SessionStore::SessionStore(QObject* parent)
    : QObject(parent) {}

void SessionStore::save(const Session& session) {
    // UPSERT：新会话插入，已存在会话则更新最新状态。
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare(R"(
        INSERT INTO sessions (id, name, status, config, created_at, updated_at, is_pinned)
        VALUES (?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(id) DO UPDATE SET
            name = excluded.name,
            status = excluded.status,
            config = excluded.config,
            created_at = excluded.created_at,
            updated_at = excluded.updated_at,
            is_pinned = excluded.is_pinned
    )");
    
    query.addBindValue(session.id);
    query.addBindValue(session.name);
    query.addBindValue(statusToString(session.status));
    query.addBindValue(QJsonDocument(QJsonObject()).toJson(QJsonDocument::Compact));
    query.addBindValue(session.createdAt.isValid() ? session.createdAt : QDateTime::currentDateTime());
    query.addBindValue(session.updatedAt.isValid() ? session.updatedAt : QDateTime::currentDateTime());
    query.addBindValue(session.isPinned ? 1 : 0);
    
    query.exec();
}

Session SessionStore::load(const QString& id) {
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT id, name, status, created_at, updated_at, is_pinned FROM sessions WHERE id = ?");
    query.addBindValue(id);
    
    if (query.exec() && query.next()) {
        Session session;
        session.id = query.value(0).toString();
        session.name = query.value(1).toString();
        session.status = stringToStatus(query.value(2).toString());
        session.createdAt = query.value(3).toDateTime();
        session.updatedAt = query.value(4).toDateTime();
        session.isPinned = query.value(5).toInt() != 0;
        return session;
    }
    
    return Session();
}

QList<Session> SessionStore::listAll() {
    QList<Session> sessions;
    
    QSqlQuery query(DatabaseManager::instance().database());
    // 置顶优先，其次按最近更新时间倒序。
    if (query.exec("SELECT id, name, status, created_at, updated_at, is_pinned FROM sessions ORDER BY is_pinned DESC, updated_at DESC")) {
        while (query.next()) {
            Session session;
            session.id = query.value(0).toString();
            session.name = query.value(1).toString();
            session.status = stringToStatus(query.value(2).toString());
            session.createdAt = query.value(3).toDateTime();
            session.updatedAt = query.value(4).toDateTime();
            session.isPinned = query.value(5).toInt() != 0;
            sessions.append(session);
        }
    }
    
    return sessions;
}

void SessionStore::remove(const QString& id) {
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("DELETE FROM sessions WHERE id = ?");
    query.addBindValue(id);
    query.exec();
}

QString SessionStore::statusToString(Session::Status status) const {
    switch (status) {
        case Session::Active: return "active";
        case Session::Paused: return "paused";
        case Session::Archived: return "archived";
        default: return "active";
    }
}

Session::Status SessionStore::stringToStatus(const QString& str) const {
    if (str == "paused") return Session::Paused;
    if (str == "archived") return Session::Archived;
    return Session::Active;
}

bool SessionStore::existsByName(const QString& name) {
    QSqlQuery query(DatabaseManager::instance().database());
    query.prepare("SELECT COUNT(*) FROM sessions WHERE name = ?");
    query.addBindValue(name);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    return false;
}

QString SessionStore::findUniqueName(const QString& baseName) {
    if (!existsByName(baseName)) {
        return baseName;
    }
    
    int counter = 1;
    QString newName;
    do {
        newName = QString("%1 (%2)").arg(baseName).arg(counter);
        counter++;
    } while (existsByName(newName));
    
    return newName;
}

}
