#ifndef CLAWPP_INFRASTRUCTURE_CONFIG_CONFIG_MANAGER_H
#define CLAWPP_INFRASTRUCTURE_CONFIG_CONFIG_MANAGER_H

#include <QObject>
#include <QJsonObject>
#include <QStringList>
#include "common/types.h"

namespace clawpp {

/**
 * @class ConfigManager
 * @brief 管理用户配置文件的加载、默认回退与写回。
 *
 * 配置来源优先级：
 * 1) 用户目录 ~/.clawpp/config.json
 * 2) 若不存在或损坏，回退到编译期默认值
 */
class ConfigManager : public QObject {
    Q_OBJECT

public:
    static ConfigManager& instance();
    
    bool load();
    bool save();
    
    ProviderConfig getProviderConfig() const;
    MemoryConfig getMemoryConfig() const;
    PermissionConfig getPermissionConfig() const;
    AppConfig getAppConfig() const;
    
    QString getTelegramToken() const;
    void setTelegramToken(const QString& token);
    
    QString getFeishuAppId() const;
    void setFeishuAppId(const QString& appId);
    
    QString getFeishuAppSecret() const;
    void setFeishuAppSecret(const QString& appSecret);
    
    QString getFeishuVerificationToken() const;
    void setFeishuVerificationToken(const QString& verificationToken);
    
    int getFeishuPort() const;
    void setFeishuPort(int port);
    QString getExternalValue(const QString& key) const;
    void setExternalValue(const QString& key, const QString& value);
    
    void setApiKey(const QString& apiKey);
    void setProviderType(const QString& type);
    void setBaseUrl(const QString& baseUrl);
    void setModel(const QString& model);
    
    QString getApiKey() const;
    QString getProviderType() const;
    QString getBaseUrl() const;
    QString getModel() const;
    QString getTheme() const;

private:
    ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    QString getConfigPath() const;
    void loadDefaults();
    
    QJsonObject m_config;
};

}

#endif
