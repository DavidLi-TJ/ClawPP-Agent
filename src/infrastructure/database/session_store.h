#ifndef CLAWPP_INFRASTRUCTURE_DATABASE_SESSION_STORE_H
#define CLAWPP_INFRASTRUCTURE_DATABASE_SESSION_STORE_H

#include <QObject>
#include <QSqlQuery>
#include "common/types.h"

namespace clawpp {

/**
 * @class SessionStore
 * @brief sessions 表访问层，负责会话元数据的增删改查与命名去重。
 */
class SessionStore : public QObject {
    Q_OBJECT

public:
    explicit SessionStore(QObject* parent = nullptr);
    
    void save(const Session& session);
    Session load(const QString& id);
    QList<Session> listAll();
    void remove(const QString& id);
    bool existsByName(const QString& name);
    QString findUniqueName(const QString& baseName);

private:
    QString statusToString(Session::Status status) const;
    Session::Status stringToStatus(const QString& str) const;
};

}

#endif
