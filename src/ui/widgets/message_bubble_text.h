
#ifndef CLAWPP_UI_WIDGETS_MESSAGE_BUBBLE_TEXT_H
#define CLAWPP_UI_WIDGETS_MESSAGE_BUBBLE_TEXT_H

#include <QFrame>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QTextBrowser>
#include <QTextEdit>
#include <QTextOption>

namespace clawpp {

class AdaptiveTextBrowser : public QTextBrowser {
    Q_OBJECT
public:
    explicit AdaptiveTextBrowser(QWidget* parent = nullptr) : QTextBrowser(parent) {
        setFrameStyle(QFrame::NoFrame);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setStyleSheet("background: transparent; border: none;");
        setLineWrapMode(QTextEdit::WidgetWidth);
        setOpenExternalLinks(true);
        setOpenLinks(true);
        document()->setDocumentMargin(4);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    QSize sizeHint() const override {
        QSizeF docSize = document()->size();
        return QSize(docSize.width(), docSize.height());
    }

    void adjustHeightToDocument() {
        document()->setTextWidth(viewport()->width());
        const int h = static_cast<int>(document()->size().height()) + 2;
        if (height() != h) {
            setFixedHeight(h);
        }
    }

protected:
    void resizeEvent(QResizeEvent* e) override {
        QTextBrowser::resizeEvent(e);
        adjustHeightToDocument();
    }
};

}

#endif

