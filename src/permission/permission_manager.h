#ifndef CLAWPP_PERMISSION_PERMISSION_MANAGER_H
#define CLAWPP_PERMISSION_PERMISSION_MANAGER_H

#include <QObject>
#include <QList>
#include <QCache>
#include <QHash>
#include <functional>
#include "common/types.h"

namespace clawpp {

/**
 * @class PermissionManager
 * @brief 工具调用权限判断器，支持规则匹配、会话白名单与结果缓存。
 */
class PermissionManager : public QObject {
    Q_OBJECT

public:
    static PermissionManager& instance();
    
    void initialize(const PermissionConfig& config);
    PermissionResult checkPermission(const ToolCall& toolCall);
    PermissionResult checkPermission(const QString& sessionId, const ToolCall& toolCall);
    void addRule(const QString& pattern, PermissionRule::Action action, 
                 const QStringList& tools = {});
    void removeRule(const QString& pattern);
    void clearCache();
    void addToWhitelist(const QString& sessionId, const QString& pattern);

signals:
    void permissionRequest(const ToolCall& toolCall, std::function<void(bool)> callback);
    void whitelistAdded(const QString& sessionId, const QString& pattern);

private:
    PermissionManager();
    PermissionManager(const PermissionManager&) = delete;
    PermissionManager& operator=(const PermissionManager&) = delete;
    
    bool isWhitelisted(const QString& sessionId, const ToolCall& toolCall) const;
    bool matchesPattern(const QString& pattern, const ToolCall& toolCall) const;
    QString wildcardToRegex(const QString& s) const;
    PermissionResult actionToResult(PermissionRule::Action action) const;
    
    PermissionConfig m_config;
    QList<PermissionRule> m_rules;
    QCache<QString, PermissionResult> m_cache;
    QHash<QString, QStringList> m_sessionWhitelist;
};

}

#endif
