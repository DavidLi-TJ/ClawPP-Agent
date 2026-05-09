#ifndef CLAWPP_PROVIDER_ANTHROPIC_PROVIDER_H
#define CLAWPP_PROVIDER_ANTHROPIC_PROVIDER_H

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QTimer>
#include <functional>

#include "i_llm_provider.h"

namespace clawpp {

class AnthropicProvider : public ILLMProvider {
    Q_OBJECT

public:
    explicit AnthropicProvider(const ProviderConfig& config, QObject* parent = nullptr);

    QString name() const override;
    bool isAvailable() const override;
    QString lastError() const override;

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
    QString buildUrl(const QString& endpoint) const;
    QJsonObject buildRequestBody(const MessageList& messages,
                                 const ChatOptions& options,
                                 bool stream) const;
    QJsonArray buildMessages(const MessageList& messages) const;
    QString parseResponseText(const QByteArray& response) const;
    QString parseErrorResponse(const QByteArray& response, const QString& fallback) const;
    void consumeSseEvents(const QByteArray& data, const StreamCallback& onChunk);
    void clearActiveRequest();

private:
    QNetworkAccessManager* m_networkManager;
    ProviderConfig m_config;
    QString m_lastError;

    QPointer<QNetworkReply> m_activeReply;
    QPointer<QTimer> m_timeoutTimer;

    QByteArray m_streamBuffer;
    int m_inputTokens;
    int m_outputTokens;
    quint64 m_requestGeneration;
};

} // namespace clawpp

#endif
