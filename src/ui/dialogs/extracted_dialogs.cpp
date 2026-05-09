#include "extracted_dialogs.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QVBoxLayout>

namespace clawpp {

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent) {
    setupUi();
    loadFromConfig();
}

void SettingsDialog::setupUi() {
    setWindowTitle(QString::fromUtf8("设置"));
    setModal(true);
    resize(680, 520);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(20, 20, 20, 20);
    rootLayout->setSpacing(16);

    m_tabs = new QTabWidget(this);
    m_tabs->setObjectName("settingsTabWidget");
    rootLayout->addWidget(m_tabs, 1);

    QWidget* providerPage = new QWidget(m_tabs);
    auto* providerLayout = new QVBoxLayout(providerPage);
    providerLayout->setContentsMargins(4, 4, 4, 4);
    providerLayout->setSpacing(12);

    QGroupBox* providerGroup = new QGroupBox(QString::fromUtf8("模型服务"), providerPage);
    auto* providerForm = new QFormLayout(providerGroup);
    providerForm->setLabelAlignment(Qt::AlignLeft);

    m_apiKeyEdit = new QLineEdit(providerGroup);
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyEdit->setClearButtonEnabled(true);
    providerForm->addRow(QString::fromUtf8("API Key"), m_apiKeyEdit);

    m_baseUrlEdit = new QLineEdit(providerGroup);
    m_baseUrlEdit->setClearButtonEnabled(true);
    providerForm->addRow(QString::fromUtf8("Base URL"), m_baseUrlEdit);

    m_modelEdit = new QLineEdit(providerGroup);
    m_modelEdit->setClearButtonEnabled(true);
    providerForm->addRow(QString::fromUtf8("模型名称"), m_modelEdit);

    providerLayout->addWidget(providerGroup);
    providerLayout->addStretch();
    m_tabs->addTab(providerPage, QString::fromUtf8("模型"));

    QWidget* externalPage = new QWidget(m_tabs);
    auto* externalLayout = new QVBoxLayout(externalPage);
    externalLayout->setContentsMargins(4, 4, 4, 4);
    externalLayout->setSpacing(12);

    QGroupBox* externalGroup = new QGroupBox(QString::fromUtf8("外部平台"), externalPage);
    auto* externalForm = new QFormLayout(externalGroup);
    externalForm->setLabelAlignment(Qt::AlignLeft);

    m_telegramTokenEdit = new QLineEdit(externalGroup);
    m_telegramTokenEdit->setEchoMode(QLineEdit::Password);
    m_telegramTokenEdit->setClearButtonEnabled(true);
    externalForm->addRow(QString::fromUtf8("Telegram Token"), m_telegramTokenEdit);

    m_feishuAppIdEdit = new QLineEdit(externalGroup);
    m_feishuAppIdEdit->setClearButtonEnabled(true);
    externalForm->addRow(QString::fromUtf8("飞书 App ID"), m_feishuAppIdEdit);

    m_feishuAppSecretEdit = new QLineEdit(externalGroup);
    m_feishuAppSecretEdit->setEchoMode(QLineEdit::Password);
    m_feishuAppSecretEdit->setClearButtonEnabled(true);
    externalForm->addRow(QString::fromUtf8("飞书 App Secret"), m_feishuAppSecretEdit);

    m_feishuVerificationTokenEdit = new QLineEdit(externalGroup);
    m_feishuVerificationTokenEdit->setEchoMode(QLineEdit::Password);
    m_feishuVerificationTokenEdit->setClearButtonEnabled(true);
    externalForm->addRow(QString::fromUtf8("飞书 Verification Token"), m_feishuVerificationTokenEdit);

    m_feishuPortSpin = new QSpinBox(externalGroup);
    m_feishuPortSpin->setRange(1, 65535);
    m_feishuPortSpin->setValue(8080);
    externalForm->addRow(QString::fromUtf8("飞书端口"), m_feishuPortSpin);

    externalLayout->addWidget(externalGroup);
    externalLayout->addStretch();
    m_tabs->addTab(externalPage, QString::fromUtf8("外部平台"));

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
    rootLayout->addWidget(m_buttonBox);
}

void SettingsDialog::loadFromConfig() {
    const ConfigManager& config = ConfigManager::instance();
    const ProviderConfig provider = config.getProviderConfig();

    m_apiKeyEdit->setText(provider.apiKey);
    m_baseUrlEdit->setText(provider.baseUrl);
    m_modelEdit->setText(provider.model);
    m_telegramTokenEdit->setText(config.getTelegramToken());
    m_feishuAppIdEdit->setText(config.getFeishuAppId());
    m_feishuAppSecretEdit->setText(config.getFeishuAppSecret());
    m_feishuVerificationTokenEdit->setText(config.getFeishuVerificationToken());
    m_feishuPortSpin->setValue(config.getFeishuPort());
}

void SettingsDialog::saveToConfig() {
    // 只在用户确认后统一写回配置，避免表单边改边落盘。
    ConfigManager& config = ConfigManager::instance();
    config.setApiKey(m_apiKeyEdit->text().trimmed());
    config.setBaseUrl(m_baseUrlEdit->text().trimmed());
    config.setModel(m_modelEdit->text().trimmed());
    config.setTelegramToken(m_telegramTokenEdit->text().trimmed());
    config.setFeishuAppId(m_feishuAppIdEdit->text().trimmed());
    config.setFeishuAppSecret(m_feishuAppSecretEdit->text().trimmed());
    config.setFeishuVerificationToken(m_feishuVerificationTokenEdit->text().trimmed());
    config.setFeishuPort(m_feishuPortSpin->value());
}

void SettingsDialog::accept() {
    saveToConfig();
    QDialog::accept();
}

} // namespace clawpp
