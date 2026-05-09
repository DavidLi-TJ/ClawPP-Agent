#ifndef CLAWPP_AGENT_I_AGENT_CORE_H
#define CLAWPP_AGENT_I_AGENT_CORE_H

#include <QObject>
#include <QString>
#include "common/types.h"

/**
 * @namespace clawpp
 * @brief Claw++ 项目命名空间
 */
namespace clawpp {

/**
 * @class IAgentCore
 * @brief Agent 核心接口类
 * 定义所有 Agent 实现的基本接口
 * 
 * 这是一个抽象基类，所有具体的 Agent 实现都应该继承此类
 */
class IAgentCore : public QObject {
    Q_OBJECT

public:
    explicit IAgentCore(QObject* parent = nullptr)
        : QObject(parent) {}
    
    virtual ~IAgentCore() = default;
    
    /**
     * @brief 运行 Agent
     * @param input 用户输入
     */
    virtual void run(const QString& input) = 0;
    
    /**
     * @brief 停止 Agent
     */
    virtual void stop() = 0;
    
    /**
     * @brief 设置上下文消息
     * @param messages 消息列表
     */
    virtual void setContext(const MessageList& messages) = 0;
    
    /**
     * @brief 设置系统提示词
     * @param prompt 系统提示词
     */
    virtual void setSystemPrompt(const QString& prompt) = 0;

signals:
    void responseChunk(const StreamChunk& chunk);     ///< 响应片段信号
    void toolCallRequested(const ToolCall& toolCall); ///< 工具调用请求信号
    void completed(const QString& result);            ///< 完成信号
    void errorOccurred(const QString& error);         ///< 错误发生信号
};

}

#endif
