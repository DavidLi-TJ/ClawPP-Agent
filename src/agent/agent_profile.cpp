#include "agent_profile.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFileInfo>

namespace clawpp {

AgentConfig AgentConfig::fromJson(const QJsonObject& json) {
    AgentConfig config;
    config.name = json["name"].toString();
    config.displayName = json["displayName"].toString();
    config.description = json["description"].toString();
    config.maxIterations = json["maxIterations"].toInt(40);
    config.temperature = json["temperature"].toDouble(0.1);
    config.maxTokens = json["maxTokens"].toInt(4096);
    config.defaultModel = json["defaultModel"].toString();
    config.permissionRequired = json["permissionRequired"].toBool(true);
    
    if (json.contains("enabledTools")) {
        QJsonArray tools = json["enabledTools"].toArray();
        for (const auto& tool : tools) {
            config.enabledTools.append(tool.toString());
        }
    }
    
    if (json.contains("disabledTools")) {
        QJsonArray tools = json["disabledTools"].toArray();
        for (const auto& tool : tools) {
            config.disabledTools.append(tool.toString());
        }
    }
    
    return config;
}

QJsonObject AgentConfig::toJson() const {
    QJsonObject json;
    json["name"] = name;
    json["displayName"] = displayName;
    json["description"] = description;
    json["maxIterations"] = maxIterations;
    json["temperature"] = temperature;
    json["maxTokens"] = maxTokens;
    json["defaultModel"] = defaultModel;
    json["permissionRequired"] = permissionRequired;
    
    QJsonArray enabled;
    for (const QString& tool : enabledTools) {
        enabled.append(tool);
    }
    json["enabledTools"] = enabled;
    
    QJsonArray disabled;
    for (const QString& tool : disabledTools) {
        disabled.append(tool);
    }
    json["disabledTools"] = disabled;
    
    return json;
}

AgentProfile::AgentProfile(const QString& profilePath, QObject* parent)
    : QObject(parent)
    , m_profilePath(profilePath)
    , m_loaded(false) {
    QFileInfo info(profilePath);
    m_name = info.fileName();
}

bool AgentProfile::load() {
    QString configPath = m_profilePath + "/config.json";
    
    QFile configFile(configPath);
    if (configFile.exists()) {
        if (configFile.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(configFile.readAll());
            configFile.close();
            
            if (doc.isObject()) {
                m_config = AgentConfig::fromJson(doc.object());
                m_name = m_config.name.isEmpty() ? m_name : m_config.name;
            }
        }
    }
    
    m_soulCache = loadSoul();
    m_agentsCache = loadAgents();
    m_toolsCache = loadTools();
    
    m_loaded = true;
    return true;
}

bool AgentProfile::isValid() const {
    return m_loaded && !m_name.isEmpty();
}

QString AgentProfile::name() const {
    return m_name;
}

QString AgentProfile::profilePath() const {
    return m_profilePath;
}

AgentConfig AgentProfile::config() const {
    return m_config;
}

QString AgentProfile::resolveTemplatePath(const QString& filename) const {
    QString profileFile = m_profilePath + "/" + filename;
    if (QFile::exists(profileFile)) {
        return profileFile;
    }
    
    QString globalFile = "templates/" + filename;
    if (QFile::exists(globalFile)) {
        return globalFile;
    }
    
    return QString();
}

QString AgentProfile::readFile(const QString& filename) const {
    QString path = resolveTemplatePath(filename);
    if (path.isEmpty()) {
        return QString();
    }
    
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    
    QString content = QString::fromUtf8(file.readAll());
    file.close();
    return content;
}

QString AgentProfile::loadSoul() {
    if (!m_soulCache.isEmpty()) {
        return m_soulCache;
    }
    return readFile("SOUL.md");
}

QString AgentProfile::loadAgents() {
    if (!m_agentsCache.isEmpty()) {
        return m_agentsCache;
    }
    return readFile("AGENTS.md");
}

QString AgentProfile::loadTools() {
    if (!m_toolsCache.isEmpty()) {
        return m_toolsCache;
    }
    return readFile("TOOLS.md");
}

QString AgentProfile::loadMemory() {
    return readFile("memory/MEMORY.md");
}

QString AgentProfile::loadHistory() {
    return readFile("memory/HISTORY.md");
}

QString AgentProfile::buildSystemPrompt() {
    QStringList parts;
    
    QString soul = loadSoul();
    if (!soul.isEmpty()) {
        parts.append("=== 核心身份 ===");
        parts.append(soul);
    }
    
    QString agents = loadAgents();
    if (!agents.isEmpty()) {
        parts.append("\n=== 行为规范 ===");
        parts.append(agents);
    }
    
    QString tools = loadTools();
    if (!tools.isEmpty()) {
        parts.append("\n=== 工具说明 ===");
        parts.append(tools);
    }
    
    return parts.join("\n");
}

QString AgentProfile::buildToolDefinitions() {
    QString tools = loadTools();
    return tools;
}

AgentProfile* AgentProfile::createDefault(QObject* parent) {
    AgentProfile* profile = new AgentProfile("templates", parent);
    profile->m_name = "default";
    profile->m_config.name = "default";
    profile->m_config.displayName = "Default Agent";
    profile->m_config.description = "Default CpQClaw Agent";
    profile->m_config.maxIterations = 40;
    profile->m_config.temperature = 0.1;
    profile->m_config.maxTokens = 4096;
    profile->m_config.permissionRequired = true;
    profile->m_loaded = true;
    return profile;
}

}
