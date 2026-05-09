#ifndef CLAWPP_QML_BACKEND_H
#define CLAWPP_QML_BACKEND_H

#include <QObject>
#include <QPointer>
#include <QUrl>
#include <QVariantMap>
#include <QVariantList>
#include <QHash>
#include <QTimer>
#include "qml_models.h"
#include "application/session_manager.h"
#include "application/agent_service.h"
#include "application/template_loader.h"
#include "infrastructure/config/config_manager.h"
#include "infrastructure/network/external_platform_manager.h"
#include "memory/memory_manager.h"
#include "provider/provider_manager.h"

namespace clawpp {

class QmlBackend : public QObject {
    Q_OBJECT
    Q_PROPERTY(SessionListModel* sessionsModel READ sessionsModel CONSTANT)
    Q_PROPERTY(MessageListModel* messagesModel READ messagesModel CONSTANT)
    Q_PROPERTY(QString currentSessionId READ currentSessionId NOTIFY currentSessionChanged)
    Q_PROPERTY(QString currentSessionName READ currentSessionName NOTIFY currentSessionChanged)
    Q_PROPERTY(QString currentModelName READ currentModelName NOTIFY currentModelNameChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString toolHintText READ toolHintText NOTIFY toolHintTextChanged)
    Q_PROPERTY(QString usageText READ usageText NOTIFY usageTextChanged)
    Q_PROPERTY(bool generating READ generating NOTIFY generatingChanged)

public:
    explicit QmlBackend(QObject* parent = nullptr);
    ~QmlBackend() override;

    SessionListModel* sessionsModel() const;
    MessageListModel* messagesModel() const;

    QString currentSessionId() const;
    QString currentSessionName() const;
    QString currentModelName() const;
    QString statusText() const;
    QString toolHintText() const;
    QString usageText() const;
    bool generating() const;

    Q_INVOKABLE void createSession(const QString& name = QString());
    Q_INVOKABLE void selectSession(const QString& sessionId);
    Q_INVOKABLE void refreshSessions();
    Q_INVOKABLE void importSession(const QUrl& fileUrl);
    Q_INVOKABLE void renameCurrentSession(const QString& name);
    Q_INVOKABLE void renameSessionById(const QString& sessionId, const QString& name);
    Q_INVOKABLE void deleteCurrentSession();
    Q_INVOKABLE void deleteSessionById(const QString& sessionId);
    Q_INVOKABLE int deleteSessionsByIds(const QVariantList& sessionIds);
    Q_INVOKABLE int deleteUnpinnedSessions();
    Q_INVOKABLE void togglePinCurrentSession();
    Q_INVOKABLE void togglePinSessionById(const QString& sessionId);
    Q_INVOKABLE void sendMessage(const QString& content);
    Q_INVOKABLE void deleteMessageAt(int row);
    Q_INVOKABLE void stopGeneration();
    Q_INVOKABLE QString workspacePath() const;
    Q_INVOKABLE QString kernelPath() const;
    Q_INVOKABLE QString configFilePath() const;
    Q_INVOKABLE QString logDirectoryPath() const;
    Q_INVOKABLE QString memoryDirectoryPath() const;
    Q_INVOKABLE QString readWorkspaceFile(const QString& relativePath) const;
    Q_INVOKABLE bool writeWorkspaceFile(const QString& relativePath, const QString& content);
    Q_INVOKABLE QVariantMap getSettings() const;
    Q_INVOKABLE QVariantMap getCompressionInfo() const;
    Q_INVOKABLE QVariantMap getMemoryOverview() const;
    Q_INVOKABLE QVariantList searchMemory(const QString& query, int topK = 6) const;
    Q_INVOKABLE bool appendMemoryNote(const QString& note);
    Q_INVOKABLE bool clearMemoryStorage();
    Q_INVOKABLE QVariantList getSkillsOverview() const;
    Q_INVOKABLE QString reloadSkills();
    Q_INVOKABLE QStringList getSkillLoadErrors() const;
    Q_INVOKABLE QVariantMap getTokenStats() const;
    Q_INVOKABLE QVariantMap getTokenUsageAnalytics(int days = 30, int year = 0) const;
    Q_INVOKABLE bool resetTokenUsage(const QString& scope, int year = 0, int month = 0);
    Q_INVOKABLE bool applySettings(const QVariantMap& settings);
    Q_INVOKABLE bool canSendMessage() const;
    Q_INVOKABLE QString quickApiProbe();
    Q_INVOKABLE QString runQuickSubagentTask(const QString& task);
    Q_INVOKABLE QVariantList getWorkflowTemplates() const;
    Q_INVOKABLE QString startAgentWorkflow(const QString& templateId,
                                           const QString& mode,
                                           const QString& task);
    Q_INVOKABLE void clearCurrentContext();
    Q_INVOKABLE QString compressCurrentContext();
    Q_INVOKABLE QString exportCurrentSession();
    Q_INVOKABLE QString testProviderConfig(const QString& providerType,
                                          const QString& apiKey,
                                          const QString& baseUrl,
                                          const QString& model);
    Q_INVOKABLE QString testExternalConfig(const QString& telegramToken,
                                          const QString& feishuAppId,
                                          const QString& feishuAppSecret) const;

signals:
    void currentSessionChanged();
    void currentModelNameChanged();
    void statusTextChanged();
    void toolHintTextChanged();
    void usageTextChanged();
    void generatingChanged();

private:
    void initializeCore();
    void setupConnections();
    void syncCurrentSession();
    void setGenerating(bool generating);
    void setToolHintText(const QString& hint);
    void syncCompressionThresholdFromModel();
    void updateUsage(int promptTokens, int completionTokens, int totalTokens);
    void flushPendingAssistantChunk(bool force = false);
    void runAssistantWithInternalPrompt(const QString& prompt);
    void persistUsageSnapshotIfNeeded();
    void ensureSessionExists();
    QString tokenUsageHistoryPath() const;
    void loadTokenUsageHistory();
    bool saveTokenUsageHistory() const;
    void rebuildTokenUsageAccumulators();

    static void registerTools();

    SessionListModel* m_sessionsModel;
    MessageListModel* m_messagesModel;
    SessionManager* m_sessionManager;
    AgentService* m_agentService;
    ProviderManager* m_providerManager;
    MemoryManager* m_memoryManager;
    ExternalPlatformManager* m_externalPlatformManager;
    TemplateLoader* m_templateLoader;
    QPointer<QObject> m_parentGuard;
    QString m_workspaceRoot;
    QString m_kernelRoot;

    QString m_statusText;
    QString m_toolHintText;
    QString m_usageText;
    QString m_currentSessionId;
    QString m_currentSessionName;
    bool m_generating;
    int m_promptTokens;
    int m_completionTokens;
    int m_totalTokens;
    int m_lastApiContextTokens;
    int m_accPromptTokens;
    int m_accCompletionTokens;
    int m_accTotalTokens;
    QVariantList m_tokenUsageHistory;
    bool m_usageDirtyCurrentRun;
    int m_streamingAssistantRow;
    QString m_pendingAssistantChunk;
    QTimer* m_chunkFlushTimer;
    bool m_providerTestPassed = false;

    struct ExternalConversationContext {
        QString platform;
        QString userId;
        QString messageId;
    };
    QHash<QString, ExternalConversationContext> m_externalConversationContexts;
};

} // namespace clawpp

#endif
