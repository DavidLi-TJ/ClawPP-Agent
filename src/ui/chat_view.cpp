#include "chat_view.h"
#include "widgets/message_bubble.h"
#include <QTextCursor>

namespace clawpp {

InputTextEdit::InputTextEdit(QWidget* parent)
    : QPlainTextEdit(parent) {
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setReadOnly(false);
    setTextInteractionFlags(Qt::TextEditorInteraction);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_InputMethodEnabled, true);
}

void InputTextEdit::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier)) {
            insertPlainText("\n");
            return;
        } else {
            emit sendRequested();
            return;
        }
    }
    QPlainTextEdit::keyPressEvent(event);
}

void InputTextEdit::mousePressEvent(QMouseEvent* event) {
    QPlainTextEdit::mousePressEvent(event);
    setFocus(Qt::MouseFocusReason);
    ensureCursorVisible();
}

void InputTextEdit::focusInEvent(QFocusEvent* event) {
    QPlainTextEdit::focusInEvent(event);
    ensureCursorVisible();
    viewport()->update();
}

ChatView::ChatView(QWidget* parent)
    : QWidget(parent)
    , m_scrollArea(nullptr)
    , m_messageContainer(nullptr)
    , m_messageLayout(nullptr)
    , m_inputEdit(nullptr)
    , m_fallbackInput(nullptr)
    , m_sendBtn(nullptr)
    , m_stopBtn(nullptr)
    , m_quickCmdBtn(nullptr)
    , m_quickCmdMenu(nullptr)
    , m_currentBubble(nullptr)
    , m_isStreaming(false)
    , m_followOutput(true)
    , m_iteration(0)
    , m_currentThinkingBubble(nullptr) {
    setObjectName("chatViewRoot");
    setupUi();
    setupQuickCommands();
}

ChatView::~ChatView() {
}

void ChatView::reloadProvider() {
    // 兼容入口，当前 provider 生命周期由 AgentService 统一管理。
}

void ChatView::loadMessages(const MessageList& messages) {
    clearMessages();
    m_messages = messages;
    
    for (const Message& msg : messages) {
        addMessageBubble(msg);
    }
    
    QTimer::singleShot(10, this, &ChatView::scrollToBottom);
}

void ChatView::clearMessages() {
    m_messages.clear();
    
    while (m_messageLayout->count() > 1) {
        QLayoutItem* item = m_messageLayout->takeAt(0);
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
    
    m_currentBubble = nullptr;
    m_currentThinkingBubble = nullptr;
    m_currentContent.clear();
    m_currentThinkingContent.clear();
    m_streamToolIdsNoticed.clear();
    m_isStreaming = false;
    m_followOutput = true;
    m_iteration = 0;
}

MessageList ChatView::messages() const {
    return m_messages;
}

void ChatView::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_scrollArea = new QScrollArea();
    m_scrollArea->setObjectName("chatScrollArea");
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_messageContainer = new QWidget();
    m_messageContainer->setObjectName("chatMessageContainer");
    m_messageLayout = new QVBoxLayout(m_messageContainer);
    m_messageLayout->setContentsMargins(18, 18, 18, 14);
    m_messageLayout->setSpacing(12);
    m_messageLayout->addStretch();

    m_scrollArea->setWidget(m_messageContainer);
    mainLayout->addWidget(m_scrollArea, 1);

    // Auto-scroll when new content arrives and resizes the container
    auto* vbar = m_scrollArea->verticalScrollBar();
    connect(vbar, &QScrollBar::rangeChanged, this, [this](int min, int max) {
        if (m_isStreaming) {
            Q_UNUSED(min);
            if (m_followOutput && !m_scrollArea->verticalScrollBar()->isSliderDown()) {
                m_scrollArea->verticalScrollBar()->setValue(max);
            }
        }
    });
    connect(vbar, &QScrollBar::sliderPressed, this, [this]() {
        m_followOutput = false;
    });
    connect(vbar, &QScrollBar::actionTriggered, this, [this](int action) {
        if (action == QAbstractSlider::SliderSingleStepSub
            || action == QAbstractSlider::SliderPageStepSub
            || action == QAbstractSlider::SliderMove) {
            m_followOutput = false;
        }
    });
    connect(vbar, &QScrollBar::valueChanged, this, [this, vbar](int value) {
        const bool nearBottom = value >= (vbar->maximum() - 12);
        if (nearBottom) {
            m_followOutput = true;
            return;
        }
        if (m_isStreaming) {
            m_followOutput = false;
        }
    });

    QWidget* inputContainer = new QWidget();
    inputContainer->setObjectName("chatInputContainer");
    QVBoxLayout* inputVLayout = new QVBoxLayout(inputContainer);
    inputVLayout->setContentsMargins(16, 12, 16, 14);
    inputVLayout->setSpacing(8);

    m_inputEdit = new InputTextEdit();
    m_inputEdit->setObjectName("chatInputEdit");
    m_inputEdit->setPlaceholderText("输入消息...");
    m_inputEdit->setMinimumHeight(72);
    m_inputEdit->setMaximumHeight(100);
    m_inputEdit->setReadOnly(false);
    m_inputEdit->setTextInteractionFlags(Qt::TextEditorInteraction);
    m_inputEdit->setFocusPolicy(Qt::StrongFocus);
    m_inputEdit->setTabChangesFocus(false);
    m_inputEdit->setFocus(Qt::OtherFocusReason);
    m_inputEdit->installEventFilter(this);
    m_inputEdit->viewport()->installEventFilter(this);
    connect(m_inputEdit, &InputTextEdit::sendRequested, this, &ChatView::onSend);

    m_fallbackInput = new QLineEdit();
    m_fallbackInput->setObjectName("chatFallbackInput");
    m_fallbackInput->setPlaceholderText("若上方输入框不可用，请在此输入并回车发送...");
    connect(m_fallbackInput, &QLineEdit::returnPressed, this, &ChatView::onSend);

    QHBoxLayout* bottomLayout = new QHBoxLayout();
    
    m_quickCmdBtn = new QToolButton();
    m_quickCmdBtn->setObjectName("chatQuickCommandButton");
    m_quickCmdBtn->setText(QString::fromUtf8("\u26A1"));
    m_quickCmdBtn->setToolTip("快捷命令");
    m_quickCmdBtn->setFixedSize(36, 36);
    m_quickCmdMenu = new QMenu(m_quickCmdBtn);
    m_quickCmdBtn->setMenu(m_quickCmdMenu);
    m_quickCmdBtn->setPopupMode(QToolButton::InstantPopup);
    bottomLayout->addWidget(m_quickCmdBtn);
    
    QLabel* hintLabel = new QLabel("回车发送，Ctrl+回车换行");
    hintLabel->setObjectName("chatHintLabel");
    bottomLayout->addWidget(hintLabel);
    bottomLayout->addStretch();

    m_stopBtn = new QPushButton("停止");
    m_stopBtn->setObjectName("chatStopButton");
    m_stopBtn->setFixedSize(80, 36);
    m_stopBtn->setVisible(false);
    connect(m_stopBtn, &QPushButton::clicked, this, &ChatView::onStop);
    bottomLayout->addWidget(m_stopBtn);

    m_sendBtn = new QPushButton("发送");
    m_sendBtn->setObjectName("chatSendButton");
    m_sendBtn->setFixedSize(80, 36);
    connect(m_sendBtn, &QPushButton::clicked, this, &ChatView::onSend);
    bottomLayout->addWidget(m_sendBtn);

    QPushButton* testApiBtn = new QPushButton("测试 API");
    testApiBtn->setObjectName("chatTestApiButton");
    testApiBtn->setFixedSize(96, 36);
    connect(testApiBtn, &QPushButton::clicked, this, [this]() {
        appendSystemNotice(QStringLiteral("正在发送 API 测试请求..."));
        emit apiTestRequested();
    });
    bottomLayout->addWidget(testApiBtn);

    QPushButton* loadFilesBtn = new QPushButton("Load Files");
    loadFilesBtn->setObjectName("chatLoadFilesButton");
    loadFilesBtn->setFixedSize(96, 36);
    connect(loadFilesBtn, &QPushButton::clicked, this, [this]() { emit quickLoadFilesRequested(); });
    bottomLayout->addWidget(loadFilesBtn);

    QPushButton* clearContextBtn = new QPushButton("Clear Context");
    clearContextBtn->setObjectName("chatClearContextButton");
    clearContextBtn->setFixedSize(110, 36);
    connect(clearContextBtn, &QPushButton::clicked, this, [this]() { emit quickClearContextRequested(); });
    bottomLayout->addWidget(clearContextBtn);

    QPushButton* compressContextBtn = new QPushButton("Compress Context");
    compressContextBtn->setObjectName("chatCompressContextButton");
    compressContextBtn->setFixedSize(130, 36);
    connect(compressContextBtn, &QPushButton::clicked, this, [this]() { emit quickCompressContextRequested(); });
    bottomLayout->addWidget(compressContextBtn);

    QPushButton* showMemoryBtn = new QPushButton("Show Memory");
    showMemoryBtn->setObjectName("chatShowMemoryButton");
    showMemoryBtn->setFixedSize(106, 36);
    connect(showMemoryBtn, &QPushButton::clicked, this, [this]() { emit quickOpenMemoryRequested(); });
    bottomLayout->addWidget(showMemoryBtn);

    QPushButton* exportSessionBtn = new QPushButton("Export Session");
    exportSessionBtn->setObjectName("chatExportSessionButton");
    exportSessionBtn->setFixedSize(120, 36);
    connect(exportSessionBtn, &QPushButton::clicked, this, [this]() { emit quickExportSessionRequested(); });
    bottomLayout->addWidget(exportSessionBtn);

    inputVLayout->addWidget(m_inputEdit);
    inputVLayout->addWidget(m_fallbackInput);
    inputVLayout->addLayout(bottomLayout);
    mainLayout->addWidget(inputContainer);
}

bool ChatView::eventFilter(QObject* watched, QEvent* event) {
    if ((watched == m_inputEdit || (m_inputEdit && watched == m_inputEdit->viewport()))
        && (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease)) {
        m_inputEdit->setFocus(Qt::MouseFocusReason);
        QTextCursor cursor = m_inputEdit->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_inputEdit->setTextCursor(cursor);
        m_inputEdit->ensureCursorVisible();
        return false;
    }
    return QWidget::eventFilter(watched, event);
}

void ChatView::setupQuickCommands() {
    struct QuickCommand {
        QString icon;
        QString name;
        QString prompt;
    };
    
    QList<QuickCommand> commands = {
        {QString::fromUtf8("\U0001F4D6"), "解释代码", "请详细解释下面这段代码：\n\n"},
        {QString::fromUtf8("\U0001F50D"), "查找问题", "请分析下面这段代码可能存在的问题：\n\n"},
        {QString::fromUtf8("\u2728"), "优化代码", "请优化下面这段代码以提升性能：\n\n"},
        {QString::fromUtf8("\U0001F4DD"), "添加注释", "请为下面这段代码补充详细注释：\n\n"},
        {QString::fromUtf8("\U0001F9EA"), "生成测试", "请为下面这段代码生成单元测试：\n\n"},
        {QString::fromUtf8("\U0001F4DA"), "生成文档", "请为下面这段代码生成文档说明：\n\n"},
    };
    
    for (const auto& cmd : commands) {
        QAction* action = m_quickCmdMenu->addAction(cmd.icon + " " + cmd.name);
        QString prompt = cmd.prompt;
        connect(action, &QAction::triggered, this, [this, prompt]() {
            QString currentText = m_inputEdit->toPlainText();
            if (currentText.isEmpty()) {
                m_inputEdit->setPlainText(prompt);
            } else {
                m_inputEdit->setPlainText(prompt + currentText);
            }
            m_inputEdit->setFocus();
        });
    }
    
    m_quickCmdMenu->addSeparator();
    QAction* customAction = m_quickCmdMenu->addAction("自定义命令...");
    connect(customAction, &QAction::triggered, this, [this]() {
        m_inputEdit->setFocus();
    });
}

void ChatView::updateStreamingUi(bool streaming) {
    m_isStreaming = streaming;
    m_sendBtn->setVisible(!streaming);
    m_stopBtn->setVisible(streaming);
    // 保持输入框始终可编辑，避免“生成中无法输入/测试 API”的体验问题。
    m_inputEdit->setEnabled(true);
    m_inputEdit->setReadOnly(false);
    m_inputEdit->setTextInteractionFlags(Qt::TextEditorInteraction);
    if (m_fallbackInput) {
        m_fallbackInput->setEnabled(true);
    }
}

void ChatView::sendMessage(const QString& text) {
    const QString normalized = text.trimmed();
    if (normalized.isEmpty()) {
        return;
    }

    Message userMsg(MessageRole::User, normalized);
    m_messages.append(userMsg);
    addMessageBubble(userMsg);
    m_inputEdit->clear();
    
    emit messagesChanged(m_messages);
    updateStreamingUi(true);
    m_followOutput = true;
    m_iteration = 0;

    m_currentContent.clear();
    m_currentThinkingContent.clear();
    m_streamToolIdsNoticed.clear();

    Message assistantMsg(MessageRole::Assistant, QString());
    m_currentBubble = addMessageBubble(assistantMsg);
    if (m_currentBubble) {
        m_currentBubble->showLoading();
    }

    QTimer::singleShot(20, this, [this, normalized]() {
        emit messageSubmitted(normalized);
    });
}

void ChatView::onStreamChunk(const StreamChunk& chunk) {
    if (!chunk.content.isEmpty()) {
        m_currentContent += chunk.content;
        if (m_currentBubble) {
            if (m_currentBubble->isLoading()) {
                m_currentBubble->hideLoading();
            }
            m_currentBubble->appendContent(chunk.content);
            if (m_followOutput) {
                scrollToBottom();
            }
        }
    }
}

void ChatView::onStreamComplete() {
    m_currentThinkingBubble = nullptr;
    m_currentThinkingContent.clear();
    m_streamToolIdsNoticed.clear();
    if (m_currentBubble) {
        m_currentBubble->hideLoading();
    }

    if (!m_currentContent.isEmpty()) {
        Message assistantMsg(MessageRole::Assistant, m_currentContent);
        m_messages.append(assistantMsg);
        emit messagesChanged(m_messages);
    }

    updateStreamingUi(false);
    m_currentBubble = nullptr;
    m_iteration = 0;
    m_currentContent.clear();
}

void ChatView::appendResponseChunk(const StreamChunk& chunk) {
    onStreamChunk(chunk);
}

void ChatView::finishStreaming() {
    onStreamComplete();
}

void ChatView::showError(const QString& error) {
    onStreamError(error);
}

void ChatView::appendSystemNotice(const QString& text) {
    const Message notice(MessageRole::System, text);
    addMessageBubble(notice);
}

void ChatView::appendThinkingSegment(const QString& thought) {
    if (thought.trimmed().isEmpty()) {
        return;
    }
    if (!m_currentThinkingBubble) {
        Message thinkingMsg(MessageRole::System, QStringLiteral("Thinking: %1").arg(thought));
        m_currentThinkingBubble = addMessageBubble(thinkingMsg);
        m_currentThinkingContent = thought;
        return;
    }
    m_currentThinkingContent += thought;
    m_currentThinkingBubble->setContent(QStringLiteral("Thinking: %1").arg(m_currentThinkingContent));
}

void ChatView::appendActionSegment(const QString& action) {
    if (action.trimmed().isEmpty()) {
        return;
    }
    Message actionMsg(MessageRole::System, action);
    addMessageBubble(actionMsg);
}

void ChatView::appendObservationSegment(const QString& observation, bool success) {
    const QString prefix = success ? QStringLiteral("Observation(OK): ") : QStringLiteral("Observation(ERR): ");
    Message obsMsg(MessageRole::System, prefix + observation);
    addMessageBubble(obsMsg);
}

void ChatView::forceInputInteractive() {
    if (!m_inputEdit) {
        return;
    }
    m_inputEdit->setEnabled(true);
    m_inputEdit->setReadOnly(false);
    m_inputEdit->setTextInteractionFlags(Qt::TextEditorInteraction);
    m_inputEdit->setFocusPolicy(Qt::StrongFocus);
    m_inputEdit->setFocus(Qt::ActiveWindowFocusReason);
    QTextCursor cursor = m_inputEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_inputEdit->setTextCursor(cursor);
    m_inputEdit->ensureCursorVisible();
    m_inputEdit->viewport()->update();
    if (m_fallbackInput && !m_fallbackInput->isEnabled()) {
        m_fallbackInput->setEnabled(true);
    }
}

void ChatView::onStop() {
    emit stopRequested();

    if (m_currentBubble) {
        m_currentBubble->appendContent("\n\n**已停止生成**");
    }

    if (!m_currentContent.trimmed().isEmpty()) {
        Message assistantMsg(MessageRole::Assistant, m_currentContent + "\n\n[已停止生成]");
        m_messages.append(assistantMsg);
        emit messagesChanged(m_messages);
    }

    updateStreamingUi(false);
    m_currentBubble = nullptr;
    m_iteration = 0;
    m_currentContent.clear();
}

MessageBubble* ChatView::addMessageBubble(const Message& message) {
    MessageBubble* bubble = new MessageBubble(message.role);
    bubble->setContent(message.content);
    m_messageLayout->insertWidget(m_messageLayout->count() - 1, bubble);
    bubble->animateEntrance();
    QTimer::singleShot(10, this, [this]() {
        if (m_followOutput) {
            scrollToBottom();
        }
    });
    return bubble;
}

void ChatView::scrollToBottom() {
    m_followOutput = true;
    m_scrollArea->verticalScrollBar()->setValue(m_scrollArea->verticalScrollBar()->maximum());
}

void ChatView::onSend() {
    QString text;
    if (m_fallbackInput && !m_fallbackInput->text().trimmed().isEmpty()) {
        text = m_fallbackInput->text().trimmed();
        m_fallbackInput->clear();
    } else {
        text = m_inputEdit->toPlainText().trimmed();
    }
    if (text.isEmpty() || m_isStreaming) return;
    sendMessage(text);
}

void ChatView::onStreamError(const QString& error) {
    if (m_currentBubble) {
        m_currentBubble->hideLoading();
        m_currentBubble->appendContent("\n\n**错误：** " + error);
    } else {
        Message assistantMsg(MessageRole::Assistant, "错误：" + error);
        m_currentBubble = addMessageBubble(assistantMsg);
    }

    QString persistedText = m_currentContent;
    if (!persistedText.isEmpty()) {
        persistedText += "\n\n";
    }
    persistedText += QString("错误：%1").arg(error);
    Message assistantMsg(MessageRole::Assistant, persistedText);
    m_messages.append(assistantMsg);
    emit messagesChanged(m_messages);
    
    updateStreamingUi(false);
    m_currentBubble = nullptr;
    m_currentContent.clear();
}

}
