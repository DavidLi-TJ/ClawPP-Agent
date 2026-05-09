#ifndef CLAWPP_PROVIDER_PROVIDER_MANAGER_H
#define CLAWPP_PROVIDER_PROVIDER_MANAGER_H

#include <QObject>
#include <QMap>
#include <QQueue>
#include <QString>
#include <QHash>
#include "i_llm_provider.h"
#include "openai_provider.h"
#include "anthropic_provider.h"
#include "gemini_provider.h"

namespace clawpp {

/**
 * @class ProviderManager
 * @brief 提供者管理器
 * 
 * 单例模式管理所有 LLM 提供者
 * 提供统一的聊天接口
 */
class ProviderManager : public ILLMProvider {
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     * @return 提供者管理器实例
     */
    explicit ProviderManager(QObject* parent = nullptr);
    static ProviderManager& instance();

    QString name() const override;
    bool isAvailable() const override;
    QString lastError() const override;
    bool supportsStreaming() const override;
    bool supportsTools() const override;
    void abort() override;
    
    /// 更新 provider 配置，并重建默认 provider。
    void setConfig(const ProviderConfig& config);
    /// 初始化 provider 列表（当前默认注册 openai）。
    void initialize(const ProviderConfig& config);
    /// 注册或替换一个 provider。
    void registerProvider(const QString& providerId, ILLMProvider* provider);
    /// 设定当前 provider 名称。
    bool setDefaultProvider(const QString& providerId);
    bool hasProvider(const QString& providerId) const;
    QString currentProviderId() const;
    QStringList providerIds() const;                  ///< 列出已注册 provider
    ILLMProvider* getProvider(const QString& providerId = QString()) const;  ///< 获取提供者
    ILLMProvider* getDefault() const;                 ///< 获取当前 provider
    QStringList listAvailable() const;                ///< 列出可用提供者
    /// 指定 provider 执行聊天（不改变当前 provider）。
    void chatWithProvider(const QString& providerId,
                          const MessageList& messages,
                          const ChatOptions& options,
                          std::function<void(const QString&)> onSuccess,
                          std::function<void(const QString&)> onError);
    /// 指定 provider 执行流式聊天（不改变当前 provider）。
    void chatStreamWithProvider(const QString& providerId,
                                const MessageList& messages,
                                const ChatOptions& options,
                                StreamCallback onToken,
                                std::function<void()> onComplete,
                                ErrorCallback onError);
    
    /**
     * @brief 聊天（非流式）
     * @param messages 消息列表
     * @param options 聊天选项
     * @param onSuccess 成功回调
     * @param onError 错误回调
     */
    void chat(const MessageList& messages,
              const ChatOptions& options,
              std::function<void(const QString&)> onSuccess,
              std::function<void(const QString&)> onError);
    
    /**
     * @brief 聊天（流式）
     * @param messages 消息列表
     * @param options 聊天选项
     * @param onToken token 回调
     * @param onComplete 完成回调
     * @param onError 错误回调
     */
    void chatStream(const MessageList& messages,
                    const ChatOptions& options,
                    StreamCallback onToken,
                    std::function<void()> onComplete,
                    ErrorCallback onError);

private:
    struct PromptCacheEntry {
        QString response;
        qint64 expiresAtMs = 0;
    };

    bool isCacheableRequest(const MessageList& messages, const ChatOptions& options) const;
    QString buildPromptCacheKey(const QString& providerId,
                                const MessageList& messages,
                                const ChatOptions& options) const;
    QString getCachedPromptResponse(const QString& key);
    void putCachedPromptResponse(const QString& key, const QString& response);
    void pruneExpiredPromptCache();

    ProviderConfig m_config;
    QString m_lastError;
    ProviderManager(const ProviderManager&) = delete;
    ProviderManager& operator=(const ProviderManager&) = delete;

    QMap<QString, ILLMProvider*> m_providers;         ///< 提供者映射表
    QString normalizeProviderId(const QString& providerId) const;
    ILLMProvider* createProvider(const QString& providerId, const ProviderConfig& config);
    ProviderConfig configForProvider(const QString& providerId, const ProviderConfig& base) const;

    QString m_defaultProvider;                        ///< 当前提供者名称
    QHash<QString, PromptCacheEntry> m_promptCache;
    QQueue<QString> m_promptCacheOrder;
    int m_promptCacheMaxEntries = 64;
    int m_promptCacheTtlMs = 5 * 60 * 1000;
};

}

#endif
