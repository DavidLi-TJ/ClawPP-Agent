#include "loading_indicator.h"
#include <QEasingCurve>
#include <QStringList>

namespace clawpp {

LoadingIndicator::LoadingIndicator(QWidget* parent)
    : QWidget(parent)
    , m_textLabel(nullptr)
    , m_spinnerLabel(nullptr)
    , m_opacityEffect(nullptr)
    , m_animation(nullptr)
    , m_frameTimer(nullptr)
    , m_frameIndex(0)
    , m_running(false) {
    setupUi();
}

void LoadingIndicator::setupUi() {
    setFixedHeight(40);
    setObjectName("loadingIndicator");
    
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 8, 16, 8);
    layout->setSpacing(8);
    
    m_textLabel = new QLabel("AI 正在生成");
    m_textLabel->setObjectName("loadingIndicatorText");
    layout->addWidget(m_textLabel);
    
    m_spinnerLabel = new QLabel(QString::fromUtf8("◐"));
    m_spinnerLabel->setObjectName("loadingIndicatorSpinner");
    layout->addWidget(m_spinnerLabel);
    
    layout->addStretch();
    
    m_opacityEffect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(m_opacityEffect);
    
    m_animation = new QPropertyAnimation(this, "opacity", this);
    m_animation->setDuration(800);
    m_animation->setStartValue(0.4);
    m_animation->setEndValue(1.0);
    m_animation->setLoopCount(-1);
    m_animation->setEasingCurve(QEasingCurve::InOutSine);
    
    m_frameTimer = new QTimer(this);
    connect(m_frameTimer, &QTimer::timeout, this, &LoadingIndicator::updateAnimation);
    
    hide();
}

void LoadingIndicator::start() {
    if (m_running) return;
    
    m_running = true;
    m_frameIndex = 0;
    show();
    m_animation->start();
    m_frameTimer->start(120);
}

void LoadingIndicator::stop() {
    m_running = false;
    m_animation->stop();
    m_frameTimer->stop();
    hide();
}

bool LoadingIndicator::isRunning() const {
    return m_running;
}

qreal LoadingIndicator::opacity() const {
    return m_opacityEffect ? m_opacityEffect->opacity() : 1.0;
}

void LoadingIndicator::setOpacity(qreal value) {
    if (m_opacityEffect) {
        m_opacityEffect->setOpacity(value);
    }
}

void LoadingIndicator::updateAnimation() {
    // 用四帧字符模拟一个轻量的“转圈”状态。
    static const QStringList frames = {
        QString::fromUtf8("◐"),
        QString::fromUtf8("◓"),
        QString::fromUtf8("◑"),
        QString::fromUtf8("◒")
    };

    m_frameIndex = (m_frameIndex + 1) % frames.size();
    m_spinnerLabel->setText(frames[m_frameIndex]);
}

}
