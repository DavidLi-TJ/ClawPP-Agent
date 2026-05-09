#include "skill.h"
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QRegularExpression>
#include <QStandardPaths>

namespace clawpp {

namespace {

QString trimQuoted(QString value) {
    value = value.trimmed();
    if ((value.startsWith('"') && value.endsWith('"')) ||
        (value.startsWith('\'') && value.endsWith('\''))) {
        value = value.mid(1, value.size() - 2);
    }
    return value;
}

QString extractFrontmatter(const QString& content) {
    QRegularExpression regex(QStringLiteral("(?s)^---\\s*\\n(.*?)\\n---\\s*\\n"));
    QRegularExpressionMatch match = regex.match(content);
    return match.hasMatch() ? match.captured(1) : QString();
}

QString extractScalar(const QString& block, const QString& key) {
    const QString pattern = QStringLiteral("(?m)^[ \\t]*%1:\\s*(.*?)\\s*$")
                                .arg(QRegularExpression::escape(key));
    QRegularExpression regex(pattern);
    QRegularExpressionMatch match = regex.match(block);
    return match.hasMatch() ? trimQuoted(match.captured(1)) : QString();
}

QString extractIndentedBlock(const QString& block, const QString& key, int indent) {
    const QString indentPrefix(indent, QLatin1Char(' '));
    const QString childPrefix(indent + 2, QLatin1Char(' '));
    const QString pattern = QStringLiteral("(?ms)^%1%2:\\s*\\n((?:%3.*\\n?)*)")
                                .arg(QRegularExpression::escape(indentPrefix),
                                     QRegularExpression::escape(key),
                                     QRegularExpression::escape(childPrefix));
    QRegularExpression regex(pattern);
    QRegularExpressionMatch match = regex.match(block);
    return match.hasMatch() ? match.captured(1) : QString();
}

QJsonArray extractList(const QString& block, const QString& key) {
    QJsonArray values;
    const QString section = extractIndentedBlock(block, key, 0);
    if (section.isEmpty()) {
        return values;
    }

    for (const QString& rawLine : section.split('\n')) {
        const QString line = rawLine.trimmed();
        if (!line.startsWith("- ")) {
            continue;
        }

        const QString value = trimQuoted(line.mid(2));
        if (!value.isEmpty()) {
            values.append(value);
        }
    }

    return values;
}

QJsonObject buildRequiresObject(const QString& block) {
    QJsonObject requiresObject;

    auto parseItems = [](const QString& sectionName, const QString& text) {
        QJsonArray items;
        const QString pattern = QStringLiteral("(?ms)^[ \\t]*%1:\\s*\\n((?:[ \\t]+.*\\n?)*)")
                                    .arg(QRegularExpression::escape(sectionName));
        QRegularExpression regex(pattern);
        QRegularExpressionMatch match = regex.match(text);
        if (!match.hasMatch()) {
            return items;
        }

        for (const QString& rawLine : match.captured(1).split('\n')) {
            const QString line = rawLine.trimmed();
            if (!line.startsWith("- ")) {
                continue;
            }
            const QString value = trimQuoted(line.mid(2));
            if (!value.isEmpty()) {
                items.append(value);
            }
        }
        return items;
    };

    requiresObject[QStringLiteral("bins")] = parseItems(QStringLiteral("bins"), block);
    requiresObject[QStringLiteral("files")] = parseItems(QStringLiteral("files"), block);
    return requiresObject;
}

QJsonObject buildMetadataObject(const QString& frontmatter) {
    QJsonObject metadata;

    auto buildProviderObject = [](const QString& providerBlock, int indent) {
        QJsonObject providerObject;
        if (providerBlock.isEmpty()) {
            return providerObject;
        }

        const QString description = extractScalar(providerBlock, QStringLiteral("description"));
        if (!description.isEmpty()) {
            providerObject[QStringLiteral("description")] = description;
        }

        const QString emoji = extractScalar(providerBlock, QStringLiteral("emoji"));
        if (!emoji.isEmpty()) {
            providerObject[QStringLiteral("emoji")] = emoji;
        }

        const QString always = extractScalar(providerBlock, QStringLiteral("always"));
        if (!always.isEmpty()) {
            providerObject[QStringLiteral("always")] = (always.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0);
        }

        const QString requiresBlock = extractIndentedBlock(providerBlock, QStringLiteral("requires"), indent);
        if (!requiresBlock.isEmpty()) {
            providerObject[QStringLiteral("requires")] = buildRequiresObject(requiresBlock);
        }

        return providerObject;
    };

    const QString openclawBlock = extractIndentedBlock(frontmatter, QStringLiteral("openclaw"), 2);
    const QString nanobotBlock = extractIndentedBlock(frontmatter, QStringLiteral("nanobot"), 2);

    const QJsonObject openclawObject = buildProviderObject(openclawBlock, 4);
    if (!openclawObject.isEmpty()) {
        metadata[QStringLiteral("openclaw")] = openclawObject;
    }

    const QJsonObject nanobotObject = buildProviderObject(nanobotBlock, 4);
    if (!nanobotObject.isEmpty()) {
        metadata[QStringLiteral("nanobot")] = nanobotObject;
    }

    return metadata;
}

} // namespace

QJsonObject Skill::toJson() const {
    QJsonObject obj;
    obj["name"] = name;
    obj["description"] = description;
    obj["triggers"] = QJsonArray::fromStringList(triggers);
    obj["system_prompt"] = systemPrompt;
    obj["tools"] = tools;
    obj["examples"] = examples;
    obj["source"] = source;
    obj["available"] = meta.available;
    return obj;
}

Skill Skill::fromJson(const QJsonObject& obj) {
    Skill skill;
    skill.name = obj["name"].toString();
    skill.description = obj["description"].toString();
    skill.systemPrompt = obj["system_prompt"].toString();
    skill.tools = obj["tools"].toArray();
    skill.examples = obj["examples"].toArray();
    
    QJsonArray triggersArray = obj["triggers"].toArray();
    for (const QJsonValue& v : triggersArray) {
        skill.triggers.append(v.toString());
    }
    
    return skill;
}

Skill Skill::fromJsonFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return Skill();
    }
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();
    
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return Skill();
    }
    
    Skill skill = fromJson(doc.object());
    skill.filePath = filePath;
    skill.lastModified = QFileInfo(filePath).lastModified();
    if (skill.triggers.isEmpty() && !skill.name.isEmpty()) {
        skill.triggers.append(QStringLiteral("/%1").arg(skill.name));
    }
    
    return skill;
}

Skill Skill::fromSkillMd(const QString& filePath) {
    QFileInfo info(filePath);
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return Skill();
    }
    
    QString fullContent = QString::fromUtf8(file.readAll());
    file.close();
    
    QJsonObject frontmatter = parseFrontmatter(fullContent);
    QString bodyContent = extractContent(fullContent).trimmed();

    Skill skill;
    skill.filePath = filePath;
    skill.source = QStringLiteral("workspace");
    skill.content = bodyContent;
    skill.systemPrompt = bodyContent;
    skill.lastModified = info.lastModified();

    skill.name = frontmatter[QStringLiteral("name")].toString(info.dir().dirName());
    skill.description = frontmatter[QStringLiteral("description")].toString();

    QJsonArray triggersArray = frontmatter[QStringLiteral("triggers")].toArray();
    for (const QJsonValue& value : triggersArray) {
        QString trigger = value.toString().trimmed();
        if (!trigger.isEmpty()) {
            skill.triggers.append(trigger);
        }
    }

    QJsonArray toolsArray = frontmatter[QStringLiteral("tools")].toArray();
    skill.tools = toolsArray;

    QJsonArray examplesArray = frontmatter[QStringLiteral("examples")].toArray();
    skill.examples = examplesArray;
    
    if (!frontmatter.isEmpty()) {
        QJsonObject metadata = frontmatter[QStringLiteral("metadata")].toObject();
        QJsonObject providerMeta = metadata[QStringLiteral("openclaw")].toObject();
        if (providerMeta.isEmpty()) {
            providerMeta = metadata[QStringLiteral("nanobot")].toObject();
        }

        skill.meta.description = providerMeta[QStringLiteral("description")].toString(skill.description);
        if (skill.description.isEmpty()) {
            skill.description = skill.meta.description;
        }

        skill.meta.always = providerMeta[QStringLiteral("always")].toBool(false);

        QJsonObject requiresObj = providerMeta[QStringLiteral("requires")].toObject();
        QJsonArray binsArray = requiresObj[QStringLiteral("bins")].toArray();
        for (const QJsonValue& value : binsArray) {
            const QString bin = value.toString().trimmed();
            if (!bin.isEmpty()) {
                skill.meta.requiresBins.append(bin);
            }
        }

        QJsonArray filesArray = requiresObj[QStringLiteral("files")].toArray();
        for (const QJsonValue& value : filesArray) {
            QString path = value.toString().trimmed();
            if (path.isEmpty()) {
                continue;
            }
            if (QFileInfo(path).isRelative()) {
                path = QFileInfo(info.absolutePath(), path).absoluteFilePath();
            }
            skill.meta.requiresFiles.append(path);
        }
    }
    
    if (skill.triggers.isEmpty() && !skill.name.isEmpty()) {
        skill.triggers.append(QStringLiteral("/%1").arg(skill.name));
    }

    skill.meta.available = SkillLoader::checkRequirements(skill.meta);
    
    return skill;
}

QJsonObject Skill::parseFrontmatter(const QString& content) {
    const QString frontmatter = extractFrontmatter(content);
    if (frontmatter.isEmpty()) {
        return QJsonObject();
    }

    QJsonObject result;

    const QString name = extractScalar(frontmatter, QStringLiteral("name"));
    if (!name.isEmpty()) {
        result[QStringLiteral("name")] = name;
    }

    const QString description = extractScalar(frontmatter, QStringLiteral("description"));
    if (!description.isEmpty()) {
        result[QStringLiteral("description")] = description;
    }

    const QString homepage = extractScalar(frontmatter, QStringLiteral("homepage"));
    if (!homepage.isEmpty()) {
        result[QStringLiteral("homepage")] = homepage;
    }

    const QJsonArray triggers = extractList(frontmatter, QStringLiteral("triggers"));
    if (!triggers.isEmpty()) {
        result[QStringLiteral("triggers")] = triggers;
    }

    const QJsonArray tools = extractList(frontmatter, QStringLiteral("tools"));
    if (!tools.isEmpty()) {
        result[QStringLiteral("tools")] = tools;
    }

    const QJsonArray examples = extractList(frontmatter, QStringLiteral("examples"));
    if (!examples.isEmpty()) {
        result[QStringLiteral("examples")] = examples;
    }

    const QJsonObject metadata = buildMetadataObject(frontmatter);
    if (!metadata.isEmpty()) {
        result[QStringLiteral("metadata")] = metadata;
    }

    return result;
}

QString Skill::extractContent(const QString& fullContent) {
    QRegularExpression re(QStringLiteral("(?s)^---\\s*\n.*?\n---\\s*\n"));
    QString result = fullContent;
    result.remove(re);
    return result;
}

bool Skill::isValid() const {
    return !name.isEmpty() && (!content.isEmpty() || !systemPrompt.isEmpty() || !description.isEmpty());
}

bool Skill::isAvailable() const {
    return meta.available;
}

bool Skill::matchesTrigger(const QString& text) const {
    const QString lowerText = text.toLower();
    for (const QString& trigger : triggers) {
        const QString normalizedTrigger = trigger.trimmed().toLower();
        if (!normalizedTrigger.isEmpty() && lowerText.contains(normalizedTrigger)) {
            return true;
        }
    }

    if (!name.isEmpty()) {
        const QString fallbackTrigger = QStringLiteral("/%1").arg(name.toLower());
        if (lowerText.contains(fallbackTrigger)) {
            return true;
        }
    }

    return false;
}

QList<Skill> SkillLoader::loadFromDirectory(const QString& directory, const QString& source) {
    QList<Skill> skills;
    
    QDir dir(directory);
    if (!dir.exists()) {
        return skills;
    }
    
    QDirIterator it(directory,
                    QStringList() << QStringLiteral("SKILL.md")
                                  << QStringLiteral("skill.md")
                                  << QStringLiteral("*.json"),
                    QDir::Files,
                    QDirIterator::Subdirectories);

    while (it.hasNext()) {
        const QString filePath = it.next();
        Skill skill;
        if (filePath.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive)) {
            skill = Skill::fromJsonFile(filePath);
        } else {
            skill = Skill::fromSkillMd(filePath);
        }

        if (!skill.isValid()) {
            continue;
        }

        skill.source = source;
        if (skill.triggers.isEmpty() && !skill.name.isEmpty()) {
            skill.triggers.append(QStringLiteral("/%1").arg(skill.name));
        }
        skills.append(skill);
    }
    
    return skills;
}

QString SkillLoader::buildSkillsSummary(const QList<Skill>& skills) {
    QStringList lines;
    lines.append("<skills>");
    
    for (const Skill& skill : skills) {
        QString available = skill.meta.available ? "true" : "false";
        lines.append(QString("  <skill available=\"%1\">").arg(available));
        lines.append(QString("    <name>%1</name>").arg(skill.name));
        lines.append(QString("    <description>%1</description>").arg(skill.description.isEmpty() ? skill.name : skill.description));
        if (!skill.triggers.isEmpty()) {
            lines.append(QString("    <triggers>%1</triggers>").arg(skill.triggers.join(", ")));
        }
        lines.append(QString("    <location>%1</location>").arg(skill.filePath));
        lines.append("  </skill>");
    }
    
    lines.append("</skills>");
    return lines.join("\n");
}

QStringList SkillLoader::getAlwaysSkills(const QList<Skill>& skills) {
    QStringList result;
    for (const Skill& skill : skills) {
        if (skill.meta.always && skill.meta.available) {
            result.append(skill.name);
        }
    }
    return result;
}

QString SkillLoader::loadSkillContent(const Skill& skill) {
    if (!skill.content.isEmpty()) {
        return skill.content;
    }
    
    if (!skill.filePath.isEmpty() && QFile::exists(skill.filePath)) {
        QFile file(skill.filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = QString::fromUtf8(file.readAll());
            file.close();
            return Skill::extractContent(content);
        }
    }
    
    return skill.systemPrompt;
}

bool SkillLoader::checkRequirements(const SkillMeta& meta) {
    for (const QString& bin : meta.requiresBins) {
        QString path = QStandardPaths::findExecutable(bin);
        if (path.isEmpty()) {
            return false;
        }
    }
    
    for (const QString& file : meta.requiresFiles) {
        if (!QFile::exists(file)) {
            return false;
        }
    }
    
    return true;
}

}
