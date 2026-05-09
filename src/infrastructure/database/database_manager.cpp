#include "database_manager.h"
#include "infrastructure/logging/logger.h"

namespace clawpp {

namespace {

bool columnExists(const QSqlDatabase& db, const QString& tableName, const QString& columnName) {
    QSqlQuery query(db);
    if (!query.exec(QString("PRAGMA table_info(%1)").arg(tableName))) {
        LOG_ERROR(QStringLiteral("DatabaseManager PRAGMA table_info failed (%1): %2")
                      .arg(tableName, query.lastError().text()));
        return false;
    }

    while (query.next()) {
        if (query.value(1).toString() == columnName) {
            return true;
        }
    }
    return false;
}

}

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::DatabaseManager() = default;

bool DatabaseManager::initialize() {
    if (m_db.isValid() && m_db.isOpen()) {
        return true;
    }

    QString dbPath = getDatabasePath();

    QDir dir = QFileInfo(dbPath).dir();
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            LOG_ERROR(QStringLiteral("DatabaseManager failed to create database directory: %1")
                          .arg(dir.absolutePath()));
            return false;
        }
    }

    // 使用固定连接名，避免多次 initialize 重复 addDatabase 导致连接污染。
    if (QSqlDatabase::contains(QString::fromUtf8(kConnectionName))) {
        m_db = QSqlDatabase::database(QString::fromUtf8(kConnectionName));
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE", QString::fromUtf8(kConnectionName));
    }
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        LOG_ERROR(QStringLiteral("DatabaseManager failed to open database %1: %2")
                      .arg(dbPath, m_db.lastError().text()));
        return false;
    }

    return createTables();
}

void DatabaseManager::close() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

QSqlDatabase& DatabaseManager::database() {
    return m_db;
}

bool DatabaseManager::execute(const QString& sql) {
    QSqlQuery query(m_db);
    if (!query.exec(sql)) {
        QString compactSql = sql.simplified();
        if (compactSql.length() > 160) {
            compactSql = compactSql.left(160) + QStringLiteral("...");
        }
        LOG_ERROR(QStringLiteral("DatabaseManager SQL exec failed: %1 | error: %2")
                      .arg(compactSql, query.lastError().text()));
        return false;
    }
    return true;
}

QString DatabaseManager::getDatabasePath() const {
    QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    return homeDir + "/.clawpp/data/clawpp.db";
}

bool DatabaseManager::createTables() {
    // conversations: 会话消息明细；sessions: 会话元数据；permission_whitelist: 权限白名单。
    QString conversationsSql = R"(
        CREATE TABLE IF NOT EXISTS conversations (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            session_id TEXT NOT NULL,
            role TEXT NOT NULL,
            content TEXT NOT NULL,
            token_count INTEGER DEFAULT 0,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            is_compressed BOOLEAN DEFAULT 0,
            original_ids TEXT
        )
    )";

    QString sessionsSql = R"(
        CREATE TABLE IF NOT EXISTS sessions (
            id TEXT PRIMARY KEY,
            name TEXT,
            status TEXT DEFAULT 'active',
            config TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";

    QString permissionsSql = R"(
        CREATE TABLE IF NOT EXISTS permission_whitelist (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            session_id TEXT NOT NULL,
            pattern TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";

    bool ok = execute(conversationsSql) && execute(sessionsSql) && execute(permissionsSql);

    // 轻量迁移：补齐旧版本 sessions 缺失的 is_pinned 字段。
    if (ok && !columnExists(m_db, QStringLiteral("sessions"), QStringLiteral("is_pinned"))) {
        ok = execute(QStringLiteral("ALTER TABLE sessions ADD COLUMN is_pinned BOOLEAN DEFAULT 0"));
    }

    return ok;
}

}
