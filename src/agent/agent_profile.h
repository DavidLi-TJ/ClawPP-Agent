#ifndef CLAWPP_AGENT_AGENT_PROFILE_H
#define CLAWPP_AGENT_AGENT_PROFILE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>

namespace clawpp {

struct AgentConfig {
    QString name;
    QString displayName;
    QString description;
    int maxIterations = 40;
    double temperature = 0.1;
    int maxTokens = 4096;
    QStringList enabledTools;
    QStringList disabledTools;
    QString defaultModel;
    bool permissionRequired = true;
    
    static AgentConfig fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

class AgentProfile : public QObject {
    Q_OBJECT

public:
    explicit AgentProfile(const QString& profilePath, QObject* parent = nullptr);
    
    bool load();
    bool isValid() const;
    
    QString name() const;
    QString profilePath() const;
    AgentConfig config() const;
    
    QString loadSoul();
    QString loadAgents();
    QString loadTools();
    QString loadMemory();
    QString loadHistory();
    
    QString buildSystemPrompt();
    QString buildToolDefinitions();
    
    static AgentProfile* createDefault(QObject* parent = nullptr);

private:
    QString readFile(const QString& filename) const;
    QString resolveTemplatePath(const QString& filename) const;

    QString m_profilePath;
    QString m_name;
    AgentConfig m_config;
    bool m_loaded;
    
    QString m_soulCache;
    QString m_agentsCache;
    QString m_toolsCache;
};

}

#endif
