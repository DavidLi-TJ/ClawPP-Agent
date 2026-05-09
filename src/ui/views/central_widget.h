#ifndef CLAWPP_UI_VIEWS_CENTRAL_WIDGET_H
#define CLAWPP_UI_VIEWS_CENTRAL_WIDGET_H

#include <QWidget>
#include <QTabWidget>
#include <QVBoxLayout>

namespace clawpp {

class ChatView;
class LogView;

class CentralWidget : public QWidget {
    Q_OBJECT

public:
    explicit CentralWidget(QWidget* parent = nullptr);
    
    ChatView* chatView() const;
    LogView* logView() const;
    
    void showChatTab();
    void showLogTab();

private:
    void setupUi();

    QTabWidget* m_tabWidget;
    ChatView* m_chatView;
    LogView* m_logView;
};

}

#endif
