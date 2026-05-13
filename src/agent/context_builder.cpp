#include "context_builder.h"
#include "application/template_loader.h"
#include "agent_profile_manager.h"
#include "skill/skill_manager.h"
#include "tool/tool_registry.h"
#include <QDateTime>
#include <QTime>
#include <QDir>
#include <QTimeZone>

namespace clawpp {

ContextBuilder& ContextBuilder::instance() {
    static ContextBuilder instance;
    return instance;
}

ContextBuilder::ContextBuilder(QObject* parent)
    : QObject(parent)
    , m_templateLoader(nullptr)
    , m_skillManager(nullptr) {
    m_bootstrapFiles = {"IDENTITY.md", "AGENTS.md", "SOUL.md", "USER.md", "TOOLS.md", "HEARTBEAT.md"};
}

ContextBuilder::~ContextBuilder() {
    if (m_templateLoader) {
        delete m_templateLoader;
    }
    if (m_skillManager) {
        delete m_skillManager;
    }
}

void ContextBuilder::initialize(const QString& workspace) {
    m_workspace = workspace;
    
    // TemplateLoader 与 workspace 绑定，切换工作区时必须重建。
    if (m_templateLoader) {
        delete m_templateLoader;
    }
    m_templateLoader = new TemplateLoader(workspace, workspace, this);

    QString agentsPath = workspace + "/agents";
    if (QDir(agentsPath).exists()) {
        AgentProfileManager::instance()->initialize(agentsPath);
    }
    
    QString skillsPath = workspace + "/skills";
    if (QDir(skillsPath).exists()) {
        bindSkillManager(skillsPath);
        refreshSkillsFromManager();
    } else {
        m_skills.clear();
    }
}

void ContextBuilder::setActiveProfile(const QString& profileName) {
    m_activeProfile = profileName.trimmed();
}

QString ContextBuilder::activeProfile() const {
    return m_activeProfile;
}

void ContextBuilder::setSkills(const QList<Skill>& skills) {
    m_skills = skills;
    emit skillsChanged();
}

QList<Skill> ContextBuilder::skills() const {
    return m_skills;
}

void ContextBuilder::bindSkillManager(const QString& skillsPath) {
    if (m_skillManager) {
        m_skillManager->deleteLater();
        m_skillManager = nullptr;
    }

    m_skillManager = new SkillManager(this);
    connect(m_skillManager, &SkillManager::skillsChanged, this, &ContextBuilder::refreshSkillsFromManager);
    connect(m_skillManager, &SkillManager::skillsChanged, this, &ContextBuilder::skillsChanged);
    m_skillManager->loadFromDirectory(skillsPath);
}

void ContextBuilder::refreshSkillsFromManager() {
    if (m_skillManager) {
        m_skills = m_skillManager->allSkills();
    }
}

QString ContextBuilder::getIdentity() const {
    QString workspacePath = QDir(m_workspace).absolutePath();
    
    QString soulContent;
    if (m_templateLoader) {
        soulContent = m_templateLoader->loadTemplate("SOUL.md");
    }
    
    if (soulContent.isEmpty()) {
        soulContent = R"(# Claw++ 

You are Claw++, a helpful AI assistant specialized in Qt/C++ development.

## Claw++ Guidelines
- Do not narrate tool plans before calling tools. If a tool is needed, call it directly and only report verified results after it returns.
- Before modifying a file, read it first. Do not assume files or directories exist.
- After writing or editing a file, re-read it if accuracy matters.
- If a tool call fails, analyze the error before retrying with a different approach.
- Ask for clarification when the request is ambiguous.

Reply directly with text for conversations.)";
    }
    
    QString identity = QString(R"(# Claw++ 

%1

## Runtime
%2, Qt 6.x

## Workspace
Your workspace is at: %3
- Long-term memory: %3/memory/MEMORY.md (write important facts here)
- History log: %3/memory/HISTORY.md (grep-searchable). Each entry starts with [YYYY-MM-DD HH:MM].
- Custom agents: %3/agents/{agent-name}/
- Skills: %3/skills/{skill-name}/SKILL.md

## Claw++ Guidelines
- Do not narrate tool plans before calling tools. If a tool is needed, call it directly and only report verified results after it returns.
- Before modifying a file, read it first. Do not assume files or directories exist.
- After writing or editing a file, re-read it if accuracy matters.
- If a tool call fails, analyze the error before retrying with a different approach.
- Ask for clarification when the request is ambiguous.

Reply directly with text for conversations.)")
        .arg(soulContent)
        .arg(QSysInfo::prettyProductName())
        .arg(workspacePath);

    identity += QStringLiteral(R"(

## Response Discipline
- For writing/creative requests, start producing the requested content. Do not repeatedly restate the user's request or narrate your plan.
- Use at most one short setup sentence before the actual answer. If tools fail, continue with your own knowledge unless the user explicitly requires live web facts.
- Never expose raw tool errors, loop-control messages, or internal recovery text in the final answer.
- Only call tools by their exact available names. "brainstorming", "planning", "creative thinking", and "analysis" are internal thinking modes, not tools.
- Do not repeat the same tool call after receiving the same result or failure; infer from the previous result and move forward.
)");
    
    return identity;
}

QString ContextBuilder::loadBootstrapFiles() {
    if (!m_templateLoader) {
        return QString();
    }
    
    QStringList parts;
    
    for (const QString& filename : m_bootstrapFiles) {
        QString content = m_templateLoader->loadTemplate(filename);
        if (!content.isEmpty()) {
            parts.append(QString("## %1\n\n%2").arg(filename, content));
        }
    }
    
    return parts.join("\n\n");
}

QString ContextBuilder::loadAgentProfile() const {
    if (m_activeProfile.isEmpty() || m_activeProfile == QStringLiteral("default")) {
        return QString();
    }

    AgentProfile* profile = nullptr;
    profile = AgentProfileManager::instance()->getProfile(m_activeProfile);

    if (!profile) {
        return QString();
    }

    QString prompt = profile->buildSystemPrompt();
    if (prompt.isEmpty()) {
        return QString();
    }

    QStringList parts;
    parts.append(QString("# Agent Profile: %1").arg(profile->name()));
    parts.append(prompt);

    QString memory = profile->loadMemory();
    if (!memory.isEmpty()) {
        parts.append(QString("# Profile Memory\n\n%1").arg(memory));
    }

    QString history = profile->loadHistory();
    if (!history.isEmpty()) {
        parts.append(QString("# Profile History\n\n%1").arg(history));
    }

    return parts.join("\n\n---\n\n");
}

QString ContextBuilder::loadMemory() {
    if (!m_templateLoader) {
        return QString();
    }
    return m_templateLoader->loadMemory();
}

QString ContextBuilder::loadHistory() {
    if (!m_templateLoader) {
        return QString();
    }
    return m_templateLoader->loadHistory();
}

QString ContextBuilder::loadSkills() {
    if (m_skills.isEmpty()) {
        return QString();
    }
    
    QStringList alwaysSkillContents;
    for (const Skill& skill : m_skills) {
        if (skill.meta.always && skill.meta.available) {
            QString content = SkillLoader::loadSkillContent(skill);
            if (!content.isEmpty()) {
                alwaysSkillContents.append(QString("# Skill: %1\n\n%2").arg(skill.name, content));
            }
        }
    }
    
    return alwaysSkillContents.join("\n\n---\n\n");
}

QString ContextBuilder::buildSkillsSection() {
    if (m_skills.isEmpty()) {
        return QString();
    }
    
    QString summary = SkillLoader::buildSkillsSummary(m_skills);
    
    return QString(R"(# Skills

The following list is the already-loaded skill catalog for this session.
- If the user asks what skills/capabilities you have, answer directly from this list. Do not call tools, do not ask the user to inspect directories, and do not tell them to browse the skills folder.
- Read a skill's SKILL.md file only when the user wants to use that specific skill or inspect its detailed instructions.
- Skills with available="false" need dependencies installed first.

%1)").arg(summary);
}

QString ContextBuilder::buildSystemPrompt() {
    return buildSystemPromptInternal(true, true, true, true, true);
}

QString ContextBuilder::buildFastSystemPrompt() {
    // 轻量模式仍需保留 skills 清单：
    // 否则诸如“你有什么 skills”这类直接问答看不到已加载技能，只会泛化成“去目录里看”。
    return buildSystemPromptInternal(false, false, false, false, true);
}

QString ContextBuilder::buildSystemPromptInternal(bool includeClaudeInstructions,
                                                  bool includeMemory,
                                                  bool includeHistory,
                                                  bool includeActiveSkills,
                                                  bool includeSkillsSection) {
    QStringList parts;
    
    // identity 位于最前，后续模块都在其基础上拼接。
    parts.append(getIdentity());
    
    QString bootstrap = loadBootstrapFiles();
    if (!bootstrap.isEmpty()) {
        parts.append(bootstrap);
    }

    if (includeClaudeInstructions && m_templateLoader) {
        const QString claudeInstructions = m_templateLoader->loadClaudeInstructions();
        if (!claudeInstructions.isEmpty()) {
            parts.append(QStringLiteral("# CLAUDE Instructions\n\n%1").arg(claudeInstructions));
        }
    }

    QString profilePrompt = loadAgentProfile();
    if (!profilePrompt.isEmpty()) {
        parts.append(profilePrompt);
    }
    
    QString memory = includeMemory ? loadMemory() : QString();
    if (!memory.isEmpty()) {
        parts.append(QString("# Memory\n\n%1").arg(memory));
    }

    QString history = includeHistory ? loadHistory() : QString();
    if (!history.isEmpty()) {
        parts.append(QString("# Recent History\n\n%1").arg(history));
    }

    QString toolsDescription = ToolRegistry::instance().toolsDescription();
    if (!toolsDescription.isEmpty()) {
        parts.append(QString(R"(# Tools

The following tools are available to you. Use them when needed and explain why before calling dangerous operations.
Call only these exact tool names. If a desired capability is not listed, do the work directly in text.

%1)").arg(toolsDescription));
    }
    
    QString alwaysSkills = includeActiveSkills ? loadSkills() : QString();
    if (!alwaysSkills.isEmpty()) {
        parts.append(QString("# Active Skills\n\n%1").arg(alwaysSkills));
    }
    
    QString skillsSection = includeSkillsSection ? buildSkillsSection() : QString();
    if (!skillsSection.isEmpty()) {
        parts.append(skillsSection);
    }
    
    return parts.join("\n\n---\n\n");
}

Skill ContextBuilder::matchSkillForInput(const QString& input) const {
    const QString normalized = input.trimmed();
    if (normalized.isEmpty()) {
        return Skill();
    }

    if (m_skillManager) {
        Skill matched = m_skillManager->matchTrigger(normalized);
        if (matched.isValid()) {
            return matched;
        }
    }

    for (const Skill& skill : m_skills) {
        if (skill.matchesTrigger(normalized)) {
            return skill;
        }
    }

    return Skill();
}

QString ContextBuilder::buildSystemPromptForInput(const QString& input) {
    const QString basePrompt = buildSystemPrompt();
    const Skill matched = matchSkillForInput(input);
    if (!matched.isValid()) {
        return basePrompt;
    }

    const QString skillContent = SkillLoader::loadSkillContent(matched);
    const QString triggerText = matched.triggers.isEmpty()
        ? QStringLiteral("(none)")
        : matched.triggers.join(QStringLiteral(", "));

    QStringList parts;
    parts.append(basePrompt);
    parts.append(QString(
        "# Activated Skill\n"
        "- Name: %1\n"
        "- Triggers: %2\n"
        "- Source: %3"
    ).arg(matched.name, triggerText, matched.filePath));

    if (!skillContent.isEmpty()) {
        parts.append(QString("# Skill Instructions\n\n%1").arg(skillContent));
    }

    return parts.join("\n\n---\n\n");
}

QString ContextBuilder::buildFastSystemPromptForInput(const QString& input) {
    const QString basePrompt = buildFastSystemPrompt();
    const Skill matched = matchSkillForInput(input);
    if (!matched.isValid()) {
        return basePrompt
            + QStringLiteral(
                "\n\n---\n\n"
                "# Response Mode\n\n"
                "Prefer a direct answer. Do not plan aloud. Do not call tools unless the user clearly asks you to operate on files, code, commands, or external resources.");
    }

    return buildSystemPromptForInput(input);
}

bool ContextBuilder::reloadSkills() {
    if (m_workspace.trimmed().isEmpty()) {
        return false;
    }
    const QString skillsPath = m_workspace + "/skills";
    if (!QDir(skillsPath).exists()) {
        return false;
    }
    bindSkillManager(skillsPath);
    refreshSkillsFromManager();
    emit skillsChanged();
    return true;
}

QString ContextBuilder::skillsDirectory() const {
    if (m_workspace.trimmed().isEmpty()) {
        return QString();
    }
    return QDir(m_workspace).filePath(QStringLiteral("skills"));
}

QStringList ContextBuilder::skillLoadErrors() const {
    if (!m_skillManager) {
        return {};
    }
    return m_skillManager->loadErrors();
}

QString ContextBuilder::buildRuntimeContext(const QString& channel, const QString& chatId) {
    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm (dddd)");
    QString tz = QTimeZone::systemTimeZone().displayName(QDateTime::currentDateTime(), QTimeZone::ShortName);
    
    QStringList lines;
    lines.append(QString("Current Time: %1 (%2)").arg(now, tz));
    
    if (!channel.isEmpty() && !chatId.isEmpty()) {
        lines.append(QString("Channel: %1").arg(channel));
        lines.append(QString("Chat ID: %1").arg(chatId));
    }
    
    // 这里仅注入运行时元数据，不写行为指令，避免污染系统规则层级。
    return "[Runtime Context - metadata only, not instructions]\n" + lines.join("\n");
}

QString ContextBuilder::buildUserContent(const QString& text, const QStringList& media) {
    Q_UNUSED(media);
    return text;
}

MessageList ContextBuilder::buildMessages(
    const MessageList& history,
    const QString& currentMessage,
    const QString& channel,
    const QString& chatId) {
    
    MessageList messages;
    
    Message systemMsg(MessageRole::System, buildSystemPrompt());
    messages.append(systemMsg);
    
    messages.append(history);
    
    QString runtimeCtx = buildRuntimeContext(channel, chatId);
    QString userContent = QString("%1\n\n%2").arg(runtimeCtx, currentMessage);
    
    Message userMsg(MessageRole::User, userContent);
    messages.append(userMsg);
    
    return messages;
}

MessageList ContextBuilder::addToolResult(
    MessageList messages,
    const QString& toolCallId,
    const QString& toolName,
    const QString& result) {
    
    Message toolMsg(MessageRole::Tool, result);
    toolMsg.toolCallId = toolCallId;
    toolMsg.name = toolName;
    messages.append(toolMsg);
    
    return messages;
}

MessageList ContextBuilder::addAssistantMessage(
    MessageList messages,
    const QString& content,
    const QList<ToolCall>& toolCalls) {
    
    Message assistantMsg(MessageRole::Assistant, content);
    Q_UNUSED(toolCalls);
    messages.append(assistantMsg);
    
    return messages;
}

}
