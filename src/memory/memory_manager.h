#ifndef CLAWPP_MEMORY_MEMORY_MANAGER_H
#define CLAWPP_MEMORY_MEMORY_MANAGER_H

#include <QObject>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include "i_memory_system.h"
#include "common/types.h"

namespace clawpp {

class ConversationHistory;

/**
 * @class MemoryManager
 * @brief 负责短期对话上下文与工作区长期记忆文件的统一管理。
 *
 * 设计要点：
 * - 会话内上下文由 ConversationHistory 维护（内存态）。
 * - 跨会话记忆落盘到 workspace/memory 目录（文件态）。
 * - 当 token 超阈值时执行中段压缩，避免上下文无限增长。
 */
class MemoryManager : public IMemorySystem {
    Q_OBJECT

public:
    explicit MemoryManager(const QString& workspace, QObject* parent = nullptr);
    explicit MemoryManager(const MemoryConfig& config, QObject* parent = nullptr);
    ~MemoryManager();
    
    void addMessage(const Message& message) override;
    void setSessionContext(const QString& sessionId, const QString& sessionName = QString()) override;
    void setContext(const MessageList& messages) override;
    MessageList getRecentMessages(int count) const override;
    MessageList getAllMessages() const override;
    MessageList getContext() const override;
    void clear() override;
    int getTokenCount() const override;
    void setCompressionTokenThreshold(int threshold);
    int compressionTokenThreshold() const;
    bool needsCompression() const override;
    void compress() override;
    bool forceCompress();
    QStringList queryRelevantMemory(const QString& query, int topK = 6) const override;
    
    void setWorkspace(const QString& workspace);
    QString workspace() const;
    
    QString readLongTerm() const;
    QString readHistoryLog() const;
    void writeLongTerm(const QString& content);
    void appendHistory(const QString& entry);
    void clearPersistentStorage();
    QString getMemoryContext() const;
    
    void appendHistoryEntry(const QString& role, const QString& content, const QStringList& toolsUsed = QStringList());
    void saveMemoryUpdate(const QString& historyEntry, const QString& memoryUpdate);

signals:
    void messageAdded(const Message& message);
    void compressionStarted();
    void compressionCompleted();
    void memoryUpdated();

private:
    void ensureMemoryDirectory();
    void ensureMemorySeedFile();
    QString formatHistoryEntry(const QString& role, const QString& content, const QStringList& toolsUsed);
    QString buildSessionDigest() const;
    QString previewText(const QString& text, int maxLength = 160) const;
    void appendLongTermMemory(const QString& content);
    void maybeCaptureAutoMemory(const Message& message);
    bool shouldCaptureAutoMemory(const QString& text, QString* note) const;
    bool hasExistingMemoryNote(const QString& note, const QString& content) const;
    QString buildAutoMemoryEntry(const QString& role, const QString& note) const;
    QString memoryIndexTemplate() const;
    bool compressInternal(bool force);
    int effectiveTokenThreshold() const;

    QString m_workspace;
    MemoryConfig m_config;
    ConversationHistory* m_history; // QObject 子对象，由 Qt 父子关系托管生命周期。
    QString m_memoryFilePath;
    QString m_historyFilePath;
    QString m_currentSessionId;
    QString m_currentSessionName;
    int m_runtimeTokenThreshold;
};

}

#endif
