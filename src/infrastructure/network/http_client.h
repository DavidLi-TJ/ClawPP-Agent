#ifndef CLAWPP_INFRASTRUCTURE_NETWORK_HTTP_CLIENT_H
#define CLAWPP_INFRASTRUCTURE_NETWORK_HTTP_CLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QPointer>
#include <QByteArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTimer>
#include <QSet>
#include <functional>
#include <memory>

namespace clawpp {

/**
 * @class HttpClient
 * @brief 统一封装 HTTP 普通请求与 SSE 流式请求。
 *
 * 关键能力：
 * - 统一超时控制
 * - 可重试状态码（429/5xx）自动退避重试
 * - 错误信息标准化
 */
class HttpClient : public QObject {
    Q_OBJECT

public:
    explicit HttpClient(QObject* parent = nullptr);
    ~HttpClient() = default;

    void setTimeout(int timeoutMs);
    void setBaseUrl(const QString& baseUrl);
    void setApiKey(const QString& apiKey);

    void post(const QString& endpoint,
              const QJsonObject& body,
              std::function<void(const QByteArray&)> onSuccess,
              std::function<void(const QString&)> onError);

    void postStream(const QString& endpoint,
                    const QJsonObject& body,
                    std::function<void(const QByteArray&)> onData,
                    std::function<void()> onComplete,
                    std::function<void(const QString&)> onError);

    void abort();

private:
    static QString parseErrorResponse(const QByteArray& data, const QString& defaultError);
    static int httpStatusCode(QNetworkReply* reply);
    static bool isRetryableStatusCode(int statusCode);
    static int retryDelayMs(QNetworkReply* reply, int attempt);
    static QString buildHttpErrorMessage(int statusCode, const QString& detail);

    void postImpl(const QString& endpoint,
                  const QJsonObject& body,
                  int attempt,
                quint64 requestGeneration,
                  std::function<void(const QByteArray&)> onSuccess,
                  std::function<void(const QString&)> onError);

    void postStreamImpl(const QString& endpoint,
                        const QJsonObject& body,
                        int attempt,
                    quint64 requestGeneration,
                        std::function<void(const QByteArray&)> onData,
                        std::function<void()> onComplete,
                        std::function<void(const QString&)> onError);

private:
    QNetworkAccessManager* m_networkManager;
    QString m_baseUrl;
    QString m_apiKey;
    int m_timeoutMs;
    quint64 m_requestGeneration;
    QSet<QNetworkReply*> m_activeReplies;
};

}

#endif
