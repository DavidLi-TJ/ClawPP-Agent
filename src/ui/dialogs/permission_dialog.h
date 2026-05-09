#ifndef CLAWPP_UI_DIALOGS_PERMISSION_DIALOG_H
#define CLAWPP_UI_DIALOGS_PERMISSION_DIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QProgressBar>
#include "common/types.h"

namespace clawpp {

class PermissionDialog : public QDialog {
    Q_OBJECT

public:
    enum Result { Denied = 0, Allowed = 1, AlwaysAllow = 2 };
    
    explicit PermissionDialog(const ToolCall& toolCall,
                               const QString& riskDescription = QString(),
                               QWidget* parent = nullptr);
    
    Result dialogResult() const;

private:
    void setupUi(const QString& riskDescription);
    void startTimeout();
    void onTimeout();
    void onAllow();
    void onDeny();
    
    ToolCall m_toolCall;
    QTimer* m_timer;
    QLabel* m_timeoutLabel;
    QProgressBar* m_timeoutBar;
    QCheckBox* m_alwaysAllowCheck;
    int m_timeoutSeconds;
    int m_remainingSeconds;
    Result m_result;
};

}

#endif
