#include "message_bubble.h"
#include "loading_indicator.h"
#include <QColor>
#include <QGraphicsDropShadowEffect>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QSizePolicy>
#include <QGraphicsOpacityEffect>
#include <QEasingCurve>
#include <QtGlobal>

namespace clawpp {

MessageBubble::MessageBubble(MessageRole role, QWidget* parent)
    : QFrame(parent)
    , m_role(role)
    , m_avatarLabel(nullptr)
    , m_contentView(nullptr)
    , m_loadingIndicator(nullptr)
    , m_bubbleFrame(nullptr)
    , m_renderTimer(new QTimer(this)) {
    m_renderTimer->setSingleShot(true);
    m_renderTimer->setInterval(16); // smoother incremental rendering (~60fps max)
    connect(m_renderTimer, &QTimer::timeout, this, [this]() {
        renderContent();
        adjustSize();
    });
    setupUi();
}

void MessageBubble::setContent(const QString& content) {
    hideLoading();
    m_rawContent = content;
    renderContent();
    adjustSize();
}

void MessageBubble::appendContent(const QString& text) {
    hideLoading();
    m_rawContent += text;
    if (!m_renderTimer->isActive()) {
        m_renderTimer->start();
    }
}

QString MessageBubble::content() const {
    return m_rawContent;
}

MessageRole MessageBubble::role() const {
    return m_role;
}

void MessageBubble::animateEntrance() {
    if (!m_fadeAnimation || !m_opacityEffect) {
        return;
    }

    m_fadeAnimation->stop();
    m_opacityEffect->setOpacity(0.0);
    m_fadeAnimation->setStartValue(0.0);
    m_fadeAnimation->setEndValue(1.0);
    m_fadeAnimation->start();
}

void MessageBubble::showLoading() {
    if (m_role == MessageRole::Assistant && m_loadingIndicator) {
        m_contentView->hide();
        m_loadingIndicator->show();
        m_loadingIndicator->start();
        adjustSize();
    }
}

void MessageBubble::hideLoading() {
    if (m_loadingIndicator) {
        m_loadingIndicator->stop();
        m_loadingIndicator->hide();
        m_contentView->show();
        adjustSize();
    }
}

bool MessageBubble::isLoading() const {
    return m_loadingIndicator && m_loadingIndicator->isRunning();
}

void MessageBubble::setupUi() {
    setFrameStyle(QFrame::NoFrame);
    setAttribute(Qt::WA_StyledBackground, true);

    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(8, 2, 8, 2);
    mainLayout->setSpacing(10);

    m_avatarLabel = new QLabel(this);
    m_avatarLabel->setObjectName("messageAvatarLabel");
    m_avatarLabel->setAlignment(Qt::AlignCenter);
    m_avatarLabel->setFixedSize(36, 36);
    const bool isSystem = (m_role == MessageRole::System || m_role == MessageRole::Tool);
    m_avatarLabel->setProperty("role", m_role == MessageRole::User ? "user" : (isSystem ? "system" : "assistant"));
    m_avatarLabel->setText(m_role == MessageRole::User ? QString::fromUtf8("我") : (isSystem ? QString::fromUtf8("SYS") : QString::fromUtf8("AI")));

    m_bubbleFrame = new QFrame();
    if (m_role == MessageRole::User) {
        m_bubbleFrame->setObjectName("userMessageBubble");
    } else if (m_role == MessageRole::System || m_role == MessageRole::Tool) {
        m_bubbleFrame->setObjectName("systemMessageBubble");
    } else {
        m_bubbleFrame->setObjectName("assistantMessageBubble");
    }
    m_bubbleFrame->setMinimumWidth(120);
    m_bubbleFrame->setMaximumWidth(820);
    m_bubbleFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

    auto* shadow = new QGraphicsDropShadowEffect(m_bubbleFrame);
    shadow->setBlurRadius(22);
    shadow->setOffset(0, 3);
    shadow->setColor(QColor(15, 23, 42, 36));
    m_bubbleFrame->setGraphicsEffect(shadow);

    QHBoxLayout* bubbleLayout = new QHBoxLayout(m_bubbleFrame);
    bubbleLayout->setContentsMargins(18, 12, 18, 12);
    bubbleLayout->setSpacing(8);

    m_contentView = new AdaptiveTextBrowser(m_bubbleFrame);
    m_contentView->setObjectName("messageContentView");
    m_contentView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    bubbleLayout->addWidget(m_contentView, 1);

    if (m_role == MessageRole::Assistant) {
        m_loadingIndicator = new LoadingIndicator(m_bubbleFrame);
        bubbleLayout->addWidget(m_loadingIndicator);
        m_loadingIndicator->hide();
    }

    m_opacityEffect = new QGraphicsOpacityEffect(this);
    m_opacityEffect->setOpacity(1.0);
    setGraphicsEffect(m_opacityEffect);

    m_fadeAnimation = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    m_fadeAnimation->setDuration(220);
    m_fadeAnimation->setEasingCurve(QEasingCurve::OutCubic);

    if (m_role != MessageRole::User) {
        mainLayout->addWidget(m_avatarLabel);
        mainLayout->addWidget(m_bubbleFrame);
        mainLayout->addStretch();
    } else {
        mainLayout->addStretch();
        mainLayout->addWidget(m_bubbleFrame);
        mainLayout->addWidget(m_avatarLabel);
    }

    updateBubbleWidth();
}

void MessageBubble::updateBubbleWidth() {
    if (!m_bubbleFrame) {
        return;
    }

    const int availableWidth = qMax(360, width());
    int bubbleWidth = 0;
    if (m_role == MessageRole::User) {
        bubbleWidth = qBound(180, static_cast<int>(availableWidth * 0.42), 420);
    } else {
        bubbleWidth = qBound(240, static_cast<int>(availableWidth * 0.62), 560);
    }
    m_bubbleFrame->setMaximumWidth(bubbleWidth);
}

void MessageBubble::resizeEvent(QResizeEvent* event) {
    QFrame::resizeEvent(event);
    updateBubbleWidth();
}

void MessageBubble::renderContent() {
    if (!m_contentView) {
        return;
    }

    if (m_role == MessageRole::Assistant || m_role == MessageRole::System || m_role == MessageRole::Tool) {
        m_contentView->setMarkdown(m_rawContent);
    } else {
        m_contentView->setPlainText(m_rawContent);
    }

    m_contentView->adjustHeightToDocument();
    adjustSize();
}

}
