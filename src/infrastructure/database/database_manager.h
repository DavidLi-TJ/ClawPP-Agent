#ifndef CLAWPP_INFRASTRUCTURE_DATABASE_DATABASE_MANAGER_H
#define CLAWPP_INFRASTRUCTURE_DATABASE_DATABASE_MANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QStandardPaths>
#include <QDir>
#include "common/result.h"

namespace clawpp {

/**
 * @class DatabaseManager
 * @brief 数据库管理器
 * 
 * 单例模式管理 SQLite 数据库连接
 * 负责初始化数据库、创建表、执行 SQL
 * 
 * 数据库文件位置：~/.clawpp/data.db
 */
class DatabaseManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     * @return 数据库管理器实例
     */
    static DatabaseManager& instance();

    bool initialize();           ///< 初始化数据库
    void close();                ///< 关闭数据库
    QSqlDatabase& database();    ///< 获取数据库连接
    bool execute(const QString& sql);  ///< 执行 SQL

private:
    static constexpr const char* kConnectionName = "clawpp_main";

    DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    QString getDatabasePath() const;  ///< 获取数据库文件路径
    bool createTables();              ///< 创建表

    QSqlDatabase m_db;  ///< 数据库连接
};

}

#endif
