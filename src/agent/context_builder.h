#ifndef CLAWPP_AGENT_CONTEXT_BUILDER_H
#define CLAWPP_AGENT_CONTEXT_BUILDER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QSysInfo>
#include <QTimeZone>
#include "common/types.h"
#include "skill/skill.h"

namespace clawpp {

class TemplateLoader;
class SkillManager;

/**
 * @class ContextBuilder
 * @brief 组装系统提示词与运行时上下文的中心组件。
 *
 * 该类将模板、技能、记忆、工具描述等多源信息拼接为最终 prompt，
 * 并负责按用户输入触发技能匹配。
 */
class ContextBuilder : public QObject {
    Q_OBJECT

public:
    static ContextBuilder& instance();
    
    /// 初始化工作区相关资源（模板、技能目录、Profile）。
    void initialize(const QString& workspace);
    /// 设置当前激活的 Agent Profile 名称。
    void setActiveProfile(const QString& profileName);
    QString activeProfile() const;
    
    /// 构建完整系统提示词（不绑定具体用户输入）。
    QString buildSystemPrompt();
    /// 构建和输入绑定的系统提示词（可激活匹配技能）。
    QString buildSystemPromptForInput(const QString& input);
    bool reloadSkills();
    QString skillsDirectory() const;
    QStringList skillLoadErrors() const;
    /// 构建运行时元数据上下文（时间、渠道、chatId）。
    QString buildRuntimeContext(const QString& channel = QString(), const QString& chatId = QString());
    QString buildUserContent(const QString& text, const QStringList& media = QStringList());
    Skill matchSkillForInput(const QString& input) const;
    
    MessageList buildMessages(
        const MessageList& history,
        const QString& currentMessage,
        const QString& channel = QString(),
        const QString& chatId = QString()
    );
    
    MessageList addToolResult(
        MessageList messages,
        const QString& toolCallId,
        const QString& toolName,
        const QString& result
    );
    
    MessageList addAssistantMessage(
        MessageList messages,
        const QString& content,
        const QList<ToolCall>& toolCalls = QList<ToolCall>()
    );
    
    void setSkills(const QList<Skill>& skills);
    QList<Skill> skills() const;

signals:
    void skillsChanged();

private:
    ContextBuilder(QObject* parent = nullptr);
    ~ContextBuilder();
    
    QString getIdentity() const;
    QString loadBootstrapFiles();
    QString loadAgentProfile() const;
    QString loadMemory();
    QString loadHistory();
    QString loadSkills();
    QString buildSkillsSection();
    void bindSkillManager(const QString& skillsPath);
    void refreshSkillsFromManager();

    QString m_workspace;
    TemplateLoader* m_templateLoader;
    SkillManager* m_skillManager;
    QStringList m_bootstrapFiles;
    QList<Skill> m_skills;
    QString m_activeProfile;
};

}

#endif
