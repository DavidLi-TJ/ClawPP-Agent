#include "qml_models.h"

namespace clawpp {

SessionListModel::SessionListModel(QObject* parent)
    : QAbstractListModel(parent) {}

int SessionListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : m_items.size();
}

QVariant SessionListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return {};
    }

    const Item& item = m_items.at(index.row());
    switch (role) {
        case IdRole: return item.id;
        case NameRole: return item.name;
        case PinnedRole: return item.pinned;
        case StatusTextRole: return item.statusText;
        case SelectedRole: return item.selected;
        case UpdatedAtRole: return item.updatedAt;
        default: return {};
    }
}

QHash<int, QByteArray> SessionListModel::roleNames() const {
    return {
        {IdRole, "id"},
        {NameRole, "name"},
        {PinnedRole, "pinned"},
        {StatusTextRole, "statusText"},
        {SelectedRole, "selected"},
        {UpdatedAtRole, "updatedAt"},
    };
}

void SessionListModel::setSessions(const QList<Session>& sessions, const QString& currentId) {
    beginResetModel();
    m_items.clear();
    m_items.reserve(sessions.size());

    for (const Session& session : sessions) {
        Item item;
        item.id = session.id;
        item.name = session.name;
        item.pinned = session.isPinned;
        item.selected = session.id == currentId;
        item.updatedAt = session.updatedAt.toString(QStringLiteral("yyyy-MM-dd HH:mm"));
        switch (session.status) {
            case Session::Active: item.statusText = QStringLiteral("active"); break;
            case Session::Paused: item.statusText = QStringLiteral("paused"); break;
            case Session::Archived: item.statusText = QStringLiteral("archived"); break;
        }
        m_items.append(item);
    }

    endResetModel();
}

QVariantMap SessionListModel::get(int row) const {
    QVariantMap map;
    if (row < 0 || row >= m_items.size()) {
        return map;
    }

    const Item& item = m_items.at(row);
    map.insert(QStringLiteral("id"), item.id);
    map.insert(QStringLiteral("name"), item.name);
    map.insert(QStringLiteral("pinned"), item.pinned);
    map.insert(QStringLiteral("statusText"), item.statusText);
    map.insert(QStringLiteral("selected"), item.selected);
    map.insert(QStringLiteral("updatedAt"), item.updatedAt);
    return map;
}

QString SessionListModel::sessionIdAt(int row) const {
    if (row < 0 || row >= m_items.size()) {
        return {};
    }
    return m_items.at(row).id;
}

MessageListModel::MessageListModel(QObject* parent)
    : QAbstractListModel(parent) {}

int MessageListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : m_items.size();
}

QVariant MessageListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return {};
    }

    const Message& message = m_items.at(index.row()).message;
    switch (role) {
        case SenderRole: return senderText(message.role);
        case ContentRole: return message.content;
        case DisplayContentRole: return displayContent(message);
        case RoleTypeRole: return static_cast<int>(message.role);
        case IsUserRole: return message.role == MessageRole::User;
        case IsAssistantRole: return message.role == MessageRole::Assistant;
        case TimestampRole: return message.timestamp.toString(Qt::ISODate);
        case ToolCallsRole: {
            if (message.toolCalls.isEmpty()) return QVariant();
            QJsonArray arr;
            for (const auto& call : message.toolCalls) {
                QJsonObject obj;
                obj["id"] = call.id;
                obj["type"] = call.type;
                obj["name"] = call.name;
                obj["arguments"] = call.arguments;
                arr.append(obj);
            }
            return QJsonDocument(arr).toJson(QJsonDocument::Compact);
        }
        default: return {};
    }
}

QHash<int, QByteArray> MessageListModel::roleNames() const {
    return {
        {SenderRole, "sender"},
        {ContentRole, "content"},
        {DisplayContentRole, "displayContent"},
        {RoleTypeRole, "roleType"},
        {IsUserRole, "isUser"},
        {IsAssistantRole, "isAssistant"},
        {TimestampRole, "timestamp"},
        {ToolCallsRole, "toolCalls"},
    };
}

void MessageListModel::setMessages(const MessageList& messages) {
    beginResetModel();
    m_items.clear();
    m_items.reserve(messages.size());
    for (const Message& message : messages) {
        m_items.append(Item{message});
    }
    endResetModel();
}

MessageList MessageListModel::messages() const {
    MessageList messages;
    messages.reserve(m_items.size());
    for (const Item& item : m_items) {
        messages.append(item.message);
    }
    return messages;
}

QVariantMap MessageListModel::get(int row) const {
    QVariantMap map;
    if (row < 0 || row >= m_items.size()) {
        return map;
    }
    const Message& message = m_items.at(row).message;
    map.insert(QStringLiteral("sender"), senderText(message.role));
    map.insert(QStringLiteral("content"), message.content);
    map.insert(QStringLiteral("displayContent"), displayContent(message));
    map.insert(QStringLiteral("roleType"), static_cast<int>(message.role));
    map.insert(QStringLiteral("isUser"), message.role == MessageRole::User);
    map.insert(QStringLiteral("isAssistant"), message.role == MessageRole::Assistant);
    map.insert(QStringLiteral("timestamp"), message.timestamp.toString(Qt::ISODate));
    return map;
}

int MessageListModel::addMessage(int roleType, const QString& content, const QString& name) {
    const MessageRole role = static_cast<MessageRole>(roleType);
    Message message(role, content);
    message.name = name;

    const int row = m_items.size();
    beginInsertRows(QModelIndex(), row, row);
    m_items.append(Item{message});
    endInsertRows();
    return row;
}

void MessageListModel::updateMessage(int row, const QString& content) {
    if (row < 0 || row >= m_items.size()) {
        return;
    }

    m_items[row].message.content = content;
    m_items[row].message.timestamp = QDateTime::currentDateTime();
    const QModelIndex modelIndex = index(row);
    emit dataChanged(modelIndex, modelIndex, {ContentRole, DisplayContentRole, TimestampRole});
}

void MessageListModel::appendToMessage(int row, const QString& content) {
    if (row < 0 || row >= m_items.size()) {
        return;
    }

    m_items[row].message.content += content;
    const QModelIndex modelIndex = index(row);
    emit dataChanged(modelIndex, modelIndex, {ContentRole, DisplayContentRole});
}

bool MessageListModel::removeMessageAt(int row) {
    if (row < 0 || row >= m_items.size()) {
        return false;
    }

    beginRemoveRows(QModelIndex(), row, row);
    m_items.removeAt(row);
    endRemoveRows();
    return true;
}

void MessageListModel::clear() {
    beginResetModel();
    m_items.clear();
    endResetModel();
}

QString MessageListModel::senderText(MessageRole role) const {
    switch (role) {
        case MessageRole::User: return QStringLiteral("user");
        case MessageRole::Assistant: return QStringLiteral("assistant");
        case MessageRole::System: return QStringLiteral("system");
        case MessageRole::Tool: return QStringLiteral("tool");
    }
    return QStringLiteral("assistant");
}

QString MessageListModel::displayContent(const Message& message) const {
    if (message.role == MessageRole::Assistant && message.content.trimmed().isEmpty()) {
        return QStringLiteral("正在生成…");
    }
    return message.content;
}

} // namespace clawpp
