#ifndef CLAWPP_AGENT_AGENT_PROFILE_MANAGER_H
#define CLAWPP_AGENT_AGENT_PROFILE_MANAGER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QFileSystemWatcher>
#include "agent_profile.h"

namespace clawpp {

class AgentProfileManager : public QObject {
    Q_OBJECT

public:
    static AgentProfileManager* instance();
    
    void initialize(const QString& agentsDirectory);
    void shutdown();
    
    AgentProfile* getProfile(const QString& name);
    AgentProfile* getDefaultProfile();
    QStringList listProfiles() const;
    bool hasProfile(const QString& name) const;
    
    void reloadProfile(const QString& name);
    void reloadAll();
    
    AgentProfile* createProfile(const QString& name, const AgentConfig& config);
    bool deleteProfile(const QString& name);

signals:
    void profileLoaded(const QString& name);
    void profileReloaded(const QString& name);
    void profileCreated(const QString& name);
    void profileDeleted(const QString& name);
    void profileError(const QString& name, const QString& error);

private slots:
    void onDirectoryChanged(const QString& path);
    void onFileChanged(const QString& path);

private:
    explicit AgentProfileManager(QObject* parent = nullptr);
    ~AgentProfileManager();
    
    void loadProfiles();
    void loadProfileFromDirectory(const QString& dirPath);
    void setupWatcher();

    static AgentProfileManager* s_instance;
    
    QMap<QString, AgentProfile*> m_profiles;
    QString m_agentsDirectory;
    QFileSystemWatcher* m_watcher;
    AgentProfile* m_defaultProfile;
};

}

#endif
