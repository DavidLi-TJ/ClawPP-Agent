#ifndef CLAWPP_UI_SESSION_PANEL_H
#define CLAWPP_UI_SESSION_PANEL_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>
#include "common/types.h"

namespace clawpp {

class SessionListWidget : public QListWidget {
    Q_OBJECT

public:
    explicit SessionListWidget(QWidget* parent = nullptr);

signals:
    void renameRequested(const QString& sessionId);
    void copyRequested(const QString& sessionId);
    void exportRequested(const QString& sessionId);
    void pinRequested(const QString& sessionId, bool pinned);
    void deleteRequested(const QString& sessionId);

private:
    void showContextMenu(const QPoint& pos);
};

class SessionPanel : public QWidget {
    Q_OBJECT

public:
    explicit SessionPanel(QWidget* parent = nullptr);
    
    void setSessions(const QList<Session>& sessions);
    void setCurrentSession(const QString& sessionId);
    void clearSelection();
    QString currentSessionId() const;

signals:
    void sessionCreated(const QString& name);
    void sessionSelected(const QString& sessionId);
    void sessionRenamed(const QString& sessionId, const QString& name);
    void sessionDeleted(const QString& sessionId);
    void sessionCopied(const QString& sessionId);
    void sessionImportRequested();
    void sessionExported(const QString& sessionId, const QString& fileName);
    void sessionPinned(const QString& sessionId, bool pinned);

private:
    void setupUi();
    void onNewSession();
    void onSessionClicked(QListWidgetItem* item);
    void onRenameSession(const QString& sessionId);
    void onCopySession(const QString& sessionId);
    void onExportSession(const QString& sessionId);
    void onPinSession(const QString& sessionId, bool pinned);
    void onDeleteSession(const QString& sessionId);
    void onSearchChanged(const QString& text);
    
    SessionListWidget* m_sessionList;
    QLineEdit* m_searchEdit;
    QPushButton* m_newButton;
    QString m_currentSessionId;
};

}

#endif
