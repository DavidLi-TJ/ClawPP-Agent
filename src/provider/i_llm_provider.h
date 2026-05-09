#ifndef CLAWPP_PROVIDER_I_LLM_PROVIDER_H
#define CLAWPP_PROVIDER_I_LLM_PROVIDER_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <functional>
#include "common/types.h"

namespace clawpp {

/**
 * @class ILLMProvider
 * @brief LLM 提供者接口类
 * 
 * 定义大语言模型提供者的基本接口
 * 支持流式和非流式聊天
 */
class ILLMProvider : public QObject {
    Q_OBJECT

public:
    explicit ILLMProvider(QObject* parent = nullptr)
        : QObject(parent) {}
    
    virtual ~ILLMProvider() = default;
    
    /**
     * @brief 获取提供者名称
     * @return 提供者名称
     */
    virtual QString name() const = 0;
    
    /**
     * @brief 检查是否可用
     * @return 是否可用
     */
    virtual bool isAvailable() const = 0;
    
    /**
     * @brief 获取最后错误信息
     * @return 错误信息
     */
    virtual QString lastError() const = 0;

    /**
     * @brief 是否支持流式输出
     * @return 是否支持
     */
    virtual bool supportsStreaming() const { return true; }

    /**
     * @brief 是否支持工具调用
     * @return 是否支持
     */
    virtual bool supportsTools() const { return true; }
    
    /**
     * @brief 聊天（非流式）
     * @param messages 消息列表
     * @param options 聊天选项
     * @param onSuccess 成功回调
     * @param onError 错误回调
     */
    virtual void chat(const MessageList& messages,
                      const ChatOptions& options,
                      std::function<void(const QString&)> onSuccess,
                      std::function<void(const QString&)> onError) = 0;
    
    /**
     * @brief 聊天（流式）
     * @param messages 消息列表
     * @param options 聊天选项
     * @param onToken token 回调
     * @param onComplete 完成回调
     * @param onError 错误回调
     */
    virtual void chatStream(const MessageList& messages,
                            const ChatOptions& options,
                            StreamCallback onToken,
                            std::function<void()> onComplete,
                            ErrorCallback onError) = 0;
    
    virtual void abort() = 0;  ///< 中止请求

signals:
    void responseReceived(const QString& content);  ///< 响应接收信号
    void streamToken(const QString& token);         ///< 流式 token 信号
    void streamComplete();                          ///< 流式完成信号
    void errorOccurred(const QString& error);       ///< 错误发生信号
};

}

#endif
