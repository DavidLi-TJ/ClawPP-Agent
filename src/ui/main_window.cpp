#include "main_window.h"
#include "chat_view.h"
#include "views/log_view.h"
#include "application/session_manager.h"
#include "application/template_loader.h"
#include "agent/context_builder.h"
#include "agent/agent_profile_manager.h"
#include "dialogs/extracted_dialogs.h"
#include "application/agent_orchestrator.h"
#include "infrastructure/database/database_manager.h"
#include "ui/dialogs/permission_dialog.h"
#include "ui/dialogs/memory_dashboard_dialog.h"
#include "common/model_context_limits.h"
#include <QInputDialog>
#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QWidgetAction>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QDateTime>
#include <QStyle>
#include <QTimer>
#include <QEvent>
#include <QMouseEvent>
#include <QGuiApplication>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QGraphicsDropShadowEffect>
#include <functional>
#include <QtConcurrent/QtConcurrent>

namespace clawpp {

namespace {

bool isWorkspaceRoot(const QString& path) {
    const QDir dir(path);
    return dir.exists(QStringLiteral("src"))
        && dir.exists(QStringLiteral("templates"))
        && dir.exists(QStringLiteral("config"));
}

QString detectWorkspaceRoot() {
    QStringList seeds;
    seeds.append(QDir::currentPath());

    const QString appDir = QCoreApplication::applicationDirPath();
    if (!appDir.isEmpty() && !seeds.contains(appDir)) {
        seeds.append(appDir);
    }

    for (const QString& seed : seeds) {
        QDir dir(seed);
        for (int depth = 0; depth < 8; ++depth) {
            const QString candidate = dir.absolutePath();
            if (isWorkspaceRoot(candidate)) {
                return candidate;
            }

            if (!dir.cdUp()) {
                break;
            }
        }
    }

    return QDir::currentPath();
}

}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    // 轻量启动优化：提前异步初始化数据库，减少主线程阻塞感。
    QFuture<void> dbInitFuture = QtConcurrent::run([]() {
        DatabaseManager::instance().initialize();
    });
    m_sessionManager = new SessionManager(this);
    m_isWindowDragging = false;

    setupUi();
    setupModernUI();
    dbInitFuture.waitForFinished();
    setupServices();
    setupConnections();
    refreshSessionList();

    setWindowOpacity(0.0);
    QTimer::singleShot(0, this, [this]() {
        auto* fadeAnimation = new QPropertyAnimation(this, "windowOpacity", this);
        fadeAnimation->setDuration(240);
        fadeAnimation->setStartValue(0.0);
        fadeAnimation->setEndValue(1.0);
        fadeAnimation->setEasingCurve(QEasingCurve::OutCubic);
        fadeAnimation->start(QAbstractAnimation::DeleteWhenStopped);
    });
}

SessionPanel* MainWindow::sessionPanel() const {
    return m_sessionPanel;
}

ChatView* MainWindow::chatView() const {
    return m_chatView;
}

SessionManager* MainWindow::sessionManager() const {
    return m_sessionManager;
}

void MainWindow::setupUi() {
    setWindowTitle(QString::fromUtf8("Claw++"));
    setMinimumSize(1280, 760);
    resize(1560, 920);
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);

    m_statusBar = new QStatusBar(this);
    m_statusBar->setObjectName("appStatusBar");
    setStatusBar(m_statusBar);
    m_statusBar->hide();

    QWidget* centralWidget = new QWidget(this);
    centralWidget->setObjectName("mainWindowRoot");
    auto* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(0);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setObjectName("mainSplitter");
    m_splitter->setHandleWidth(1);
    m_splitter->setChildrenCollapsible(false);

    m_splitter->addWidget(createLeftPanel());
    m_splitter->addWidget(createRightPanel());
    m_splitter->setSizes(QList<int>() << 360 << 1220);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(m_splitter);
    setCentralWidget(centralWidget);
}

void MainWindow::setupModernUI() {
    setMenuBar(nullptr);

    m_topBar = new QWidget();
    QWidget* topBar = m_topBar;
    topBar->setObjectName("topBar");
    topBar->setFixedHeight(52);
    topBar->installEventFilter(this);

    auto* topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(16, 0, 12, 0);
    topLayout->setSpacing(10);

    QWidget* leftSection = new QWidget(topBar);
    auto* leftLayout = new QHBoxLayout(leftSection);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    QPushButton* sessionBadge = new QPushButton(QString::fromUtf8("default ·"));
    sessionBadge->setObjectName("topBarSessionBadge");
    sessionBadge->setCursor(Qt::PointingHandCursor);
    leftLayout->addWidget(sessionBadge);

    QLabel* statusDot = new QLabel("●");
    statusDot->setObjectName("topBarStatusDot");
    leftLayout->addWidget(statusDot);

    QLabel* statusLabel = new QLabel(QString::fromUtf8("运行中"));
    statusLabel->setObjectName("topBarStatusLabel");
    leftLayout->addWidget(statusLabel);

    QLabel* separator1 = new QLabel("·");
    separator1->setObjectName("topBarSeparator");
    leftLayout->addWidget(separator1);

    QPushButton* pageVisitBtn = new QPushButton(QString::fromUtf8("网页访问"));
    pageVisitBtn->setObjectName("topBarActionBtn");
    pageVisitBtn->setCursor(Qt::PointingHandCursor);
    leftLayout->addWidget(pageVisitBtn);

    QPushButton* remoteAddrBtn = new QPushButton(QString::fromUtf8("远程地址"));
    remoteAddrBtn->setObjectName("topBarActionBtn");
    remoteAddrBtn->setCursor(Qt::PointingHandCursor);
    leftLayout->addWidget(remoteAddrBtn);

    QLabel* separator2 = new QLabel("·");
    separator2->setObjectName("topBarSeparator");
    leftLayout->addWidget(separator2);

    QLabel* endpointLabel = new QLabel(QString::fromUtf8("1 端点"));
    endpointLabel->setObjectName("topBarEndpointLabel");
    leftLayout->addWidget(endpointLabel);

    leftLayout->addStretch();
    topLayout->addWidget(leftSection, 1);

    QWidget* rightSection = new QWidget(topBar);
    auto* rightLayout = new QHBoxLayout(rightSection);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    auto createIconBtn = [this](const QString& icon, const QString& tooltip) {
        QToolButton* btn = new QToolButton();
        btn->setObjectName("topBarIconBtn");
        btn->setText(icon);
        btn->setToolTip(tooltip);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedSize(32, 32);
        return btn;
    };

    rightLayout->addWidget(createIconBtn(QString::fromUtf8("↩"), QString::fromUtf8("撤销")));
    rightLayout->addWidget(createIconBtn(QString::fromUtf8("⟲"), QString::fromUtf8("刷新")));
    rightLayout->addWidget(createIconBtn(QString::fromUtf8("☀"), QString::fromUtf8("主题")));
    rightLayout->addWidget(createIconBtn(QString::fromUtf8("🌐"), QString::fromUtf8("设置")));

    QWidget* windowControls = new QWidget(topBar);
    windowControls->setObjectName("windowControls");
    auto* windowControlsLayout = new QHBoxLayout(windowControls);
    windowControlsLayout->setContentsMargins(8, 0, 0, 0);
    windowControlsLayout->setSpacing(4);

    auto createWindowButton = [this, windowControls](const QString& text, const QString& tooltip, const QString& objectName, const std::function<void()>& callback) {
        QToolButton* button = new QToolButton(windowControls);
        button->setObjectName(objectName);
        button->setText(text);
        button->setToolTip(tooltip);
        button->setCursor(Qt::PointingHandCursor);
        button->setFixedSize(34, 28);
        connect(button, &QToolButton::clicked, this, [callback]() { callback(); });
        return button;
    };

    m_minimizeButton = createWindowButton(QString::fromUtf8("—"), QString::fromUtf8("最小化"), "windowControlButton", [this]() { showMinimized(); });
    m_maximizeButton = createWindowButton(QString::fromUtf8("▢"), QString::fromUtf8("最大化"), "windowControlButton", [this]() {
        if (isMaximized()) {
            showNormal();
        } else {
            showMaximized();
        }
        updateWindowControls();
    });
    m_closeButton = createWindowButton(QString::fromUtf8("×"), QString::fromUtf8("关闭"), "windowCloseButton", [this]() { close(); });

    windowControlsLayout->addWidget(m_minimizeButton);
    windowControlsLayout->addWidget(m_maximizeButton);
    windowControlsLayout->addWidget(m_closeButton);
    rightLayout->addWidget(windowControls);

    topLayout->addWidget(rightSection);

    setMenuWidget(topBar);
    updateWindowControls();
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_topBar || watched == m_windowTitleLabel || watched == m_windowSubtitleLabel) {
        if (event->type() == QEvent::MouseButtonDblClick) {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                if (isMaximized()) {
                    showNormal();
                } else {
                    showMaximized();
                }
                updateWindowControls();
                return true;
            }
        }

        if (event->type() == QEvent::MouseButtonPress) {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton && !isMaximized()) {
                m_isWindowDragging = true;
                m_windowDragOffset = mouseEvent->globalPosition().toPoint() - frameGeometry().topLeft();
                return true;
            }
        }

        if (event->type() == QEvent::MouseMove) {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            if (m_isWindowDragging && (mouseEvent->buttons() & Qt::LeftButton) && !isMaximized()) {
                move(mouseEvent->globalPosition().toPoint() - m_windowDragOffset);
                return true;
            }
        }

        if (event->type() == QEvent::MouseButtonRelease) {
            m_isWindowDragging = false;
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::updateWindowControls() {
    if (!m_maximizeButton) {
        return;
    }

    const bool maximized = isMaximized();
    m_maximizeButton->setText(maximized ? QString::fromUtf8("❐") : QString::fromUtf8("▢"));
    m_maximizeButton->setToolTip(maximized ? QString::fromUtf8("还原窗口") : QString::fromUtf8("最大化"));
}

QWidget* MainWindow::createLeftPanel() {
    QWidget* panel = new QWidget();
    panel->setObjectName("sidebarShell");

    auto* mainLayout = new QVBoxLayout(panel);
    mainLayout->setContentsMargins(18, 18, 18, 18);
    mainLayout->setSpacing(14);

    QFrame* heroCard = new QFrame(panel);
    heroCard->setObjectName("sidebarHeroCard");
    auto* heroLayout = new QVBoxLayout(heroCard);
    heroLayout->setContentsMargins(18, 18, 18, 18);
    heroLayout->setSpacing(12);

    QWidget* appHeader = new QWidget(heroCard);
    auto* appHeaderLayout = new QHBoxLayout(appHeader);
    appHeaderLayout->setContentsMargins(0, 0, 0, 0);
    appHeaderLayout->setSpacing(12);

    QLabel* appIcon = new QLabel("🐯");
    appIcon->setObjectName("sidebarAppIcon");
    appIcon->setAlignment(Qt::AlignCenter);
    appIcon->setFixedSize(44, 44);
    appHeaderLayout->addWidget(appIcon);

    QWidget* appTitleWidget = new QWidget(appHeader);
    auto* appTitleLayout = new QVBoxLayout(appTitleWidget);
    appTitleLayout->setContentsMargins(0, 0, 0, 0);
    appTitleLayout->setSpacing(2);

    QLabel* appTitle = new QLabel(QString::fromUtf8("Claw++"));
    appTitle->setObjectName("sidebarAppTitle");
    appTitleLayout->addWidget(appTitle);

    QLabel* appSubtitle = new QLabel(QString::fromUtf8("桌面控制面板"));
    appSubtitle->setObjectName("sidebarAppSubtitle");
    appTitleLayout->addWidget(appSubtitle);

    appHeaderLayout->addWidget(appTitleWidget, 1);
    heroLayout->addWidget(appHeader);

    QLabel* workspaceHint = new QLabel(QString::fromUtf8("左侧控制，右侧工作区"));
    workspaceHint->setObjectName("sidebarHeroHint");
    heroLayout->addWidget(workspaceHint);
    mainLayout->addWidget(heroCard);

    QFrame* navSection = new QFrame(panel);
    navSection->setObjectName("sidebarNavSection");
    auto* navLayout = new QVBoxLayout(navSection);
    navLayout->setContentsMargins(16, 14, 16, 14);
    navLayout->setSpacing(8);

    QLabel* navLabel = new QLabel(QString::fromUtf8("工作区视图"));
    navLabel->setObjectName("sidebarSectionLabel");
    navLayout->addWidget(navLabel);

    auto createNavButton = [this](const QString& text, int index) {
        QPushButton* btn = new QPushButton(text);
        btn->setObjectName("sidebarNavButton");
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QPushButton::clicked, this, [this, index]() { onTabClicked(index); });
        return btn;
    };

    m_sessionTabBtn = createNavButton(QString::fromUtf8("消息"), 0);
    m_sessionTabBtn->setChecked(true);
    navLayout->addWidget(m_sessionTabBtn);

    m_logTabBtn = createNavButton(QString::fromUtf8("日志"), 1);
    navLayout->addWidget(m_logTabBtn);

    mainLayout->addWidget(navSection);

    QFrame* configSection = new QFrame(panel);
    configSection->setObjectName("sidebarConfigSection");
    auto* configLayout = new QVBoxLayout(configSection);
    configLayout->setContentsMargins(16, 14, 16, 14);
    configLayout->setSpacing(8);

    QLabel* configLabel = new QLabel(QString::fromUtf8("快捷控制"));
    configLabel->setObjectName("sidebarConfigLabel");
    configLayout->addWidget(configLabel);

    auto createConfigItem = [this](const QString& text, const std::function<void()>& callback) {
        QPushButton* btn = new QPushButton(text);
        btn->setObjectName("sidebarConfigItem");
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QPushButton::clicked, this, callback);
        return btn;
    };

    configLayout->addWidget(createConfigItem(QString::fromUtf8("设置"), [this]() { onSettings(); }));
    configLayout->addWidget(createConfigItem(QString::fromUtf8("记忆面板"), [this]() { onMemoryDashboard(); }));
    configLayout->addWidget(createConfigItem(QString::fromUtf8("内存管理"), [this]() { onOpenMemoryManager(); }));
    configLayout->addWidget(createConfigItem(QString::fromUtf8("清空当前会话"), [this]() { onClearSession(); }));

    mainLayout->addWidget(configSection);

    QFrame* controlCard = new QFrame(panel);
    controlCard->setObjectName("sidebarControlCard");
    auto* controlLayout = new QVBoxLayout(controlCard);
    controlLayout->setContentsMargins(16, 14, 16, 14);
    controlLayout->setSpacing(8);

    QLabel* controlLabel = new QLabel(QString::fromUtf8("状态"));
    controlLabel->setObjectName("sidebarConfigLabel");
    controlLayout->addWidget(controlLabel);

    QLabel* controlHint = new QLabel(QString::fromUtf8("当前工作区用于聊天与日志查看。"));
    controlHint->setObjectName("sidebarMultiAgentHint");
    controlHint->setWordWrap(true);
    controlLayout->addWidget(controlHint);

    mainLayout->addWidget(controlCard);

    QFrame* footerSection = new QFrame(panel);
    footerSection->setObjectName("sidebarFooter");
    auto* footerLayout = new QVBoxLayout(footerSection);
    footerLayout->setContentsMargins(16, 14, 16, 16);
    footerLayout->setSpacing(6);

    QLabel* versionLabel = new QLabel(QString::fromUtf8("Desktop v1.27.1 · Preview"));
    versionLabel->setObjectName("sidebarVersionLabel");
    footerLayout->addWidget(versionLabel);

    QLabel* backendLabel = new QLabel(QString::fromUtf8("Backend v1.27.1"));
    backendLabel->setObjectName("sidebarBackendLabel");
    footerLayout->addWidget(backendLabel);

    mainLayout->addWidget(footerSection);

    m_sessionPanel = new SessionPanel(panel);
    m_sessionPanel->setObjectName("sessionPanelCard");
    mainLayout->insertWidget(2, m_sessionPanel, 1);

    return panel;
}

QWidget* MainWindow::createRightPanel() {
    QWidget* panel = new QWidget();
    panel->setObjectName("chatSurfacePanel");
    QVBoxLayout* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(14);

    QFrame* contentHeader = new QFrame(panel);
    contentHeader->setObjectName("contentHeader");
    auto* headerLayout = new QHBoxLayout(contentHeader);
    headerLayout->setContentsMargins(20, 16, 20, 16);
    headerLayout->setSpacing(12);

    QWidget* titleBlock = new QWidget(contentHeader);
    auto* titleLayout = new QVBoxLayout(titleBlock);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(3);

    m_windowTitleLabel = new QLabel(QString::fromUtf8("消息工作区"));
    m_windowTitleLabel->setObjectName("contentTitleLabel");
    titleLayout->addWidget(m_windowTitleLabel);

    m_windowSubtitleLabel = new QLabel(QString::fromUtf8("左侧控制会话与配置，右侧呈现聊天和日志。"));
    m_windowSubtitleLabel->setObjectName("contentSubtitleLabel");
    titleLayout->addWidget(m_windowSubtitleLabel);

    headerLayout->addWidget(titleBlock);
    headerLayout->addStretch();

    m_tokenLabel = new QLabel(QString::fromUtf8("本次消耗：0 Token"));
    m_tokenLabel->setObjectName("sidebarTokenLabel");
    headerLayout->addWidget(m_tokenLabel);
    
    m_contextIndicator = new QLabel(QString::fromUtf8("🧠 上下文：0/4000 (0%)"));
    m_contextIndicator->setObjectName("contextIndicatorLabel");
    m_contextIndicator->setToolTip(QString::fromUtf8("当前对话上下文使用情况"));
    headerLayout->addWidget(m_contextIndicator);

    m_connectionLabel = new QLabel(QString::fromUtf8("未连接"));
    m_connectionLabel->setObjectName("connectionStatusLabel");
    m_connectionLabel->setProperty("connected", false);
    headerLayout->addWidget(m_connectionLabel);

    m_operationLabel = new QLabel(QString::fromUtf8("工具待命"));
    m_operationLabel->setObjectName("operationStatusLabel");
    headerLayout->addWidget(m_operationLabel);

    QPushButton* refreshBtn = new QPushButton(QString::fromUtf8("⟳ 刷新"));
    refreshBtn->setObjectName("contentRefreshBtn");
    refreshBtn->setCursor(Qt::PointingHandCursor);
    headerLayout->addWidget(refreshBtn);

    layout->addWidget(contentHeader);

    QFrame* contentBody = new QFrame(panel);
    contentBody->setObjectName("contentBody");
    auto* bodyLayout = new QHBoxLayout(contentBody);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(14);

    QFrame* chatArea = new QFrame(contentBody);
    chatArea->setObjectName("chatAreaPanel");
    auto* chatLayout = new QVBoxLayout(chatArea);
    chatLayout->setContentsMargins(0, 0, 0, 0);
    chatLayout->setSpacing(0);

    m_leftStack = new QStackedWidget(chatArea);
    m_leftStack->setObjectName("workspaceStack");

    m_chatView = new ChatView(chatArea);
    m_leftStack->addWidget(m_chatView);

    m_logView = new LogView(chatArea);
    m_leftStack->addWidget(m_logView);

    chatLayout->addWidget(m_leftStack, 1);

    bodyLayout->addWidget(chatArea, 1);
    layout->addWidget(contentBody, 1);

    return panel;
}

void MainWindow::setupServices() {
    const QString workspaceRoot = detectWorkspaceRoot();

    m_agentService = new AgentService(this);
    m_providerManager = new ProviderManager(this);
    m_memoryManager = new MemoryManager(workspaceRoot, this);
    m_externalPlatformManager = new ExternalPlatformManager(this);
    auto* templateLoader = new TemplateLoader(workspaceRoot, workspaceRoot, this);
    
    auto& config = ConfigManager::instance();
    auto providerConfig = config.getProviderConfig();
    
    m_providerManager->setConfig(providerConfig);
    
    m_agentService->setProvider(m_providerManager);
    m_agentService->setProviderConfig(providerConfig);
    m_agentService->setMemory(m_memoryManager);
    m_agentService->setToolRegistry(&ToolRegistry::instance());
    m_agentService->setPermissionManager(&PermissionManager::instance());
    m_agentService->setTemplateLoader(templateLoader);
    m_agentService->setWorkspaceRoot(workspaceRoot);
    
    m_sessionManager->setAgentService(m_agentService);
    m_sessionManager->setMemorySystem(m_memoryManager);

    // Load External platforms
    QString tgToken = config.getTelegramToken();
    if (!tgToken.isEmpty()) {
        m_externalPlatformManager->startTelegramBot(tgToken);
    }
    QString fsAppId = config.getFeishuAppId();
    QString fsAppSecret = config.getFeishuAppSecret();
    QString fsVerificationToken = config.getFeishuVerificationToken();
    if (!fsAppId.isEmpty() && !fsAppSecret.isEmpty() && !fsVerificationToken.isEmpty()) {
        m_externalPlatformManager->startFeishuBot(fsAppId, fsAppSecret, fsVerificationToken, config.getFeishuPort());
    }
}

void MainWindow::setupConnections() {
    connect(m_sessionPanel, &SessionPanel::sessionCreated, this,
        static_cast<void (MainWindow::*)(const QString&)>(&MainWindow::onSessionCreated));
    connect(m_sessionPanel, &SessionPanel::sessionSelected, this, &MainWindow::onSessionSelected);
    connect(m_sessionPanel, &SessionPanel::sessionRenamed, this, &MainWindow::onSessionRenamed);
    connect(m_sessionPanel, &SessionPanel::sessionDeleted, this, &MainWindow::onSessionDeleted);
    connect(m_sessionPanel, &SessionPanel::sessionCopied, this, &MainWindow::onSessionCopied);
    connect(m_sessionPanel, &SessionPanel::sessionImportRequested, this, &MainWindow::onSessionImportRequested);
    connect(m_sessionPanel, &SessionPanel::sessionPinned, this, &MainWindow::onSessionPinned);
    connect(m_sessionPanel, &SessionPanel::sessionExported, this, &MainWindow::onSessionExported);
    
    connect(m_sessionManager, &SessionManager::sessionCreated, this, &MainWindow::onSessionManagerCreated);
    connect(m_sessionManager, &SessionManager::sessionSwitched, this, &MainWindow::onSessionManagerSwitched);
    connect(m_sessionManager, &SessionManager::sessionDeleted, this, &MainWindow::onSessionManagerDeleted);

    // 诊断/兜底：窗口激活时强制恢复聊天输入框可交互，避免焦点链异常导致“无光标无法输入”。
    connect(qApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
        if (state == Qt::ApplicationActive && m_chatView) {
            m_chatView->forceInputInteractive();
        }
    });
    
    if (m_agentService && m_agentService->orchestrator()) {
        connect(m_chatView, &ChatView::messageSubmitted, m_agentService->orchestrator(),
                &AgentOrchestrator::sendUserInput, Qt::QueuedConnection);
        connect(m_chatView, &ChatView::apiTestRequested, m_agentService->orchestrator(),
                [this]() { m_agentService->orchestrator()->sendUserInput(QStringLiteral("请回复：API连通测试成功。")); },
                Qt::QueuedConnection);
        connect(m_chatView, &ChatView::stopRequested, m_agentService->orchestrator(),
                &AgentOrchestrator::stopActiveRun);
        connect(m_chatView, &ChatView::quickLoadFilesRequested, this, [this]() {
            const QString fileName = QFileDialog::getOpenFileName(
                this,
                QStringLiteral("选择要加载的文件"),
                QDir::currentPath(),
                QStringLiteral("所有文件 (*.*)"));
            if (!fileName.isEmpty() && m_chatView) {
                m_chatView->appendSystemNotice(QStringLiteral("已选择文件：%1").arg(fileName));
            }
        });
        connect(m_chatView, &ChatView::quickClearContextRequested, this, &MainWindow::onClearSession);
        connect(m_chatView, &ChatView::quickCompressContextRequested, this, [this]() {
            if (!m_memoryManager) {
                return;
            }
            const bool compressed = m_memoryManager->forceCompress();
            const ProviderConfig providerConfig = ConfigManager::instance().getProviderConfig();
            updateContextIndicator(m_memoryManager->getTokenCount(), modelctx::inferModelContextLimit(providerConfig));
            if (m_chatView) {
                m_chatView->appendSystemNotice(compressed
                    ? QStringLiteral("已执行手动上下文压缩。")
                    : QStringLiteral("当前上下文较短，暂无可压缩内容。"));
            }
        });
        connect(m_chatView, &ChatView::quickOpenMemoryRequested, this, &MainWindow::onOpenMemoryManager);
        connect(m_chatView, &ChatView::quickExportSessionRequested, this, [this]() {
            if (!m_sessionManager || !m_sessionManager->hasCurrentSession()) {
                return;
            }
            const QString sessionId = m_sessionManager->currentSessionId();
            const QString fileName = QFileDialog::getSaveFileName(
                this,
                QStringLiteral("导出会话"),
                QStringLiteral("session_export.json"),
                QStringLiteral("JSON 文件 (*.json);;Markdown 文件 (*.md);;文本文件 (*.txt)"));
            if (!fileName.isEmpty()) {
                onSessionExported(sessionId, fileName);
            }
        });
        connect(m_agentService->orchestrator(), &AgentOrchestrator::runStarted, this, [this](const QString& requestId) {
            if (m_logView) {
                m_logView->addLogEntry(
                    LogLevel::Info,
                    QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")),
                    QStringLiteral("Run started: %1").arg(requestId));
            }
        });
        connect(m_agentService->orchestrator(), &AgentOrchestrator::runFinished, this, [this](const QString& requestId, const QString& status) {
            if (m_logView) {
                m_logView->addLogEntry(
                    status == QStringLiteral("error") ? LogLevel::Warning : LogLevel::Info,
                    QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")),
                    QStringLiteral("Run finished: %1 (%2)").arg(requestId, status));
            }
        });
        connect(m_agentService->orchestrator(), &AgentOrchestrator::inputRejected, this, [this](const QString& reason) {
            if (m_chatView) {
                m_chatView->showError(reason);
            }
            if (m_statusBar) {
                m_statusBar->showMessage(reason, 4000);
            }
        });
    } else {
        connect(m_chatView, &ChatView::messageSubmitted, m_agentService, &AgentService::sendMessage, Qt::QueuedConnection);
        connect(m_chatView, &ChatView::apiTestRequested, m_agentService,
                [this]() { m_agentService->sendMessage(QStringLiteral("请回复：API连通测试成功。")); },
                Qt::QueuedConnection);
        connect(m_chatView, &ChatView::stopRequested, m_agentService, &AgentService::stopGeneration);
        connect(m_chatView, &ChatView::quickLoadFilesRequested, this, [this]() {
            const QString fileName = QFileDialog::getOpenFileName(
                this,
                QStringLiteral("选择要加载的文件"),
                QDir::currentPath(),
                QStringLiteral("所有文件 (*.*)"));
            if (!fileName.isEmpty() && m_chatView) {
                m_chatView->appendSystemNotice(QStringLiteral("已选择文件：%1").arg(fileName));
            }
        });
        connect(m_chatView, &ChatView::quickClearContextRequested, this, &MainWindow::onClearSession);
        connect(m_chatView, &ChatView::quickCompressContextRequested, this, [this]() {
            if (!m_memoryManager) {
                return;
            }
            const bool compressed = m_memoryManager->forceCompress();
            const ProviderConfig providerConfig = ConfigManager::instance().getProviderConfig();
            updateContextIndicator(m_memoryManager->getTokenCount(), modelctx::inferModelContextLimit(providerConfig));
            if (m_chatView) {
                m_chatView->appendSystemNotice(compressed
                    ? QStringLiteral("已执行手动上下文压缩。")
                    : QStringLiteral("当前上下文较短，暂无可压缩内容。"));
            }
        });
        connect(m_chatView, &ChatView::quickOpenMemoryRequested, this, &MainWindow::onOpenMemoryManager);
        connect(m_chatView, &ChatView::quickExportSessionRequested, this, [this]() {
            if (!m_sessionManager || !m_sessionManager->hasCurrentSession()) {
                return;
            }
            const QString sessionId = m_sessionManager->currentSessionId();
            const QString fileName = QFileDialog::getSaveFileName(
                this,
                QStringLiteral("导出会话"),
                QStringLiteral("session_export.json"),
                QStringLiteral("JSON 文件 (*.json);;Markdown 文件 (*.md);;文本文件 (*.txt)"));
            if (!fileName.isEmpty()) {
                onSessionExported(sessionId, fileName);
            }
        });
    }

    connect(m_agentService, &AgentService::conversationUpdatedRaw, m_sessionManager, &SessionManager::updateMessages);
    
    connect(m_agentService, &AgentService::responseChunk, this, [this](const StreamChunk& chunk) {
        if (m_chatView) {
            m_chatView->appendResponseChunk(chunk);
        }
    });

    connect(m_agentService, &AgentService::toolCallRequested, this, [this](const ToolCall& toolCall) {
        if (m_operationLabel) {
            m_operationLabel->setText(QStringLiteral("工具执行：%1").arg(toolCall.name));
        }
        if (m_logView) {
            m_logView->addLogEntry(
                LogLevel::Info,
                QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")),
                QStringLiteral("Tool started: %1").arg(toolCall.name));
        }
        if (m_statusBar) {
            m_statusBar->showMessage(QStringLiteral("正在执行工具：%1").arg(toolCall.name), 4000);
        }
    });

    connect(m_agentService, &AgentService::toolCallCompleted, this, [this](const ToolCall& call, const ToolResult& result) {
        const QString toolName = call.name;
        const bool truncated = result.metadata.value(QStringLiteral("output_truncated")).toBool();
        const QString cachePath = result.metadata.value(QStringLiteral("output_cache_path")).toString();
        if (m_operationLabel) {
            m_operationLabel->setText(result.success
                ? QStringLiteral("工具完成：%1").arg(toolName)
                : QStringLiteral("工具失败：%1").arg(toolName));
        }
        if (m_chatView) {
            if (!result.success) {
                QString message = result.content.trimmed();
                if (message.size() > 280) {
                    message = message.left(280) + QStringLiteral("...");
                }
                m_chatView->appendSystemNotice(QStringLiteral("工具 %1 失败：%2").arg(toolName, message));
            } else if (truncated) {
                m_chatView->appendSystemNotice(QStringLiteral("工具输出过长，已截断并缓存到：%1").arg(cachePath));
            }
            const int riskLevel = result.metadata.value(QStringLiteral("risk_level")).toInt(-1);
            const QString riskSummary = result.metadata.value(QStringLiteral("risk_summary")).toString();
            if (!result.success && riskLevel >= 0 && !riskSummary.isEmpty()) {
                m_chatView->appendSystemNotice(
                    QStringLiteral("Shell 风险评估（级别 %1）：%2").arg(riskLevel).arg(riskSummary));
            }
        }
        if (m_logView) {
            m_logView->addLogEntry(
                result.success ? LogLevel::Info : LogLevel::Warning,
                QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")),
                result.success
                    ? QStringLiteral("Tool completed: %1").arg(toolName)
                    : QStringLiteral("Tool failed: %1").arg(toolName));
        }
        if (m_statusBar) {
            m_statusBar->showMessage(result.success
                ? QStringLiteral("工具执行完成：%1").arg(toolName)
                : QStringLiteral("工具执行失败：%1").arg(toolName), 4000);
            if (truncated && !cachePath.isEmpty()) {
                m_statusBar->showMessage(QStringLiteral("工具输出已截断，完整内容：%1").arg(cachePath), 7000);
            }
        }
    });
    
    connect(m_agentService, &AgentService::responseComplete, this, [this](const QString& fullResponse) {
        if (m_chatView) {
            m_chatView->finishStreaming();
        }
        if (m_operationLabel) {
            m_operationLabel->setText(QString::fromUtf8("空闲"));
        }
        
        // Forward response to external platform if applicable
        QString currentSessionId = m_sessionManager->currentSessionId();
        if (!currentSessionId.isEmpty() && m_externalPlatformManager) {
            auto ctxIt = m_externalConversationContexts.constFind(currentSessionId);
            if (ctxIt != m_externalConversationContexts.constEnd()) {
                const ExternalConversationContext& ctx = ctxIt.value();
                if (ctx.platform == QStringLiteral("Telegram")) {
                    m_externalPlatformManager->sendTelegramMessage(ctx.userId, fullResponse);
                } else if (ctx.platform == QStringLiteral("Feishu")) {
                    if (!ctx.messageId.isEmpty()) {
                        m_externalPlatformManager->sendFeishuReply(ctx.messageId, fullResponse);
                    } else {
                        m_externalPlatformManager->sendFeishuMessage(ctx.userId, fullResponse);
                    }
                }
            }
        }
    });
    
    connect(m_agentService, &AgentService::usageReport, this, &MainWindow::updateTokenCount);
    connect(m_agentService, &AgentService::toolCallParsed, this, &MainWindow::onToolCallParsed);
    connect(m_agentService, &AgentService::toolPreExecutionStarted, this, &MainWindow::onToolPreExecutionStarted);
    connect(m_agentService, &AgentService::toolPreExecutionCompleted, this, &MainWindow::onToolPreExecutionCompleted);
    
    connect(m_agentService, &AgentService::errorOccurred, this, [this](const QString& error) {
        if (m_chatView) {
            m_chatView->showError(error);
        }
        if (m_operationLabel) {
            m_operationLabel->setText(QString::fromUtf8("错误"));
        }
        if (m_logView) {
            m_logView->addLogEntry(
                LogLevel::Error,
                QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")),
                error);
        }
        if (m_statusBar) {
            m_statusBar->showMessage(error, 5000);
        }
    });

    if (m_agentService && m_agentService->toolExecutor()) {
        connect(m_agentService->toolExecutor(), &ToolExecutor::permissionRequest, this,
            [this](const ToolCall& toolCall, std::function<void(bool)> callback) {
                QString riskDescription;
                const int riskLevel = toolCall.arguments.value(QStringLiteral("_risk_level")).toInt(-1);
                const QString riskSummary = toolCall.arguments.value(QStringLiteral("_risk_summary")).toString();
                if (toolCall.name.contains("shell", Qt::CaseInsensitive)) {
                    riskDescription = "Shell 命令可能启动进程或修改文件。";
                } else if (toolCall.name.contains("write", Qt::CaseInsensitive) || toolCall.name.contains("edit", Qt::CaseInsensitive)) {
                    riskDescription = "文件写入操作可能修改工作区内容。";
                }
                if (riskLevel >= 0 && !riskSummary.isEmpty()) {
                    riskDescription += QStringLiteral("\n风险等级：%1\n%2").arg(riskLevel).arg(riskSummary);
                }

                PermissionDialog dialog(toolCall, riskDescription, this);
                const auto result = static_cast<PermissionDialog::Result>(dialog.exec());
                if (result == PermissionDialog::AlwaysAllow) {
                    PermissionManager::instance().addToWhitelist(
                        m_sessionManager ? m_sessionManager->currentSessionId() : QString(),
                        QString("%1:%2").arg(toolCall.name, toolCall.path));
                    callback(true);
                    return;
                }

                callback(result == PermissionDialog::Allowed);
            });
    }

    connect(m_externalPlatformManager, &ExternalPlatformManager::externalMessageReceived,
            this, [this](const QString& platform, const QString& userId, const QString& content, const QString& messageId) {
        QString sessionName = QString("%1: %2").arg(platform, userId);
        QString sessionId;
        QList<Session> sessions = m_sessionManager->listSessions();
        for (const auto& s : sessions) {
            if (s.name == sessionName) {
                sessionId = s.id;
                break;
            }
        }
        
        if (sessionId.isEmpty()) {
            sessionId = m_sessionManager->createSession(sessionName);
        }

        m_externalConversationContexts[sessionId] = {platform, userId, messageId};
        
        if (m_sessionManager->currentSessionId() != sessionId) {
            m_sessionManager->switchSession(sessionId);
        }
        
        if (m_agentService && m_agentService->orchestrator()) {
            m_agentService->orchestrator()->sendUserInput(content);
        } else {
            m_agentService->sendMessage(content);
        }
    });
}

void MainWindow::refreshSessionList() {
    QList<Session> sessions = m_sessionManager->listSessions();
    m_sessionPanel->setSessions(sessions);
}

void MainWindow::onSidebarSliderChanged(int value) {
    updateSidebarState(value);
}

void MainWindow::updateSidebarState(int value) {
    QWidget* leftWidget = m_splitter->widget(0);
    if (leftWidget) {
        leftWidget->setVisible(value == 1);
        if (m_sidebarLabel) {
            m_sidebarLabel->setText(value == 1 ? "导航已展开" : "导航已收起");
        }
    }
}

void MainWindow::onTabClicked(int index) {
    if (m_leftStack) {
        const int safeIndex = qBound(0, index, m_leftStack->count() - 1);
        m_leftStack->setCurrentIndex(safeIndex);
    }
    
    if (m_sessionTabBtn) {
        m_sessionTabBtn->setChecked(index == 0);
    }
    if (m_logTabBtn) {
        m_logTabBtn->setChecked(index == 1);
    }

    if (m_windowTitleLabel && m_windowSubtitleLabel) {
        m_windowTitleLabel->setText(index == 0 ? QString::fromUtf8("消息工作区") : QString::fromUtf8("运行日志"));
        m_windowSubtitleLabel->setText(index == 0
            ? QString::fromUtf8("会话列表、聊天记录与配置集中显示。")
            : QString::fromUtf8("自动刷新日志、过滤级别和搜索关键字。"));
    }
    
    if (index == 1 && m_logView) {
        m_logView->refreshLogs();
    } else if (index == 0 && m_chatView) {
        m_chatView->forceInputInteractive();
    }
}

void MainWindow::onSessionCreated() {
    m_sessionManager->createSession("新聊天");
}

void MainWindow::onSessionCreated(const QString& name) {
    m_sessionManager->createSession(name);
}

void MainWindow::onSessionSelected(const QString& sessionId) {
    m_sessionManager->switchSession(sessionId);
}

void MainWindow::onSessionRenamed(const QString& sessionId, const QString& name) {
    m_sessionManager->renameSession(sessionId, name);
}

void MainWindow::onSessionDeleted(const QString& sessionId) {
    m_sessionManager->deleteSession(sessionId);
    m_externalConversationContexts.remove(sessionId);
    refreshSessionList();
}

void MainWindow::onSessionCopied(const QString& sessionId) {
    if (!m_sessionManager) {
        return;
    }

    const QString duplicatedSessionId = m_sessionManager->duplicateSession(sessionId);
    if (!duplicatedSessionId.isEmpty()) {
        m_sessionManager->switchSession(duplicatedSessionId);
    }

    refreshSessionList();
}

void MainWindow::onSessionImportRequested() {
    if (!m_sessionManager) return;
    
    QString fileName = QFileDialog::getOpenFileName(
        this, "导入会话", "", "JSON 文件 (*.json)");
        
    if (fileName.isEmpty()) return;
    
    QString errorMessage;
    QString importedId = m_sessionManager->importSession(fileName, &errorMessage);
    
    if (!importedId.isEmpty()) {
        refreshSessionList();
        m_sessionManager->switchSession(importedId);
        QMessageBox::information(this, "成功", "会话导入成功。");
    } else {
        QMessageBox::warning(this, "导入失败", errorMessage.isEmpty() ? "无法导入会话。" : errorMessage);
    }
}

void MainWindow::onSessionPinned(const QString& sessionId, bool pinned) {
    if (m_sessionManager) {
        m_sessionManager->setPinned(sessionId, pinned);
        refreshSessionList();
    }
}

void MainWindow::onSessionExported(const QString& sessionId, const QString& fileName) {
    QString errorMessage;
    if (m_sessionManager && m_sessionManager->exportSession(sessionId, fileName, &errorMessage)) {
        QMessageBox::information(this, "已导出", QString("会话已导出到 %1").arg(fileName));
        return;
    }

    QMessageBox::warning(this, "导出失败", errorMessage.isEmpty() ? "无法导出会话。" : errorMessage);
}

void MainWindow::onSessionManagerCreated(const QString& id, const QString& name) {
    Q_UNUSED(name);
    refreshSessionList();
    m_sessionManager->switchSession(id);
}

void MainWindow::onSessionManagerSwitched(const QString& id) {
    m_sessionPanel->setCurrentSession(id);
    
    Session session = m_sessionManager->currentSession();
    if (m_chatView && !session.id.isEmpty()) {
        const MessageList displayMessages = m_agentService
            ? m_agentService->displayConversation(session.messages)
            : session.messages;
        m_chatView->loadMessages(displayMessages);
        m_chatView->forceInputInteractive();
    }
    if (m_operationLabel) {
        m_operationLabel->setText(QString::fromUtf8("工具待命"));
    }
}

void MainWindow::onSessionManagerDeleted(const QString& id) {
    m_externalConversationContexts.remove(id);
    if (m_sessionPanel && m_sessionPanel->currentSessionId() == id) {
        m_sessionPanel->clearSelection();
    }

    if (m_chatView && m_sessionManager->currentSessionId().isEmpty()) {
        m_chatView->clearMessages();
    }

    refreshSessionList();
}

void MainWindow::onSettings() {
    SettingsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        auto& config = ConfigManager::instance();
        auto providerConfig = config.getProviderConfig();
        m_providerManager->setConfig(providerConfig);
        m_agentService->setProviderConfig(providerConfig);
        if (m_chatView) {
            m_chatView->reloadProvider();
        }

        // Restart external platforms if configured
        m_externalPlatformManager->stopTelegramBot();
        m_externalPlatformManager->stopFeishuBot();
        
        QString tgToken = config.getTelegramToken();
        if (!tgToken.isEmpty()) {
            m_externalPlatformManager->startTelegramBot(tgToken);
        }
        QString fsAppId = config.getFeishuAppId();
        QString fsAppSecret = config.getFeishuAppSecret();
        QString fsVerificationToken = config.getFeishuVerificationToken();
        if (!fsAppId.isEmpty() && !fsAppSecret.isEmpty() && !fsVerificationToken.isEmpty()) {
            m_externalPlatformManager->startFeishuBot(fsAppId, fsAppSecret, fsVerificationToken, config.getFeishuPort());
        }
    }
}

void MainWindow::onMemoryDashboard() {
    if (!m_memoryManager) return;
    MemoryDashboardDialog dialog(m_memoryManager, this);
    dialog.exec();
}

void MainWindow::onAbout() {
    QMessageBox::about(this, "关于 Claw++",
        "<h2 style='color: #1976d2;'>Claw++</h2>"
        "<p style='font-size: 14px;'>版本 1.0.0</p>"
        "<hr><p>基于 Qt/C++ 的桌面 AI Agent 应用</p>"
        "<p>支持 OpenAI 兼容的 API 提供商。</p>");
}

void MainWindow::onClearSession() {
    if (m_agentService) {
        if (m_agentService && m_agentService->orchestrator()) {
            m_agentService->orchestrator()->stopActiveRun();
        } else {
            m_agentService->stopGeneration();
        }
    }

    // 同时清空 UI 和当前会话持久化，避免切换后历史“回弹”。
    if (m_chatView) {
        m_chatView->clearMessages();
    }

    if (m_sessionManager && m_sessionManager->hasCurrentSession()) {
        m_sessionManager->updateMessages(MessageList());
    }

    if (m_operationLabel) {
        m_operationLabel->setText(QString::fromUtf8("工具待命"));
    }
}

void MainWindow::onOpenMemoryManager() {
    MemoryDashboardDialog dialog(m_memoryManager, this);
    dialog.exec();
}

void MainWindow::updateTokenCount(int prompt, int completion, int total) {
    m_tokenLabel->setText(QString("本次消耗：%1 Token").arg(total));
    const ProviderConfig providerConfig = ConfigManager::instance().getProviderConfig();
    const int limit = modelctx::inferModelContextLimit(providerConfig);
    updateContextIndicator(prompt > 0 ? prompt : total, limit);
}

void MainWindow::updateContextIndicator(int tokenCount, int limit) {
    if (!m_contextIndicator) return;
    const int threshold = qMax(1, static_cast<int>(limit * 0.6));
    double percentage = (threshold > 0) ? (static_cast<double>(tokenCount) / threshold * 100.0) : 0.0;
    QString color = percentage > 90 ? "red" : (percentage > 70 ? "orange" : "green");
    
    m_contextIndicator->setText(QString::fromUtf8("🧠 上下文：%1/%2 (%3%)")
        .arg(tokenCount)
        .arg(threshold)
        .arg(percentage, 0, 'f', 1));
    
    m_contextIndicator->setStyleSheet(QString("color: %1; font-weight: bold;").arg(color));
    m_contextIndicator->setToolTip(QString::fromUtf8("当前上下文占用压缩阈值的 %1%\n压缩阈值 = 模型最大上下文 %2 × 60%").arg(percentage, 0, 'f', 1).arg(limit));
}

void MainWindow::updateConnectionStatus(bool connected) {
    m_connectionLabel->setText(connected ? "已连接" : "未连接");
    m_connectionLabel->setProperty("connected", connected);
    m_connectionLabel->style()->unpolish(m_connectionLabel);
    m_connectionLabel->style()->polish(m_connectionLabel);
    m_connectionLabel->update();
    if (!connected && m_operationLabel) {
        m_operationLabel->setText(QString::fromUtf8("离线"));
    } else if (connected && m_operationLabel && m_operationLabel->text() == QStringLiteral("离线")) {
        m_operationLabel->setText(QString::fromUtf8("工具待命"));
    }
}

void MainWindow::onToolCallParsed(const ToolCall& toolCall) {
    Q_UNUSED(toolCall);
    if (m_statusBar) {
        m_statusBar->showMessage(QStringLiteral("已解析工具调用"), 1200);
    }
}

void MainWindow::onToolPreExecutionStarted(const QString& toolId) {
    Q_UNUSED(toolId);
}

void MainWindow::onToolPreExecutionCompleted(const QString& toolId) {
    Q_UNUSED(toolId);
}

}
