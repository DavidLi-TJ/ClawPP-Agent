#include "config_manager.h"
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QStringList>
#include "common/constants.h"

namespace clawpp {

ConfigManager& ConfigManager::instance() {
    // C++11 起，函数内静态对象初始化具备线程安全保证。
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::load() {
    QString configPath = getConfigPath();
    
    QFile file(configPath);
    if (!file.exists()) {
        // 首次启动时配置文件不存在，直接写入默认配置到内存。
        loadDefaults();
        return true;
    }
    
    if (!file.open(QIODevice::ReadOnly)) {
        loadDefaults();
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        // 配置损坏时回退默认值，避免启动失败。
        loadDefaults();
        return false;
    }
    
    m_config = doc.object();
    QJsonObject provider = m_config.value(QStringLiteral("provider")).toObject();
    if (provider.contains(QStringLiteral("custom_base_urls"))) {
        provider.remove(QStringLiteral("custom_base_urls"));
        m_config.insert(QStringLiteral("provider"), provider);
    }
    return true;
}

bool ConfigManager::save() {
    QString configPath = getConfigPath();
    
    QDir dir = QFileInfo(configPath).dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QFile file(configPath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    QJsonDocument doc(m_config);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

ProviderConfig ConfigManager::getProviderConfig() const {
    ProviderConfig config;
    
    QJsonObject provider = m_config["provider"].toObject();
    config.type = provider["type"].toString("openai");
    config.apiKey = provider["api_key"].toString();
    config.baseUrl = provider["base_url"].toString(constants::DEFAULT_BASE_URL);
    config.model = provider["model"].toString(constants::DEFAULT_MODEL);
    config.temperature = provider["temperature"].toDouble(constants::DEFAULT_TEMPERATURE);
    config.maxTokens = provider["max_tokens"].toInt(constants::DEFAULT_MAX_TOKENS);
    
    QJsonObject network = m_config["network"].toObject();
    config.timeoutMs = network["timeout_ms"].toInt(constants::DEFAULT_TIMEOUT_MS);
    
    return config;
}

MemoryConfig ConfigManager::getMemoryConfig() const {
    MemoryConfig config;
    
    QJsonObject memory = m_config["memory"].toObject();
    config.tokenThreshold = memory["token_threshold"].toInt(constants::MEMORY_TOKEN_THRESHOLD);
    config.keepFirst = memory["keep_first"].toInt(constants::MEMORY_KEEP_FIRST);
    config.keepLast = memory["keep_last"].toInt(constants::MEMORY_KEEP_LAST);
    config.summaryRatio = memory["summary_ratio"].toDouble(0.3);
    config.retrievalTopK = memory["retrieval_top_k"].toInt(6);
    config.embeddingDimension = memory["embedding_dimension"].toInt(64);
    config.optimizationThreshold = memory["optimization_threshold"].toDouble(0.92);
    
    return config;
}

PermissionConfig ConfigManager::getPermissionConfig() const {
    PermissionConfig config;
    
    QJsonObject permission = m_config["permission"].toObject();
    QString actionStr = permission["default_action"].toString("ask");
    if (actionStr == "allow") {
        config.defaultAction = PermissionRule::Action::Allow;
    } else if (actionStr == "deny") {
        config.defaultAction = PermissionRule::Action::Deny;
    } else {
        config.defaultAction = PermissionRule::Action::Ask;
    }
    config.timeoutSeconds = permission["timeout_seconds"].toInt(60);
    
    return config;
}

AppConfig ConfigManager::getAppConfig() const {
    AppConfig config;
    config.provider = getProviderConfig();
    config.memory = getMemoryConfig();
    config.permission = getPermissionConfig();
    return config;
}

void ConfigManager::setApiKey(const QString& apiKey) {
    QJsonObject provider = m_config["provider"].toObject();
    provider["api_key"] = apiKey;
    m_config["provider"] = provider;
    save();
}

void ConfigManager::setProviderType(const QString& type) {
    QJsonObject provider = m_config["provider"].toObject();
    const QString normalized = type.trimmed().toLower();
    provider["type"] = normalized.isEmpty() ? QStringLiteral("openai") : normalized;
    m_config["provider"] = provider;
    save();
}

void ConfigManager::setBaseUrl(const QString& baseUrl) {
    QJsonObject provider = m_config["provider"].toObject();
    provider["base_url"] = baseUrl;
    m_config["provider"] = provider;
    save();
}

void ConfigManager::setModel(const QString& model) {
    QJsonObject provider = m_config["provider"].toObject();
    provider["model"] = model;
    m_config["provider"] = provider;
    save();
}

QString ConfigManager::getApiKey() const {
    QJsonObject provider = m_config["provider"].toObject();
    return provider["api_key"].toString();
}

QString ConfigManager::getProviderType() const {
    QJsonObject provider = m_config["provider"].toObject();
    const QString type = provider["type"].toString(QStringLiteral("openai")).trimmed().toLower();
    return type.isEmpty() ? QStringLiteral("openai") : type;
}

QString ConfigManager::getBaseUrl() const {
    QJsonObject provider = m_config["provider"].toObject();
    return provider["base_url"].toString(constants::DEFAULT_BASE_URL);
}

QString ConfigManager::getModel() const {
    QJsonObject provider = m_config["provider"].toObject();
    return provider["model"].toString(constants::DEFAULT_MODEL);
}

QString ConfigManager::getTheme() const {
    QJsonObject ui = m_config["ui"].toObject();
    return ui["theme"].toString("default");
}

QString ConfigManager::getTelegramToken() const {
    return m_config.value("external").toObject().value("telegram_token").toString();
}

void ConfigManager::setTelegramToken(const QString& token) {
    QJsonObject extConfig = m_config.value("external").toObject();
    extConfig["telegram_token"] = token;
    m_config["external"] = extConfig;
    save();
}

QString ConfigManager::getFeishuAppId() const {
    QJsonObject external = m_config.value("external").toObject();
    QString appId = external.value("feishu_app_id").toString();
    if (!appId.isEmpty()) {
        return appId;
    }

    const QString legacyToken = external.value("feishu_token").toString();
    const QStringList parts = legacyToken.split(':');
    return parts.isEmpty() ? QString() : parts.first();
}

void ConfigManager::setFeishuAppId(const QString& appId) {
    QJsonObject extConfig = m_config.value("external").toObject();
    extConfig["feishu_app_id"] = appId;
    extConfig.remove("feishu_token");
    m_config["external"] = extConfig;
    save();
}

QString ConfigManager::getFeishuAppSecret() const {
    QJsonObject external = m_config.value("external").toObject();
    QString appSecret = external.value("feishu_app_secret").toString();
    if (!appSecret.isEmpty()) {
        return appSecret;
    }

    const QString legacyToken = external.value("feishu_token").toString();
    const QStringList parts = legacyToken.split(':');
    if (parts.size() >= 2) {
        return parts.mid(1).join(":");
    }

    return QString();
}

void ConfigManager::setFeishuAppSecret(const QString& appSecret) {
    QJsonObject extConfig = m_config.value("external").toObject();
    extConfig["feishu_app_secret"] = appSecret;
    extConfig.remove("feishu_token");
    m_config["external"] = extConfig;
    save();
}

QString ConfigManager::getFeishuVerificationToken() const {
    return m_config.value("external").toObject().value("feishu_verification_token").toString();
}

void ConfigManager::setFeishuVerificationToken(const QString& verificationToken) {
    QJsonObject extConfig = m_config.value("external").toObject();
    extConfig["feishu_verification_token"] = verificationToken;
    extConfig.remove("feishu_token");
    m_config["external"] = extConfig;
    save();
}

int ConfigManager::getFeishuPort() const {
    return m_config.value("external").toObject().value("feishu_port").toInt(8080);
}

void ConfigManager::setFeishuPort(int port) {
    QJsonObject extConfig = m_config.value("external").toObject();
    extConfig["feishu_port"] = port;
    m_config["external"] = extConfig;
    save();
}

QString ConfigManager::getExternalValue(const QString& key) const {
    return m_config.value("external").toObject().value(key).toString();
}

void ConfigManager::setExternalValue(const QString& key, const QString& value) {
    const QString cleanedKey = key.trimmed();
    if (cleanedKey.isEmpty()) {
        return;
    }
    QJsonObject extConfig = m_config.value("external").toObject();
    extConfig[cleanedKey] = value;
    m_config["external"] = extConfig;
    save();
}

QString ConfigManager::getConfigPath() const {
    QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    return homeDir + "/" + constants::USER_CONFIG_DIR + "/" + constants::USER_CONFIG_FILE;
}

void ConfigManager::loadDefaults() {
    m_config = QJsonObject{
        {"provider", QJsonObject{
            {"type", "openai"},
            {"api_key", ""},
            {"base_url", constants::DEFAULT_BASE_URL},
            {"model", constants::DEFAULT_MODEL},
            {"temperature", constants::DEFAULT_TEMPERATURE},
            {"max_tokens", constants::DEFAULT_MAX_TOKENS}
        }},
        {"network", QJsonObject{
            {"timeout_ms", constants::DEFAULT_TIMEOUT_MS}
        }},
        {"memory", QJsonObject{
            {"token_threshold", constants::MEMORY_TOKEN_THRESHOLD},
            {"keep_first", constants::MEMORY_KEEP_FIRST},
            {"keep_last", constants::MEMORY_KEEP_LAST},
            {"summary_ratio", 0.3},
            {"retrieval_top_k", 6},
            {"embedding_dimension", 64},
            {"optimization_threshold", 0.92}
        }},
        {"permission", QJsonObject{
            {"default_action", "ask"},
            {"timeout_seconds", 60}
        }},
        {"ui", QJsonObject{
            {"theme", "default"}
        }},
        {"external", QJsonObject{
            {"telegram_token", ""},
            {"feishu_app_id", ""},
            {"feishu_app_secret", ""},
            {"feishu_verification_token", ""},
            {"feishu_port", 8080},
            {"message_channel", "telegram"}
        }}
    };
}

}
