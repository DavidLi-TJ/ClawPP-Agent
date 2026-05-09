#ifndef CLAWPP_UI_DIALOGS_EXTRACTED_DIALOGS_H
#define CLAWPP_UI_DIALOGS_EXTRACTED_DIALOGS_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QTabWidget>
#include <QLabel>
#include <QFormLayout>
#include <QDialogButtonBox>

#include "infrastructure/config/config_manager.h"

namespace clawpp {

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);

protected:
    void accept() override;

private:
    void setupUi();
    void loadFromConfig();
    void saveToConfig();

    QTabWidget* m_tabs = nullptr;
    QLineEdit* m_apiKeyEdit = nullptr;
    QLineEdit* m_baseUrlEdit = nullptr;
    QLineEdit* m_modelEdit = nullptr;
    QLineEdit* m_telegramTokenEdit = nullptr;
    QLineEdit* m_feishuAppIdEdit = nullptr;
    QLineEdit* m_feishuAppSecretEdit = nullptr;
    QLineEdit* m_feishuVerificationTokenEdit = nullptr;
    QSpinBox* m_feishuPortSpin = nullptr;
    QDialogButtonBox* m_buttonBox = nullptr;
};

} // namespace clawpp

#endif
