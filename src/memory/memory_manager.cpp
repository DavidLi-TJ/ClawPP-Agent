#include "memory_manager.h"
#include "conversation_history.h"
#include "infrastructure/logging/logger.h"
#include <QSaveFile>
#include <QTextStream>
#include <QDir>
#include <QRegularExpression>
#include <QSet>
#include <QtMath>
#include <algorithm>
#include <utility>

namespace clawpp {

MemoryManager::MemoryManager(const QString& workspace, QObject* parent)
    : IMemorySystem(parent)
    , m_workspace(workspace)
    , m_history(new ConversationHistory(this))
    , m_runtimeTokenThreshold(0) {
    ensureMemoryDirectory();
    m_memoryFilePath = m_workspace + "/memory/MEMORY.md";
    m_historyFilePath = m_workspace + "/memory/HISTORY.md";
    ensureMemorySeedFile();
}

MemoryManager::MemoryManager(const MemoryConfig& config, QObject* parent)
    : IMemorySystem(parent)
    , m_config(config)
    , m_history(new ConversationHistory(this))
    , m_runtimeTokenThreshold(0) {
    m_workspace = QDir::currentPath();
    ensureMemoryDirectory();
    m_memoryFilePath = m_workspace + "/memory/MEMORY.md";
    m_historyFilePath = m_workspace + "/memory/HISTORY.md";
    ensureMemorySeedFile();
}

MemoryManager::~MemoryManager() {
}

void MemoryManager::setWorkspace(const QString& workspace) {
    m_workspace = workspace;
    ensureMemoryDirectory();
    m_memoryFilePath = m_workspace + "/memory/MEMORY.md";
    m_historyFilePath = m_workspace + "/memory/HISTORY.md";
    ensureMemorySeedFile();
}

QString MemoryManager::workspace() const {
    return m_workspace;
}

void MemoryManager::ensureMemoryDirectory() {
    QDir dir(m_workspace + "/memory");
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            LOG_ERROR(QStringLiteral("MemoryManager failed to create memory directory: %1")
                          .arg(dir.absolutePath()));
        }
    }
}

void MemoryManager::ensureMemorySeedFile() {
    QFile file(m_memoryFilePath);
    if (file.exists()) {
        return;
    }

    writeLongTerm(memoryIndexTemplate());
}

QString MemoryManager::readLongTerm() const {
    QFile file(m_memoryFilePath);
    if (!file.exists()) {
        return QString();
    }
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR(QStringLiteral("MemoryManager failed to open MEMORY.md for read: %1")
                      .arg(m_memoryFilePath));
        return QString();
    }
    
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QString content = in.readAll();
    file.close();
    
    return content;
}

QString MemoryManager::readHistoryLog() const {
    QFile file(m_historyFilePath);
    if (!file.exists()) {
        return QString();
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR(QStringLiteral("MemoryManager failed to open HISTORY.md for read: %1")
                      .arg(m_historyFilePath));
        return QString();
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QString content = in.readAll();
    file.close();

    return content;
}

void MemoryManager::writeLongTerm(const QString& content) {
    ensureMemoryDirectory();
    // 原子写入，避免程序异常退出导致 MEMORY.md 部分写入。
    QSaveFile file(m_memoryFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_ERROR(QStringLiteral("MemoryManager failed to open MEMORY.md for write: %1")
                      .arg(m_memoryFilePath));
        return;
    }
    
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << content;

    if (!file.commit()) {
        LOG_ERROR(QStringLiteral("MemoryManager failed to commit MEMORY.md: %1")
                      .arg(m_memoryFilePath));
        return;
    }
    
    emit memoryUpdated();
}

void MemoryManager::appendHistory(const QString& entry) {
    ensureMemoryDirectory();
    QFile file(m_historyFilePath);
    if (!file.open(QIODevice::Append | QIODevice::Text)) {
        LOG_ERROR(QStringLiteral("MemoryManager failed to append HISTORY.md: %1")
                      .arg(m_historyFilePath));
        return;
    }
    
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << entry << "\n";
    file.close();
}

void MemoryManager::clearPersistentStorage() {
    m_history->clear();

    QFile memoryFile(m_memoryFilePath);
    if (memoryFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        memoryFile.resize(0);
        memoryFile.close();
    }

    QFile historyFile(m_historyFilePath);
    if (historyFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        historyFile.resize(0);
        historyFile.close();
    }

    emit memoryUpdated();
}

QString MemoryManager::getMemoryContext() const {
    QStringList parts;
    
    QString memory = readLongTerm();
    if (!memory.isEmpty()) {
        parts.append(memory);
    }

    const QStringList semantic = queryRelevantMemory(
        QStringLiteral("%1 %2").arg(m_currentSessionName, m_currentSessionId),
        m_config.retrievalTopK);
    if (!semantic.isEmpty()) {
        parts.append(QStringLiteral("## Relevant Memory\n\n- %1")
                         .arg(semantic.join(QStringLiteral("\n- "))));
    }

    QFile historyFile(m_historyFilePath);
    if (historyFile.exists() && historyFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&historyFile);
        in.setEncoding(QStringConverter::Utf8);
        QStringList lines = in.readAll().split('\n', Qt::SkipEmptyParts);
        historyFile.close();

        const int maxLines = 40;
        if (!lines.isEmpty()) {
            QStringList recentLines = lines.mid(qMax(0, lines.size() - maxLines));
            parts.append(QString("## Recent History\n\n%1").arg(recentLines.join("\n")));
        }
    }
    
    return parts.join("\n\n");
}

void MemoryManager::appendHistoryEntry(const QString& role, const QString& content, const QStringList& toolsUsed) {
    QString entry = formatHistoryEntry(role, content, toolsUsed);
    appendHistory(entry);
}

void MemoryManager::saveMemoryUpdate(const QString& historyEntry, const QString& memoryUpdate) {
    if (!historyEntry.isEmpty()) {
        appendHistory(historyEntry);
    }
    
    if (!memoryUpdate.isEmpty()) {
        appendLongTermMemory(memoryUpdate);
    }
}

QString MemoryManager::formatHistoryEntry(const QString& role, const QString& content, const QStringList& toolsUsed) {
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");
    QString sessionLabel = m_currentSessionName.isEmpty() ? m_currentSessionId : m_currentSessionName;
    
    QString tools;
    if (!toolsUsed.isEmpty()) {
        tools = QString(" [tools: %1]").arg(toolsUsed.join(", "));
    }
    
    QString preview = content;
    if (preview.length() > 500) {
        preview = preview.left(500) + "...";
    }
    preview = preview.replace('\n', ' ');
    
    QString sessionPart;
    if (!sessionLabel.isEmpty()) {
        sessionPart = QString(" [session: %1]").arg(sessionLabel);
    }

    return QString("[%1]%2 %3%4: %5").arg(timestamp, sessionPart, role.toUpper(), tools, preview);
}

QString MemoryManager::buildSessionDigest() const {
    MessageList messages = m_history->getAll();
    if (messages.isEmpty()) {
        return QString();
    }

    QStringList lines;
    QString sessionLabel = m_currentSessionName.isEmpty() ? m_currentSessionId : m_currentSessionName;
    lines.append(QString("## Session Snapshot: %1").arg(sessionLabel.isEmpty() ? QStringLiteral("active") : sessionLabel));
    lines.append(QString("- Captured at: %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm")));
    lines.append(QString("- Message count: %1").arg(messages.size()));
    lines.append(QString("- Token estimate: %1").arg(m_history->totalTokens()));
    lines.append("- Recent context:");

    const int keepFirst = qMin(m_config.keepFirst, messages.size());
    const int keepLast = qMin(m_config.keepLast, qMax(0, messages.size() - keepFirst));
    const int startIndex = qMin(keepFirst, messages.size());
    const int endIndex = qMax(startIndex, messages.size() - keepLast);

    for (int i = startIndex; i < endIndex; ++i) {
        const Message& message = messages.at(i);
        QString role;
        switch (message.role) {
            case MessageRole::User: role = "User"; break;
            case MessageRole::Assistant: role = "Assistant"; break;
            case MessageRole::System: role = "System"; break;
            case MessageRole::Tool: role = "Tool"; break;
        }
        lines.append(QString("  - %1: %2").arg(role, previewText(message.content)));
    }

    return lines.join("\n");
}

QString MemoryManager::previewText(const QString& text, int maxLength) const {
    QString preview = text.simplified();
    if (preview.length() > maxLength) {
        preview = preview.left(maxLength) + "...";
    }
    return preview;
}

void MemoryManager::addMessage(const Message& message) {
    m_history->append(message);

    QStringList toolsUsed;
    for (const ToolCallData& toolCall : message.toolCalls) {
        if (!toolCall.name.isEmpty()) {
            toolsUsed.append(toolCall.name);
        }
    }
    if (message.role == MessageRole::Tool && !message.name.isEmpty()) {
        toolsUsed.append(message.name);
    }

    appendHistoryEntry(
        message.role == MessageRole::User ? "user" :
        message.role == MessageRole::Assistant ? "assistant" :
        message.role == MessageRole::System ? "system" : "tool",
        message.content,
        toolsUsed);

    maybeCaptureAutoMemory(message);

    emit messageAdded(message);

    if (needsCompression()) {
        compress();
    }
}

void MemoryManager::setSessionContext(const QString& sessionId, const QString& sessionName) {
    QString normalizedSessionId = sessionId.trimmed();
    QString normalizedSessionName = sessionName.trimmed();
    if (normalizedSessionName.isEmpty()) {
        normalizedSessionName = normalizedSessionId;
    }

    if (m_currentSessionId == normalizedSessionId && m_currentSessionName == normalizedSessionName) {
        return;
    }

    // 记录旧会话关闭事件，便于后续问题追溯。
    if (!m_currentSessionId.isEmpty()) {
        appendHistory(QString("[%1] [session: %2] SYSTEM: Session closed").arg(
            QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"),
            m_currentSessionName.isEmpty() ? m_currentSessionId : m_currentSessionName));
    }

    m_currentSessionId = normalizedSessionId;
    m_currentSessionName = normalizedSessionName;

    // 会话切换后重置内存态上下文，由 SessionManager 通过 setContext 注入新会话消息。
    m_history->clear();

    if (!m_currentSessionId.isEmpty()) {
        appendHistory(QString("[%1] [session: %2] SYSTEM: Session started").arg(
            QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"),
            m_currentSessionName.isEmpty() ? m_currentSessionId : m_currentSessionName));
    }
}

void MemoryManager::setContext(const MessageList& messages) {
    m_history->setMessages(messages);
}

MessageList MemoryManager::getRecentMessages(int count) const {
    return m_history->getLast(count);
}

MessageList MemoryManager::getAllMessages() const {
    return m_history->getAll();
}

MessageList MemoryManager::getContext() const {
    return m_history->getAll();
}

void MemoryManager::clear() {
    m_history->clear();
}

int MemoryManager::getTokenCount() const {
    return m_history->totalTokens();
}

void MemoryManager::setCompressionTokenThreshold(int threshold) {
    m_runtimeTokenThreshold = qMax(0, threshold);
}

int MemoryManager::compressionTokenThreshold() const {
    return effectiveTokenThreshold();
}

bool MemoryManager::needsCompression() const {
    return getTokenCount() > effectiveTokenThreshold();
}

void MemoryManager::compress() {
    compressInternal(false);
}

bool MemoryManager::forceCompress() {
    return compressInternal(true);
}

bool MemoryManager::compressInternal(bool force) {
    if (!force && !needsCompression()) {
        return false;
    }

    const int keepTotal = m_config.keepFirst + m_config.keepLast;
    if (m_history->count() <= keepTotal) {
        return false;
    }

    emit compressionStarted();

    // 压缩前先将会话中段摘要持久化到长期记忆，避免信息直接丢失。
    const QString digest = buildSessionDigest();
    if (!digest.isEmpty()) {
        saveMemoryUpdate(
            QString("[%1] [session: %2] SYSTEM: Session memory compressed").arg(
                QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"),
                m_currentSessionName.isEmpty() ? m_currentSessionId : m_currentSessionName),
            digest);
    }

    m_history->compressMiddle(
        m_config.keepFirst,
        m_config.keepLast,
        m_config.summaryRatio
    );

    emit compressionCompleted();
    return true;
}

int MemoryManager::effectiveTokenThreshold() const {
    if (m_runtimeTokenThreshold > 0) {
        return m_runtimeTokenThreshold;
    }
    return qMax(1, m_config.tokenThreshold);
}

QStringList MemoryManager::queryRelevantMemory(const QString& query, int topK) const {
    const QString trimmed = query.trimmed();
    QStringList result;
    const QString normalizedQuery = trimmed.toLower().simplified();
    if (normalizedQuery.isEmpty()) {
        return result;
    }

    const QStringList queryTerms = normalizedQuery.split(
        QRegularExpression(QStringLiteral("[^\\p{L}\\p{N}_]+")),
        Qt::SkipEmptyParts);

    struct Candidate {
        QString content;
        double score = 0.0;
        int order = 0;
    };

    QVector<Candidate> candidates;
    int order = 0;

    auto scoreBlock = [&](const QString& block, double baseWeight) {
        QString snippet = block.simplified();
        if (snippet.isEmpty()) {
            return;
        }

        const QString lower = snippet.toLower();
        double score = 0.0;
        for (const QString& term : queryTerms) {
            if (term.isEmpty()) {
                continue;
            }
            if (lower.contains(term)) {
                score += 1.0;
            }
        }
        if (lower.contains(normalizedQuery)) {
            score += 1.5;
        }
        if (score <= 0.0) {
            return;
        }

        if (snippet.length() > 260) {
            snippet = snippet.left(260) + QStringLiteral("...");
        }

        candidates.append(Candidate{snippet, score * baseWeight, order++});
    };

    auto splitAndScore = [&](const QString& source, double baseWeight) {
        if (source.trimmed().isEmpty()) {
            return;
        }
        const QStringList blocks = source.split(
            QRegularExpression(QStringLiteral("\\n\\s*\\n+")),
            Qt::SkipEmptyParts);
        for (const QString& block : blocks) {
            scoreBlock(block, baseWeight);
        }
    };

    splitAndScore(readLongTerm(), 1.4);
    splitAndScore(readHistoryLog(), 1.0);

    const MessageList recentMessages = m_history->getAll();
    for (const Message& message : recentMessages) {
        QString roleLabel;
        switch (message.role) {
            case MessageRole::User: roleLabel = QStringLiteral("User"); break;
            case MessageRole::Assistant: roleLabel = QStringLiteral("Assistant"); break;
            case MessageRole::System: roleLabel = QStringLiteral("System"); break;
            case MessageRole::Tool: roleLabel = QStringLiteral("Tool"); break;
        }

        const QString decorated = QStringLiteral("%1: %2").arg(roleLabel, message.content);
        scoreBlock(decorated, 0.8);
    }

    std::sort(candidates.begin(), candidates.end(), [](const Candidate& left, const Candidate& right) {
        if (!qFuzzyCompare(left.score + 1.0, right.score + 1.0)) {
            return left.score > right.score;
        }
        return left.order < right.order;
    });

    const int safeTopK = qBound(1, topK, 20);
    QSet<QString> seen;
    for (const Candidate& candidate : std::as_const(candidates)) {
        if (result.size() >= safeTopK) {
            break;
        }
        if (seen.contains(candidate.content)) {
            continue;
        }
        seen.insert(candidate.content);
        result.append(previewText(candidate.content, 220));
    }

    if (result.isEmpty()) {
        const MessageList fallback = m_history->getLast(qMin(safeTopK, 3));
        for (const Message& message : fallback) {
            if (result.size() >= safeTopK) {
                break;
            }
            result.append(previewText(message.content, 220));
        }
    }

    return result;
}

void MemoryManager::appendLongTermMemory(const QString& content) {
    const QString trimmed = content.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }

    const QString current = readLongTerm().trimmed();
    if (current.isEmpty()) {
        writeLongTerm(trimmed);
        return;
    }

    writeLongTerm(current + QStringLiteral("\n\n") + trimmed);
}

void MemoryManager::maybeCaptureAutoMemory(const Message& message) {
    if (message.role != MessageRole::User && message.role != MessageRole::Assistant) {
        return;
    }

    QString note;
    if (!shouldCaptureAutoMemory(message.content, &note)) {
        return;
    }

    const QString current = readLongTerm();
    if (hasExistingMemoryNote(note, current)) {
        return;
    }

    QString merged = current.trimmed();
    if (merged.isEmpty()) {
        merged = memoryIndexTemplate().trimmed();
    }

    const QString entry = buildAutoMemoryEntry(
        message.role == MessageRole::User ? QStringLiteral("user") : QStringLiteral("assistant"),
        note);

    const QString sectionHeader = QStringLiteral("## Auto Memory Notes");
    const QString nextHeaderPattern = QStringLiteral("\n## ");
    int sectionIndex = merged.indexOf(sectionHeader);
    if (sectionIndex < 0) {
        merged += QStringLiteral("\n\n## Auto Memory Notes\n\n- %1").arg(entry);
    } else {
        const int sectionBodyStart = merged.indexOf('\n', sectionIndex);
        if (sectionBodyStart < 0) {
            merged += QStringLiteral("\n\n- %1").arg(entry);
        } else {
            int nextHeader = merged.indexOf(nextHeaderPattern, sectionBodyStart + 1);
            if (nextHeader < 0) {
                nextHeader = merged.size();
            }
            QString sectionBody = merged.mid(sectionBodyStart + 1, nextHeader - sectionBodyStart - 1).trimmed();
            if (sectionBody == QStringLiteral("(none)") || sectionBody == QStringLiteral("（暂无）")) {
                sectionBody.clear();
            }

            QString rebuiltSection = QStringLiteral("\n");
            if (!sectionBody.isEmpty()) {
                rebuiltSection += sectionBody + QStringLiteral("\n");
            }
            rebuiltSection += QStringLiteral("- %1\n").arg(entry);

            merged.replace(sectionBodyStart, nextHeader - sectionBodyStart, rebuiltSection);
        }
    }

    writeLongTerm(merged);
}

bool MemoryManager::shouldCaptureAutoMemory(const QString& text, QString* note) const {
    if (!note) {
        return false;
    }

    QString normalized = text.simplified();
    if (normalized.length() < 10 || normalized.length() > 320) {
        return false;
    }

    static const QRegularExpression triggerRegex(
        QStringLiteral("(记住|牢记|以后|默认|偏好|不要|必须|务必|remember|always|prefer|never|do not|don't)"),
        QRegularExpression::CaseInsensitiveOption);
    if (!triggerRegex.match(normalized).hasMatch()) {
        return false;
    }

    normalized.remove(QRegularExpression(
        QStringLiteral("^(请|麻烦|请你|请务必|please)\\s*"),
        QRegularExpression::CaseInsensitiveOption));
    normalized.remove(QRegularExpression(
        QStringLiteral("^(记住|remember)\\s*[:：,，-]*\\s*"),
        QRegularExpression::CaseInsensitiveOption));
    normalized = normalized.simplified();

    if (normalized.length() < 8) {
        return false;
    }
    if (normalized.length() > 220) {
        normalized = normalized.left(220) + QStringLiteral("...");
    }

    *note = normalized;
    return true;
}

bool MemoryManager::hasExistingMemoryNote(const QString& note, const QString& content) const {
    const QString normalizedNote = note.simplified().toLower();
    const QString normalizedContent = content.simplified().toLower();
    return !normalizedNote.isEmpty() && normalizedContent.contains(normalizedNote);
}

QString MemoryManager::buildAutoMemoryEntry(const QString& role, const QString& note) const {
    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm"));
    const QString sessionLabel = m_currentSessionName.isEmpty()
        ? (m_currentSessionId.isEmpty() ? QStringLiteral("active") : m_currentSessionId)
        : m_currentSessionName;
    return QStringLiteral("[%1] [session: %2] [%3] %4").arg(timestamp, sessionLabel, role, note);
}

QString MemoryManager::memoryIndexTemplate() const {
    return QStringLiteral(
        "# MEMORY\n\n"
        "> Auto-maintained long-term memory index for this workspace.\n\n"
        "## Auto Memory Notes\n\n"
        "（暂无）\n\n"
        "## Session Digests\n\n"
        "（暂无）");
}

}
