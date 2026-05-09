#include "permission_manager.h"
#include <QRegularExpression>

namespace clawpp {

/**
 * @brief PermissionManager 构造函数
 * 初始化缓存（最多 100 条记录）
 */
PermissionManager::PermissionManager()
    : m_cache(100) {}

/**
 * @brief 获取 PermissionManager 单例实例
 * @return 权限管理器实例
 */
PermissionManager& PermissionManager::instance() {
    static PermissionManager instance;
    return instance;
}

/**
 * @brief 初始化权限管理器
 * @param config 权限配置
 * 
 * 设置默认配置和规则列表
 */
void PermissionManager::initialize(const PermissionConfig& config) {
    m_config = config;
    m_rules = config.rules;
    m_cache.clear();
    m_sessionWhitelist.clear();
}

/**
 * @brief 检查工具调用权限
 * @param toolCall 工具调用信息
 * @return 权限检查结果
 * 
 * 检查流程：
 * 1. 先查缓存
 * 2. 匹配规则列表
 * 3. 应用默认动作
 * 4. 结果缓存
 */
PermissionResult PermissionManager::checkPermission(const ToolCall& toolCall) {
    return checkPermission(QString(), toolCall);
}

PermissionResult PermissionManager::checkPermission(const QString& sessionId, const ToolCall& toolCall) {
    QString key = sessionId.trimmed() + "::" + toolCall.name + ":" + toolCall.path;

    if (m_cache.contains(key)) {
        return *m_cache.object(key);
    }

    if (isWhitelisted(sessionId, toolCall)) {
        PermissionResult result = PermissionResult::Allowed;
        m_cache.insert(key, new PermissionResult(result));
        return result;
    }

    for (const auto& rule : m_rules) {
        if (matchesPattern(rule.pattern, toolCall)) {
            if (rule.tools.isEmpty() || rule.tools.contains(toolCall.name)) {
                PermissionResult result = actionToResult(rule.action);
                m_cache.insert(key, new PermissionResult(result));
                return result;
            }
        }
    }

    PermissionResult result = actionToResult(m_config.defaultAction);
    m_cache.insert(key, new PermissionResult(result));
    return result;
}

/**
 * @brief 添加权限规则
 * @param pattern 匹配模式
 * @param action 动作（允许/拒绝/询问）
 * @param tools 适用的工具列表
 */
void PermissionManager::addRule(const QString& pattern, PermissionRule::Action action, 
                                 const QStringList& tools) {
    PermissionRule rule;
    rule.pattern = pattern;
    rule.action = action;
    rule.tools = tools;
    m_rules.append(rule);
    m_cache.clear();  // 清空缓存
}

/**
 * @brief 删除权限规则
 * @param pattern 要删除的模式
 */
void PermissionManager::removeRule(const QString& pattern) {
    m_rules.removeIf([&pattern](const PermissionRule& r) {
        return r.pattern == pattern;
    });
    m_cache.clear();
}

/**
 * @brief 清空权限缓存
 */
void PermissionManager::clearCache() {
    m_cache.clear();
}

/**
 * @brief 添加到白名单
 * @param sessionId 会话 ID
 * @param pattern 模式
 */
void PermissionManager::addToWhitelist(const QString& sessionId, const QString& pattern) {
    QString normalizedSessionId = sessionId.trimmed();
    QString normalizedPattern = pattern.trimmed();
    if (normalizedPattern.isEmpty()) {
        return;
    }

    QStringList& patterns = m_sessionWhitelist[normalizedSessionId];
    if (!patterns.contains(normalizedPattern)) {
        patterns.append(normalizedPattern);
    }

    m_cache.clear();
    emit whitelistAdded(sessionId, pattern);
}

bool PermissionManager::isWhitelisted(const QString& sessionId, const ToolCall& toolCall) const {
    const QString normalizedSessionId = sessionId.trimmed();

    auto matchesAny = [&](const QStringList& patterns) {
        for (const QString& pattern : patterns) {
            if (matchesPattern(pattern, toolCall)) {
                return true;
            }
        }
        return false;
    };

    if (m_sessionWhitelist.contains(normalizedSessionId) && matchesAny(m_sessionWhitelist.value(normalizedSessionId))) {
        return true;
    }

    if (m_sessionWhitelist.contains(QString()) && matchesAny(m_sessionWhitelist.value(QString()))) {
        return true;
    }

    return false;
}

/**
 * @brief 检查是否匹配模式
 * @param pattern 模式字符串
 * @param toolCall 工具调用
 * @return 是否匹配
 * 
 * 将通配符模式转换为正则表达式进行匹配
 */
bool PermissionManager::matchesPattern(const QString& pattern, const ToolCall& toolCall) const {
    if (pattern.trimmed().isEmpty()) {
        return false;
    }

    // 统一使用 "tool:path" 作为匹配目标，便于规则同时约束工具名与路径。
    QString target = toolCall.name + ":" + toolCall.path;
    QRegularExpression regex = QRegularExpression(wildcardToRegex(pattern));
    return regex.match(target).hasMatch();
}

/**
 * @brief 将通配符转换为正则表达式
 * @param s 通配符字符串
 * @return 正则表达式字符串
 * 
 * 支持：
 * - * 匹配任意字符
 * - ? 匹配单个字符
 */
QString PermissionManager::wildcardToRegex(const QString& s) const {
    QString pattern = QRegularExpression::escape(s);
    pattern.replace("\\*", ".*");
    pattern.replace("\\?", ".");
    return "^" + pattern + "$";
}

/**
 * @brief 将规则动作转换为权限结果
 * @param action 规则动作
 * @return 权限检查结果
 */
PermissionResult PermissionManager::actionToResult(PermissionRule::Action action) const {
    switch (action) {
        case PermissionRule::Action::Allow: return PermissionResult::Allowed;
        case PermissionRule::Action::Deny: return PermissionResult::Denied;
        case PermissionRule::Action::Ask: return PermissionResult::NeedConfirmation;
        default: return PermissionResult::NeedConfirmation;
    }
}

}
