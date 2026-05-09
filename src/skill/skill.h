#ifndef CLAWPP_SKILL_SKILL_H
#define CLAWPP_SKILL_SKILL_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>

namespace clawpp {

struct SkillMeta {
    QString description;
    QStringList requiresBins;
    QStringList requiresFiles;
    bool always = false;
    bool available = true;
};

struct Skill {
    QString name;
    QString description;
    QStringList triggers;
    QString systemPrompt;
    QString content;
    QJsonArray tools;
    QJsonArray examples;
    QString filePath;
    QString source;
    QDateTime lastModified;
    SkillMeta meta;
    
    bool isValid() const;
    bool isAvailable() const;
    QJsonObject toJson() const;
    bool matchesTrigger(const QString& text) const;
    
    static Skill fromJson(const QJsonObject& obj);
    static Skill fromJsonFile(const QString& filePath);
    static Skill fromSkillMd(const QString& filePath);
    
    static QJsonObject parseFrontmatter(const QString& content);
    static QString extractContent(const QString& fullContent);
};

class SkillLoader {
public:
    static QList<Skill> loadFromDirectory(const QString& directory, const QString& source = "workspace");
    static QString buildSkillsSummary(const QList<Skill>& skills);
    static QStringList getAlwaysSkills(const QList<Skill>& skills);
    static QString loadSkillContent(const Skill& skill);
    static bool checkRequirements(const SkillMeta& meta);
};

}

#endif
