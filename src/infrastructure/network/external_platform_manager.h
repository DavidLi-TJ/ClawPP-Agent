#ifndef CLAWPP_INFRASTRUCTURE_NETWORK_EXTERNAL_PLATFORM_MANAGER_H
#define CLAWPP_INFRASTRUCTURE_NETWORK_EXTERNAL_PLATFORM_MANAGER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QHash>
#include <QTimer>
#include <QTcpServer>
#include <QJsonObject>
#include <functional>
#include <QDateTime>

class QNetworkAccessManager;
class QNetworkReply;
class QTcpSocket;

namespace clawpp {

class ExternalPlatformManager : public QObject {
    Q_OBJECT

public:
    explicit ExternalPlatformManager(QObject* parent = nullptr);
    ~ExternalPlatformManager();

    void startTelegramBot(const QString& token);
    void stopTelegramBot();
    
    void startFeishuBot(const QString& appId,
                        const QString& appSecret,
                        const QString& verificationToken,
                        int port);
    void stopFeishuBot();

    void sendTelegramMessage(const QString& userId, const QString& text);
    void sendFeishuMessage(const QString& userId, const QString& text);
    void sendFeishuReply(const QString& messageId, const QString& text);

signals:
    void externalMessageReceived(const QString& platform,
                                 const QString& userId,
                                 const QString& content,
                                 const QString& messageId);
    void statusChanged(const QString& platform, bool isRunning, const QString& error = QString());

private slots:
    void onTelegramPoll();
    void onTelegramReplyFinished(QNetworkReply* reply);
    void onFeishuNewConnection();
    void onFeishuReadyRead(QTcpSocket* client);

private:
    QNetworkAccessManager* m_networkManager;
    
    // Telegram
    QString m_tgToken;
    QTimer* m_tgPollTimer;
    qint64 m_tgLastUpdateId;
    bool m_tgIsRunning;
    
    // Feishu Webhook
    QString m_fsAppId;
    QString m_fsAppSecret;
    QString m_fsVerificationToken;
    QString m_fsTenantAccessToken;
    qint64 m_fsTokenExpireEpoch;
    bool m_fsIsRunning;
    QTcpServer* m_fsServer;
    QHash<QTcpSocket*, QByteArray> m_fsPendingBuffers;
    
    void updateFeishuTenantToken(std::function<void(bool)> callback);
    void sendFeishuMessageInternal(const QString& endpoint,
                                   const QJsonObject& payload,
                                   std::function<void(bool)> callback = nullptr);
    QByteArray buildFeishuContent(const QString& text) const;
    bool handleFeishuRequest(QTcpSocket* client, const QByteArray& requestData);
    bool validateFeishuSignature(const QMap<QString, QString>& headers, const QByteArray& body) const;
    QByteArray hmacSha256(const QByteArray& key, const QByteArray& message) const;
    static QMap<QString, QString> parseHeaders(const QByteArray& requestHead);
    static QString headerValue(const QMap<QString, QString>& headers, const QString& key);
    static QByteArray httpResponse(int statusCode, const QByteArray& body, const QByteArray& contentType = "text/plain; charset=utf-8");
    static void sendHttpResponse(QTcpSocket* client, int statusCode, const QByteArray& body, const QByteArray& contentType = "text/plain; charset=utf-8");
};

} // namespace clawpp

#endif // CLAWPP_INFRASTRUCTURE_NETWORK_EXTERNAL_PLATFORM_MANAGER_H
