#ifndef CLAWPP_UI_MAIN_WINDOW_H
#define CLAWPP_UI_MAIN_WINDOW_H

#include <QAction>
#include <QComboBox>
#include <QDir>
#include <QDialog>
#include <QFrame>
#include <QHash>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMessageBox>
#include <QPoint>
#include <QPushButton>
#include <QSlider>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolButton>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>

class QEvent;

#include "application/agent_service.h"
#include "application/session_manager.h"
#include "infrastructure/config/config_manager.h"
#include "infrastructure/network/external_platform_manager.h"
#include "memory/memory_manager.h"
#include "provider/provider_manager.h"
#include "session_panel.h"

namespace clawpp {

class ChatView;
class LogView;

/**
 * @class MainWindow
 * @brief 桌面端主窗口，负责 UI 容器搭建与各业务服务的总装配。
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

    SessionPanel* sessionPanel() const;
    ChatView* chatView() const;
    SessionManager* sessionManager() const;

private slots:
    void onSettings();
    void onMemoryDashboard();
    void onAbout();
    void onSidebarSliderChanged(int value);
    void onTabClicked(int index);
    void onClearSession();
    void onOpenMemoryManager();

    void onSessionCreated();
    void onSessionCreated(const QString& name);
    void onSessionSelected(const QString& sessionId);
    void onSessionRenamed(const QString& sessionId, const QString& name);
    void onSessionDeleted(const QString& sessionId);
    void onSessionCopied(const QString& sessionId);
    void onSessionImportRequested();
    void onSessionPinned(const QString& sessionId, bool pinned);
    void onSessionExported(const QString& sessionId, const QString& fileName);
    void onSessionManagerCreated(const QString& id, const QString& name);
    void onSessionManagerSwitched(const QString& id);
    void onSessionManagerDeleted(const QString& id);

    void updateTokenCount(int prompt, int completion, int total);
    void updateConnectionStatus(bool connected);
    void updateContextIndicator(int tokenCount, int limit);  // 新增：更新上下文指示器
    void onToolCallParsed(const ToolCall& toolCall);
    void onToolPreExecutionStarted(const QString& toolId);
    void onToolPreExecutionCompleted(const QString& toolId);

private:
    void setupUi();
    void setupModernUI();
    void setupConnections();
    void setupServices();
    void refreshSessionList();
    void updateWindowControls();
    QWidget* createLeftPanel();
    QWidget* createRightPanel();
    void updateSidebarState(int value);
    bool eventFilter(QObject* watched, QEvent* event) override;

    ChatView* m_chatView = nullptr;
    SessionPanel* m_sessionPanel = nullptr;
    LogView* m_logView = nullptr;
    SessionManager* m_sessionManager = nullptr;
    AgentService* m_agentService = nullptr;
    ProviderManager* m_providerManager = nullptr;
    MemoryManager* m_memoryManager = nullptr;
    ExternalPlatformManager* m_externalPlatformManager = nullptr;
    QSplitter* m_splitter = nullptr;
    QStackedWidget* m_leftStack = nullptr;
    QWidget* m_topBar = nullptr;
    QLabel* m_windowTitleLabel = nullptr;
    QLabel* m_windowSubtitleLabel = nullptr;
    QToolButton* m_minimizeButton = nullptr;
    QToolButton* m_maximizeButton = nullptr;
    QToolButton* m_closeButton = nullptr;
    QPushButton* m_sessionTabBtn = nullptr;
    QPushButton* m_logTabBtn = nullptr;
    QLabel* m_sidebarLabel = nullptr;
    QStatusBar* m_statusBar = nullptr;
    QLabel* m_tokenLabel = nullptr;
    QLabel* m_connectionLabel = nullptr;
    QLabel* m_operationLabel = nullptr;
    QLabel* m_contextIndicator = nullptr;  // 新增：上下文状态指示器
    QComboBox* m_modelCombo = nullptr;
    bool m_isWindowDragging = false;
    QPoint m_windowDragOffset;

    struct ExternalConversationContext {
        QString platform;
        QString userId;
        QString messageId;
    };

    QHash<QString, ExternalConversationContext> m_externalConversationContexts;
};

} // namespace clawpp

#endif
