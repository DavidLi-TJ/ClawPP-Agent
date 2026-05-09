#ifndef CLAWPP_UI_WIDGETS_LOADING_INDICATOR_H
#define CLAWPP_UI_WIDGETS_LOADING_INDICATOR_H

#include <QWidget>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QTimer>
#include <QLabel>
#include <QHBoxLayout>
#include <QStringList>

namespace clawpp {

class LoadingIndicator : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

public:
    explicit LoadingIndicator(QWidget* parent = nullptr);
    
    void start();
    void stop();
    bool isRunning() const;
    
    qreal opacity() const;
    void setOpacity(qreal value);

private:
    void setupUi();
    void updateAnimation();

    QLabel* m_textLabel;
    QLabel* m_spinnerLabel;
    QGraphicsOpacityEffect* m_opacityEffect;
    QPropertyAnimation* m_animation;
    QTimer* m_frameTimer;
    int m_frameIndex;
    bool m_running;
};

}

#endif
