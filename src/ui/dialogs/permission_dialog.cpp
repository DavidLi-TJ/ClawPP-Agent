#include "permission_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QJsonDocument>

namespace clawpp {

PermissionDialog::PermissionDialog(const ToolCall& toolCall,
                                       const QString& riskDescription,
                                       QWidget* parent)
    : QDialog(parent)
    , m_toolCall(toolCall)
    , m_timeoutSeconds(60)
    , m_remainingSeconds(60)
    , m_result(Denied) {
    setObjectName("permissionDialog");
    setupUi(riskDescription);
    startTimeout();
}

    
void PermissionDialog::setupUi(const QString& riskDescription) {
    setWindowTitle("权限请求");
    setMinimumWidth(450);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    
    QLabel* iconLabel = new QLabel("⚠️");
    iconLabel->setObjectName("permissionIconLabel");
    iconLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(iconLabel);
    
    QLabel* titleLabel = new QLabel("工具权限请求");
    titleLabel->setObjectName("permissionTitleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    
    QLabel* toolLabel = new QLabel(QString("工具：<b>%1</b>").arg(m_toolCall.name));
    toolLabel->setObjectName("permissionToolLabel");
    mainLayout->addWidget(toolLabel);
    
    QString argsText = QString::fromUtf8(
        QJsonDocument(m_toolCall.arguments).toJson(QJsonDocument::Indented));
    QLabel* argsLabel = new QLabel(QString("参数：\n%1").arg(argsText));
    argsLabel->setObjectName("permissionArgsLabel");
    argsLabel->setWordWrap(true);
    mainLayout->addWidget(argsLabel);
    
    if (!riskDescription.isEmpty()) {
        QLabel* riskLabel = new QLabel(QString("⚠️ 风险说明：%1").arg(riskDescription));
        riskLabel->setObjectName("permissionRiskLabel");
        riskLabel->setWordWrap(true);
        mainLayout->addWidget(riskLabel);
    }
    
    m_timeoutLabel = new QLabel(QString("%1 秒后自动拒绝").arg(m_timeoutSeconds));
    m_timeoutLabel->setObjectName("permissionTimeoutLabel");
    m_timeoutLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_timeoutLabel);
    
    m_timeoutBar = new QProgressBar();
    m_timeoutBar->setObjectName("permissionTimeoutBar");
    m_timeoutBar->setRange(0, m_timeoutSeconds);
    m_timeoutBar->setValue(m_timeoutSeconds);
    m_timeoutBar->setTextVisible(false);
    mainLayout->addWidget(m_timeoutBar);
    
    m_alwaysAllowCheck = new QCheckBox("始终允许此工具");
    m_alwaysAllowCheck->setObjectName("permissionAlwaysAllowCheck");
    mainLayout->addWidget(m_alwaysAllowCheck);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(12);
    
    QPushButton* denyBtn = new QPushButton("拒绝");
    denyBtn->setObjectName("denyBtn");
    connect(denyBtn, &QPushButton::clicked, this, &PermissionDialog::onDeny);
    
    QPushButton* allowBtn = new QPushButton("允许");
    allowBtn->setObjectName("allowBtn");
    allowBtn->setDefault(true);
    connect(allowBtn, &QPushButton::clicked, this, &PermissionDialog::onAllow);
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(denyBtn);
    buttonLayout->addWidget(allowBtn);
    mainLayout->addLayout(buttonLayout);
}

void PermissionDialog::startTimeout() {
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &PermissionDialog::onTimeout);
    m_timer->start(1000);
}

void PermissionDialog::onTimeout() {
    m_remainingSeconds--;
    m_timeoutLabel->setText(QString("%1 秒后自动拒绝").arg(m_remainingSeconds));
    m_timeoutBar->setValue(m_remainingSeconds);
    
    if (m_remainingSeconds <= 0) {
        onDeny();
    }
}

void PermissionDialog::onAllow() {
    m_timer->stop();
    m_result = m_alwaysAllowCheck->isChecked() ? AlwaysAllow : Allowed;
    accept();
}

void PermissionDialog::onDeny() {
    m_timer->stop();
    m_result = Denied;
    reject();
}

PermissionDialog::Result PermissionDialog::dialogResult() const {
    return m_result;
}

}
