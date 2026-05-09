#ifndef CLAWPP_SKILL_SKILL_MANAGER_H
#define CLAWPP_SKILL_SKILL_MANAGER_H

#include <QObject>
#include <QDir>
#include <QFileSystemWatcher>
#include <QMap>
#include "skill.h"

namespace clawpp {

class SkillManager : public QObject {
    Q_OBJECT

public:
    explicit SkillManager(QObject* parent = nullptr);
    
    void loadFromDirectory(const QString& directory);
    
    QList<Skill> allSkills() const;
    QStringList skillNames() const;
    Skill getSkill(const QString& name) const;
    bool hasSkill(const QString& name) const;
    Skill matchTrigger(const QString& text) const;
    QStringList loadErrors() const;
    void clearLoadErrors();
    
    bool loadSkill(const QString& filePath);
    void unloadSkill(const QString& name);
    void reloadSkill(const QString& name);
    void reloadAll();

signals:
    void skillLoaded(const QString& name);
    void skillUnloaded(const QString& name);
    void skillReloaded(const QString& name);
    void skillError(const QString& name, const QString& error);
    void skillsChanged();

private:
    void loadSkillsFromDir(const QString& directory);
    void refreshWatchers();
    void onDirectoryChanged(const QString& path);
    void onFileChanged(const QString& path);
    bool loadSkill(const QString& filePath, bool reload);
    
    QMap<QString, Skill> m_skills;
    QMap<QString, QString> m_fileToSkill;
    QStringList m_loadErrors;
    QString m_skillsDirectory;
    QFileSystemWatcher* m_watcher;
};

}

#endif
