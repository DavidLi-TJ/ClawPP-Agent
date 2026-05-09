#ifndef CLAWPP_UI_WIDGETS_MESSAGE_BUBBLE_H
#define CLAWPP_UI_WIDGETS_MESSAGE_BUBBLE_H

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QResizeEvent>
#include "message_bubble_text.h"
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include "common/types.h"
#include <QTimer>

namespace clawpp {

class LoadingIndicator;

class MessageBubble : public QFrame {
    Q_OBJECT

public:
    explicit MessageBubble(MessageRole role, QWidget* parent = nullptr);

    void setContent(const QString& content);
    void appendContent(const QString& text);
    QString content() const;
    MessageRole role() const;
    void animateEntrance();
    
    void showLoading();
    void hideLoading();
    bool isLoading() const;

private:
    void setupUi();
    void renderContent();
    void updateBubbleWidth();

protected:
    void resizeEvent(QResizeEvent* event) override;

    MessageRole m_role;
    QLabel* m_avatarLabel;
    AdaptiveTextBrowser* m_contentView;
    LoadingIndicator* m_loadingIndicator;
    QFrame* m_bubbleFrame;
    QGraphicsOpacityEffect* m_opacityEffect;
    QPropertyAnimation* m_fadeAnimation;
    QString m_rawContent;
    QTimer* m_renderTimer;
};

}

#endif
