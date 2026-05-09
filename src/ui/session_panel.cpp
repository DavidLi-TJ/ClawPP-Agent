#include "session_panel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFile>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QColor>
#include <QFont>

namespace clawpp {

SessionListWidget::SessionListWidget(QWidget* parent)
    : QListWidget(parent) {
    setContextMenuPolicy(Qt::CustomContextMenu);
    setObjectName("sessionList");
    connect(this, &QListWidget::customContextMenuRequested,
            this, &SessionListWidget::showContextMenu);
}

void SessionListWidget::showContextMenu(const QPoint& pos) {
    QListWidgetItem* item = itemAt(pos);
    if (!item) return;
    
    QString sessionId = item->data(Qt::UserRole).toString();
    QString sessionName = item->text();
    bool isPinned = item->data(Qt::UserRole + 1).toBool();
    
    QMenu menu;
    
    QAction* renameAction = menu.addAction("重命名");
    QAction* copyAction = menu.addAction("复制会话");
    QAction* exportAction = menu.addAction("导出会话");
    
    menu.addSeparator();
    
    QAction* pinAction = menu.addAction(isPinned ? "取消置顶" : "置顶会话");
    pinAction->setCheckable(true);
    pinAction->setChecked(isPinned);
    
    menu.addSeparator();
    
    QAction* deleteAction = menu.addAction("删除会话");
    
    connect(renameAction, &QAction::triggered, [this, sessionId]() {
        emit renameRequested(sessionId);
    });
    connect(copyAction, &QAction::triggered, [this, sessionId]() {
        emit copyRequested(sessionId);
    });
    connect(exportAction, &QAction::triggered, [this, sessionId]() {
        emit exportRequested(sessionId);
    });
    connect(pinAction, &QAction::triggered, [this, sessionId, isPinned]() {
        emit pinRequested(sessionId, !isPinned);
    });
    connect(deleteAction, &QAction::triggered, [this, sessionId]() {
        emit deleteRequested(sessionId);
    });
    
    menu.exec(mapToGlobal(pos));
}

SessionPanel::SessionPanel(QWidget* parent)
    : QWidget(parent)
    , m_sessionList(nullptr)
    , m_searchEdit(nullptr)
    , m_newButton(nullptr)
    , m_currentSessionId() {
    setObjectName("sessionPanelBody");
    setupUi();
}

void SessionPanel::setupUi() {
    setMinimumWidth(264);
    setMaximumWidth(340);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(14);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    
    QLabel* titleLabel = new QLabel("会话");
    titleLabel->setObjectName("sidebarTitleLabel");
    mainLayout->addWidget(titleLabel);

    QLabel* subtitleLabel = new QLabel("切换、置顶、重命名并导出聊天记录");
    subtitleLabel->setObjectName("sidebarSubtitleLabel");
    subtitleLabel->setWordWrap(true);
    mainLayout->addWidget(subtitleLabel);
    
    QHBoxLayout* searchLayout = new QHBoxLayout();
    m_searchEdit = new QLineEdit();
    m_searchEdit->setObjectName("sessionSearchEdit");
    m_searchEdit->setPlaceholderText("搜索会话");
    searchLayout->addWidget(m_searchEdit);
    mainLayout->addLayout(searchLayout);
    
    m_sessionList = new SessionListWidget();
    connect(m_sessionList, &QListWidget::itemClicked, this, &SessionPanel::onSessionClicked);
    connect(m_sessionList, &SessionListWidget::renameRequested, this, &SessionPanel::onRenameSession);
    connect(m_sessionList, &SessionListWidget::copyRequested, this, &SessionPanel::onCopySession);
    connect(m_sessionList, &SessionListWidget::exportRequested, this, &SessionPanel::onExportSession);
    connect(m_sessionList, &SessionListWidget::pinRequested, this, &SessionPanel::onPinSession);
    connect(m_sessionList, &SessionListWidget::deleteRequested, this, &SessionPanel::onDeleteSession);
    mainLayout->addWidget(m_sessionList);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_newButton = new QPushButton("新建会话");
    m_newButton->setObjectName("sessionNewButton");
    connect(m_newButton, &QPushButton::clicked, this, &SessionPanel::onNewSession);
    buttonLayout->addWidget(m_newButton);
    
    QPushButton* importBtn = new QPushButton("导入");
    importBtn->setObjectName("sessionSecondaryButton");
    connect(importBtn, &QPushButton::clicked, this, [this]() { emit sessionImportRequested(); });
    buttonLayout->addWidget(importBtn);
    
    mainLayout->addLayout(buttonLayout);
    
    connect(m_searchEdit, &QLineEdit::textChanged, this, &SessionPanel::onSearchChanged);
}

    
void SessionPanel::setSessions(const QList<Session>& sessions) {
    m_sessionList->clear();
    
    for (const Session& session : sessions) {
        QString displayName = session.name;
        if (session.isPinned) {
            displayName = QString::fromUtf8("\U0001F4CC ") + displayName;
        }
        
        QListWidgetItem* item = new QListWidgetItem(displayName);
        item->setData(Qt::UserRole, session.id);
        item->setData(Qt::UserRole + 1, session.isPinned);
        
        if (session.status == Session::Archived) {
            item->setForeground(QColor("#9e9e9e"));
        }
        
        if (session.isPinned) {
            QFont font = item->font();
            font.setBold(true);
            item->setFont(font);
        }
        
        m_sessionList->addItem(item);
        
        if (session.id == m_currentSessionId) {
            m_sessionList->setCurrentItem(item);
        }
    }
}

void SessionPanel::setCurrentSession(const QString& sessionId) {
    m_currentSessionId = sessionId;
    
    for (int i = 0; i < m_sessionList->count(); ++i) {
        QListWidgetItem* item = m_sessionList->item(i);
        if (item->data(Qt::UserRole).toString() == sessionId) {
            m_sessionList->setCurrentItem(item);
            break;
        }
    }
}

void SessionPanel::clearSelection() {
    m_sessionList->clearSelection();
    m_currentSessionId.clear();
}

QString SessionPanel::currentSessionId() const {
    return m_currentSessionId;
}

void SessionPanel::onNewSession() {
    bool ok;
    QString name = QInputDialog::getText(this, "新建会话",
        "请输入会话名称：", QLineEdit::Normal, "", &ok);
    
    if (ok) {
        emit sessionCreated(name);
    }
}

void SessionPanel::onSessionClicked(QListWidgetItem* item) {
    QString sessionId = item->data(Qt::UserRole).toString();
    if (sessionId != m_currentSessionId) {
        emit sessionSelected(sessionId);
    }
}

void SessionPanel::onRenameSession(const QString& sessionId) {
    QString currentName;
    for (int i = 0; i < m_sessionList->count(); ++i) {
        QListWidgetItem* item = m_sessionList->item(i);
        if (item->data(Qt::UserRole).toString() == sessionId) {
            currentName = item->text();
            if (currentName.startsWith(QString::fromUtf8("\U0001F4CC "))) {
                currentName = currentName.mid(2);
            }
            break;
        }
    }
    
    bool ok;
    QString newName = QInputDialog::getText(this, "重命名会话",
        "请输入新名称：", QLineEdit::Normal, currentName, &ok);
    
    if (ok && !newName.isEmpty()) {
        emit sessionRenamed(sessionId, newName);
    }
}

void SessionPanel::onCopySession(const QString& sessionId) {
    emit sessionCopied(sessionId);
}

void SessionPanel::onExportSession(const QString& sessionId) {
    QString fileName = QFileDialog::getSaveFileName(
        this, "导出会话", "", "JSON 文件 (*.json);;Markdown 文件 (*.md);;文本文件 (*.txt)");
    
    if (!fileName.isEmpty()) {
        emit sessionExported(sessionId, fileName);
    }
}

void SessionPanel::onPinSession(const QString& sessionId, bool pinned) {
    emit sessionPinned(sessionId, pinned);
}

void SessionPanel::onDeleteSession(const QString& sessionId) {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "删除会话",
        "确定要删除这个会话吗？",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        emit sessionDeleted(sessionId);
    }
}

void SessionPanel::onSearchChanged(const QString& text) {
    QString lowerText = text.toLower();
    
    for (int i = 0; i < m_sessionList->count(); ++i) {
        QListWidgetItem* item = m_sessionList->item(i);
        bool matches = item->text().toLower().contains(lowerText);
        item->setHidden(!matches);
    }
}

}
