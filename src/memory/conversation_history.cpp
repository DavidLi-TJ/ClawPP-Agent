#include "conversation_history.h"

namespace clawpp {

ConversationHistory::ConversationHistory(QObject* parent)
    : QObject(parent) {}

void ConversationHistory::append(const Message& message) {
    m_messages.append(message);
    m_totalTokens += estimateTokens(message.content);
}

void ConversationHistory::setMessages(const MessageList& messages) {
    m_messages = messages;
    recalculateTokens();
}

MessageList ConversationHistory::getLast(int count) const {
    if (count <= 0 || count >= m_messages.size()) {
        return m_messages;
    }
    return m_messages.mid(m_messages.size() - count);
}

MessageList ConversationHistory::getAll() const {
    return m_messages;
}

void ConversationHistory::clear() {
    m_messages.clear();
    m_totalTokens = 0;
}

int ConversationHistory::count() const {
    return m_messages.size();
}

int ConversationHistory::totalTokens() const {
    return m_totalTokens;
}

void ConversationHistory::compressMiddle(int keepFirst, int keepLast, double ratio) {
    if (m_messages.size() <= keepFirst + keepLast) return;

    const double safeRatio = qBound(0.1, ratio, 1.0);
    
    MessageList compressed;
    for (int i = 0; i < keepFirst; ++i) {
        compressed.append(m_messages[i]);
    }
    
    QString summary = generateSummary(keepFirst, m_messages.size() - keepLast, safeRatio);
    Message summaryMsg(MessageRole::System, 
        QString("[Previous conversation summary: %1]").arg(summary));
    compressed.append(summaryMsg);
    
    for (int i = m_messages.size() - keepLast; i < m_messages.size(); ++i) {
        compressed.append(m_messages[i]);
    }
    
    m_messages = compressed;
    recalculateTokens();
}

int ConversationHistory::estimateTokens(const QString& text) const {
    int hanChars = 0;
    int latinOrDigitChars = 0;
    int otherVisibleChars = 0;
    for (const QChar& c : text) {
        if (c.script() == QChar::Script_Han) {
            ++hanChars;
        } else if (c.isLetterOrNumber()) {
            ++latinOrDigitChars;
        } else if (!c.isSpace() && !c.isPunct()) {
            ++otherVisibleChars;
        }
    }
    const int latinTokens = (latinOrDigitChars + 3) / 4;
    const int otherTokens = (otherVisibleChars + 1) / 2;
    return qMax(1, hanChars + latinTokens + otherTokens);
}

void ConversationHistory::recalculateTokens() {
    m_totalTokens = 0;
    for (const auto& msg : m_messages) {
        m_totalTokens += estimateTokens(msg.content);
    }
}

QString ConversationHistory::generateSummary(int start, int end, double ratio) const {
    QString content;
    // 预估容量，减少循环拼接导致的重复扩容。
    const int estimatedCount = qMax(0, qMin(end, m_messages.size()) - qMax(0, start));
    content.reserve(estimatedCount * 32);

    for (int i = start; i < end && i < m_messages.size(); ++i) {
        content += m_messages[i].content;
        content += ' ';
    }
    const int maxSummaryLength = qBound(160, static_cast<int>(estimatedCount * 60 * ratio), 1200);
    if (content.length() > maxSummaryLength) {
        content = content.left(maxSummaryLength) + "...";
    }
    return content;
}

}
