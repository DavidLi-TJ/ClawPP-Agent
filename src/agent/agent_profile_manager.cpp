#include "agent_profile_manager.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace clawpp {

AgentProfileManager* AgentProfileManager::s_instance = nullptr;

AgentProfileManager* AgentProfileManager::instance() {
    if (!s_instance) {
        s_instance = new AgentProfileManager();
    }
    return s_instance;
}

AgentProfileManager::AgentProfileManager(QObject* parent)
    : QObject(parent)
    , m_watcher(nullptr)
    , m_defaultProfile(nullptr) {
}

AgentProfileManager::~AgentProfileManager() {
    shutdown();
}

void AgentProfileManager::initialize(const QString& agentsDirectory) {
    if (!m_profiles.isEmpty() || m_watcher || m_defaultProfile) {
        shutdown();
    }

    m_agentsDirectory = agentsDirectory;
    
    m_defaultProfile = AgentProfile::createDefault(this);
    m_profiles["default"] = m_defaultProfile;
    
    loadProfiles();
    setupWatcher();
}

void AgentProfileManager::shutdown() {
    if (m_watcher) {
        delete m_watcher;
        m_watcher = nullptr;
    }
    
    for (auto it = m_profiles.begin(); it != m_profiles.end(); ++it) {
        if (it.value() != m_defaultProfile) {
            delete it.value();
        }
    }
    m_profiles.clear();
    
    if (m_defaultProfile) {
        delete m_defaultProfile;
        m_defaultProfile = nullptr;
    }
}

void AgentProfileManager::loadProfiles() {
    if (m_agentsDirectory.isEmpty() || !QDir(m_agentsDirectory).exists()) {
        return;
    }
    
    QDir dir(m_agentsDirectory);
    QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    
    for (const QString& subdir : subdirs) {
        QString profilePath = m_agentsDirectory + "/" + subdir;
        loadProfileFromDirectory(profilePath);
    }
}

void AgentProfileManager::loadProfileFromDirectory(const QString& dirPath) {
    QDir dir(dirPath);
    QString profileName = dir.dirName();
    
    if (profileName == "default" || m_profiles.contains(profileName)) {
        return;
    }
    
    AgentProfile* profile = new AgentProfile(dirPath, this);
    if (profile->load() && profile->isValid()) {
        m_profiles[profileName] = profile;
        emit profileLoaded(profileName);
    } else {
        delete profile;
    }
}

void AgentProfileManager::setupWatcher() {
    if (m_agentsDirectory.isEmpty()) {
        return;
    }
    
    m_watcher = new QFileSystemWatcher(this);
    m_watcher->addPath(m_agentsDirectory);
    
    QDir dir(m_agentsDirectory);
    QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& subdir : subdirs) {
        m_watcher->addPath(m_agentsDirectory + "/" + subdir);
    }
    
    connect(m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &AgentProfileManager::onDirectoryChanged);
    connect(m_watcher, &QFileSystemWatcher::fileChanged,
            this, &AgentProfileManager::onFileChanged);
}

AgentProfile* AgentProfileManager::getProfile(const QString& name) {
    if (m_profiles.contains(name)) {
        return m_profiles[name];
    }
    return getDefaultProfile();
}

AgentProfile* AgentProfileManager::getDefaultProfile() {
    return m_defaultProfile;
}

QStringList AgentProfileManager::listProfiles() const {
    return m_profiles.keys();
}

bool AgentProfileManager::hasProfile(const QString& name) const {
    return m_profiles.contains(name);
}

void AgentProfileManager::reloadProfile(const QString& name) {
    if (!m_profiles.contains(name)) {
        return;
    }
    
    AgentProfile* profile = m_profiles[name];
    if (profile == m_defaultProfile) {
        return;
    }
    
    profile->load();
    emit profileReloaded(name);
}

void AgentProfileManager::reloadAll() {
    for (auto it = m_profiles.begin(); it != m_profiles.end(); ++it) {
        if (it.value() != m_defaultProfile) {
            it.value()->load();
            emit profileReloaded(it.key());
        }
    }
}

AgentProfile* AgentProfileManager::createProfile(const QString& name, const AgentConfig& config) {
    if (m_profiles.contains(name)) {
        return nullptr;
    }
    
    QString profilePath = m_agentsDirectory + "/" + name;
    QDir dir;
    if (!dir.mkpath(profilePath)) {
        emit profileError(name, "Failed to create profile directory");
        return nullptr;
    }
    
    QFile configFile(profilePath + "/config.json");
    if (!configFile.open(QIODevice::WriteOnly)) {
        emit profileError(name, "Failed to create config file");
        return nullptr;
    }
    
    AgentConfig newConfig = config;
    newConfig.name = name;
    
    QJsonDocument doc(newConfig.toJson());
    configFile.write(doc.toJson());
    configFile.close();
    
    AgentProfile* profile = new AgentProfile(profilePath, this);
    profile->load();
    
    m_profiles[name] = profile;
    
    if (m_watcher) {
        m_watcher->addPath(profilePath);
    }
    
    emit profileCreated(name);
    return profile;
}

bool AgentProfileManager::deleteProfile(const QString& name) {
    if (name == "default" || !m_profiles.contains(name)) {
        return false;
    }
    
    AgentProfile* profile = m_profiles[name];
    QString profilePath = profile->profilePath();
    
    QDir dir(profilePath);
    if (dir.removeRecursively()) {
        m_profiles.remove(name);
        delete profile;
        
        if (m_watcher) {
            m_watcher->removePath(profilePath);
        }
        
        emit profileDeleted(name);
        return true;
    }
    
    return false;
}

void AgentProfileManager::onDirectoryChanged(const QString& path) {
    if (path == m_agentsDirectory) {
        QDir dir(m_agentsDirectory);
        QStringList currentSubdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        
        for (const QString& subdir : currentSubdirs) {
            if (!m_profiles.contains(subdir) && subdir != "default") {
                loadProfileFromDirectory(m_agentsDirectory + "/" + subdir);
            }
        }
    } else {
        QString profileName = QFileInfo(path).fileName();
        if (m_profiles.contains(profileName)) {
            reloadProfile(profileName);
        }
    }
}

void AgentProfileManager::onFileChanged(const QString& path) {
    QFileInfo info(path);
    QString profileName = info.dir().dirName();
    
    if (m_profiles.contains(profileName)) {
        reloadProfile(profileName);
    }
}

}
