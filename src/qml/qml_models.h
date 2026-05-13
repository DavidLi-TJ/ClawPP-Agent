#ifndef CLAWPP_QML_MODELS_H
#define CLAWPP_QML_MODELS_H

#include <QAbstractListModel>
#include <QVector>
#include "common/types.h"

namespace clawpp {

class SessionListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        PinnedRole,
        StatusTextRole,
        SelectedRole,
        UpdatedAtRole
    };

    explicit SessionListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setSessions(const QList<Session>& sessions, const QString& currentId = QString());
    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE QString sessionIdAt(int row) const;

private:
    struct Item {
        QString id;
        QString name;
        bool pinned = false;
        QString statusText;
        bool selected = false;
        QString updatedAt;
    };

    QVector<Item> m_items;
};

class MessageListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        SenderRole = Qt::UserRole + 1,
        ContentRole,
        DisplayContentRole,
        RoleTypeRole,
        IsUserRole,
        IsAssistantRole,
        TimestampRole,
        ToolCallsRole
    };

    explicit MessageListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setMessages(const MessageList& messages);
    MessageList messages() const;
    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE int addMessage(int roleType, const QString& content, const QString& name = QString());
    Q_INVOKABLE void updateMessage(int row, const QString& content);
    Q_INVOKABLE void appendToMessage(int row, const QString& content);
    void replaceMessage(int row, const Message& message);
    Q_INVOKABLE bool removeMessageAt(int row);
    Q_INVOKABLE void clear();

private:
    struct Item {
        Message message;
    };

    QString senderText(MessageRole role) const;
    QString displayContent(const Message& message) const;

    QVector<Item> m_items;
};

} // namespace clawpp

#endif
