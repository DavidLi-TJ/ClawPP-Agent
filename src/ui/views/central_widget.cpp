#include "central_widget.h"
#include "ui/chat_view.h"
#include "log_view.h"

namespace clawpp {

CentralWidget::CentralWidget(QWidget* parent)
    : QWidget(parent)
    , m_tabWidget(nullptr)
    , m_chatView(nullptr)
    , m_logView(nullptr) {
    setupUi();
}

void CentralWidget::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    m_tabWidget = new QTabWidget();
    m_tabWidget->setObjectName("mainTabWidget");
    
    m_chatView = new ChatView(this);
    m_tabWidget->addTab(m_chatView, "聊天");
    
    m_logView = new LogView(this);
    m_tabWidget->addTab(m_logView, "日志");
    
    mainLayout->addWidget(m_tabWidget);
}

ChatView* CentralWidget::chatView() const {
    return m_chatView;
}

LogView* CentralWidget::logView() const {
    return m_logView;
}

}
