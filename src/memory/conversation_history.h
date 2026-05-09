#ifndef CLAWPP_MEMORY_CONVERSATION_HISTORY_H
#define CLAWPP_MEMORY_CONVERSATION_HISTORY_H

#include <QObject>
#include <QList>
#include "common/types.h"

namespace clawpp {

/**
 * @class ConversationHistory
 * @brief 对话历史管理类
 * 
 * 负责存储和管理对话历史消息
 * 支持 token 计数、消息压缩等功能
 */
class ConversationHistory : public QObject {
    Q_OBJECT

public:
    explicit ConversationHistory(QObject* parent = nullptr);
    
    void append(const Message& message);              ///< 添加消息
    void setMessages(const MessageList& messages);    ///< 替换全部消息
    MessageList getLast(int count) const;             ///< 获取最后 N 条消息
    MessageList getAll() const;                       ///< 获取所有消息
    void clear();                                     ///< 清空历史
    int count() const;                                ///< 获取消息数量
    int totalTokens() const;                          ///< 获取总 token 数
    
    /**
     * @brief 压缩中间部分消息
     * @param keepFirst 保留开头的消息数
     * @param keepLast 保留结尾的消息数
     * @param ratio 摘要比例
     */
    void compressMiddle(int keepFirst, int keepLast, double ratio);

private:
    int estimateTokens(const QString& text) const;    ///< 估算 token 数量
    void recalculateTokens();                         ///< 重新计算 token
    QString generateSummary(int start, int end, double ratio) const;///< 生成摘要

    MessageList m_messages;                           ///< 消息列表
    int m_totalTokens = 0;                            ///< 总 token 数
};

}

#endif
