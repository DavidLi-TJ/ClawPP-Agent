#ifndef CLAWPP_PROVIDER_OPENAI_PROVIDER_H
#define CLAWPP_PROVIDER_OPENAI_PROVIDER_H

#include "i_llm_provider.h"
#include "infrastructure/network/http_client.h"
#include "infrastructure/network/sse_parser.h"
#include "infrastructure/logging/logger.h"
#include <QJsonArray>

namespace clawpp {

/**
 * @class OpenAIProvider
 * @brief OpenAI 兼容接口实现，负责请求组装、响应解析与流式事件分发。
 */
class OpenAIProvider : public ILLMProvider {
    Q_OBJECT

public:
    explicit OpenAIProvider(const ProviderConfig& config, QObject* parent = nullptr);
    
    QString name() const override;
    bool isAvailable() const override;
    QString lastError() const override;
    bool supportsStreaming() const override;
    
    void chat(const MessageList& messages,
              const ChatOptions& options,
              std::function<void(const QString&)> onSuccess,
              std::function<void(const QString&)> onError) override;
    
    void chatStream(const MessageList& messages,
                    const ChatOptions& options,
                    StreamCallback onChunk,
                    std::function<void()> onComplete,
                    ErrorCallback onError) override;
    
    void abort() override;
    void setApiKey(const QString& apiKey);
    void setBaseUrl(const QString& baseUrl);

private:
    QJsonObject buildRequestBody(const MessageList& messages,
                                  const ChatOptions& options,
                                  bool stream);
    QString parseResponse(const QByteArray& response);
    QString parseErrorResponse(const QByteArray& response);

    HttpClient* m_httpClient;
    SseParser* m_sseParser;
    ProviderConfig m_config;
    QString m_lastError;
};

}

#endif
