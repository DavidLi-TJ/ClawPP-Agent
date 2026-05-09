#include "skill_manager.h"
#include <QFileInfo>
#include <QFile>
#include <QDirIterator>

namespace clawpp {

SkillManager::SkillManager(QObject* parent)
    : QObject(parent)
    , m_watcher(new QFileSystemWatcher(this)) {
    
    connect(m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &SkillManager::onDirectoryChanged);
    connect(m_watcher, &QFileSystemWatcher::fileChanged,
            this, &SkillManager::onFileChanged);
}

void SkillManager::loadFromDirectory(const QString& directory) {
    QDir dir(directory);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    if (!m_skillsDirectory.isEmpty()) {
        m_watcher->removePaths(m_watcher->directories());
        m_watcher->removePaths(m_watcher->files());
    }
    
    m_skillsDirectory = directory;
    m_skills.clear();
    m_fileToSkill.clear();
    m_loadErrors.clear();
    
    loadSkillsFromDir(directory);
    refreshWatchers();

    emit skillsChanged();
}

QList<Skill> SkillManager::allSkills() const {
    return m_skills.values();
}

QStringList SkillManager::skillNames() const {
    return m_skills.keys();
}

Skill SkillManager::getSkill(const QString& name) const {
    return m_skills.value(name);
}

bool SkillManager::hasSkill(const QString& name) const {
    return m_skills.contains(name);
}

Skill SkillManager::matchTrigger(const QString& text) const {
    for (const Skill& skill : m_skills) {
        if (skill.matchesTrigger(text)) {
            return skill;
        }
    }
    return Skill();
}

bool SkillManager::loadSkill(const QString& filePath) {
    return loadSkill(filePath, false);
}

bool SkillManager::loadSkill(const QString& filePath, bool reload) {
    QFileInfo info(filePath);
    if (!info.exists()) {
        m_loadErrors.append(QStringLiteral("%1: 文件不存在").arg(filePath));
        emit skillError(QString(), QStringLiteral("Skill file missing: %1").arg(filePath));
        return false;
    }
    
    Skill skill;
    if (filePath.endsWith(".md", Qt::CaseInsensitive)) {
        skill = Skill::fromSkillMd(filePath);
    } else {
        skill = Skill::fromJsonFile(filePath);
    }

    if (!skill.isValid()) {
        const QString error = QStringLiteral("Skill 无效或格式错误：%1").arg(filePath);
        m_loadErrors.append(error);
        emit skillError(info.baseName(), error);
        return false;
    }
    
    m_skills[skill.name] = skill;
    m_fileToSkill[filePath] = skill.name;
    
    if (!m_watcher->files().contains(filePath)) {
        m_watcher->addPath(filePath);
    }
    
    if (reload) {
        emit skillReloaded(skill.name);
    } else {
        emit skillLoaded(skill.name);
    }

    emit skillsChanged();
    
    return true;
}

void SkillManager::unloadSkill(const QString& name) {
    if (!m_skills.contains(name)) {
        return;
    }
    
    QString filePath = m_skills[name].filePath;
    m_skills.remove(name);
    m_fileToSkill.remove(filePath);
    
    if (m_watcher->files().contains(filePath)) {
        m_watcher->removePath(filePath);
    }
    
    emit skillUnloaded(name);
    emit skillsChanged();
}

void SkillManager::reloadSkill(const QString& name) {
    if (!m_skills.contains(name)) {
        return;
    }
    
    QString filePath = m_skills[name].filePath;
    loadSkill(filePath, true);
}

void SkillManager::reloadAll() {
    for (const QString& name : m_skills.keys()) {
        reloadSkill(name);
    }

    emit skillsChanged();
}

void SkillManager::loadSkillsFromDir(const QString& directory) {
    QDirIterator iterator(directory,
                          QStringList() << "SKILL.md" << "skill.md" << "*.json",
                          QDir::Files,
                          QDirIterator::Subdirectories);

    while (iterator.hasNext()) {
        loadSkill(iterator.next());
    }
}

void SkillManager::refreshWatchers() {
    m_watcher->removePaths(m_watcher->directories());
    m_watcher->removePaths(m_watcher->files());

    if (m_skillsDirectory.isEmpty()) {
        return;
    }

    if (!m_watcher->directories().contains(m_skillsDirectory)) {
        m_watcher->addPath(m_skillsDirectory);
    }

    QDirIterator dirIterator(m_skillsDirectory,
                             QDir::Dirs | QDir::NoDotAndDotDot,
                             QDirIterator::Subdirectories);
    while (dirIterator.hasNext()) {
        const QString dirPath = dirIterator.next();
        if (!m_watcher->directories().contains(dirPath)) {
            m_watcher->addPath(dirPath);
        }
    }

    QDirIterator fileIterator(m_skillsDirectory,
                              QStringList() << "SKILL.md" << "skill.md" << "*.json",
                              QDir::Files,
                              QDirIterator::Subdirectories);
    while (fileIterator.hasNext()) {
        const QString filePath = fileIterator.next();
        if (!m_watcher->files().contains(filePath)) {
            m_watcher->addPath(filePath);
        }
    }
}

void SkillManager::onDirectoryChanged(const QString& path) {
    Q_UNUSED(path);
    loadFromDirectory(m_skillsDirectory);
}

void SkillManager::onFileChanged(const QString& path) {
    if (m_fileToSkill.contains(path)) {
        QString name = m_fileToSkill[path];
        if (QFile::exists(path)) {
            loadSkill(path, true);
        } else {
            unloadSkill(name);
        }

        emit skillsChanged();
    }
}

QStringList SkillManager::loadErrors() const {
    return m_loadErrors;
}

void SkillManager::clearLoadErrors() {
    m_loadErrors.clear();
}

}
