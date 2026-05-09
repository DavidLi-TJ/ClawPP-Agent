#ifndef CLAWPP_MEMORY_I_MEMORY_SYSTEM_H
#define CLAWPP_MEMORY_I_MEMORY_SYSTEM_H

#include <QObject>
#include <QString>
#include "common/types.h"

namespace clawpp {

/**
 * @class IMemorySystem
 * @brief 内存系统接口类
 * 
 * 定义内存管理系统的基本接口
 * 负责管理对话历史、token 计数和内存压缩
 */
class IMemorySystem : public QObject {
    Q_OBJECT

public:
    explicit IMemorySystem(QObject* parent = nullptr)
        : QObject(parent) {}
    
    virtual ~IMemorySystem() = default;
    
    /**
     * @brief 添加消息到内存
     * @param message 要添加的消息
     */
    virtual void addMessage(const Message& message) = 0;

    /**
     * @brief 设置当前会话上下文
     * @param sessionId 会话 ID
     * @param sessionName 会话名称
     */
    virtual void setSessionContext(const QString& sessionId, const QString& sessionName = QString()) = 0;

    /**
     * @brief 替换当前会话消息上下文
     * @param messages 完整消息列表
     */
    virtual void setContext(const MessageList& messages) = 0;
    
    /**
     * @brief 获取最近的消息
     * @param count 消息数量
     * @return 消息列表
     */
    virtual MessageList getRecentMessages(int count) const = 0;
    
    /**
     * @brief 获取所有消息
     * @return 所有消息
     */
    virtual MessageList getAllMessages() const = 0;
    
    /**
     * @brief 获取上下文消息
     * @return 上下文消息列表
     */
    virtual MessageList getContext() const = 0;
    
    /**
     * @brief 清空内存
     */
    virtual void clear() = 0;
    
    /**
     * @brief 获取 token 数量
     * @return token 总数
     */
    virtual int getTokenCount() const = 0;
    
    /**
     * @brief 检查是否需要压缩
     * @return 是否需要压缩
     */
    virtual bool needsCompression() const = 0;
    
    /**
     * @brief 压缩内存
     */
    virtual void compress() = 0;

    /**
     * @brief 基于语义相似度检索长期记忆
     * @param query 查询文本
     * @param topK 返回条数
     * @return 相关记忆片段（按相似度降序）
     */
    virtual QStringList queryRelevantMemory(const QString& query, int topK = 6) const = 0;
};

}

#endif
