#include "network_tool.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QRegularExpression>
#include <QUrl>

namespace {

bool validateHttpUrl(const QString& rawUrl, QString* errorMessage) {
    const QUrl url(rawUrl);
    if (!url.isValid() || url.scheme().isEmpty() || url.host().isEmpty()) {
        if (errorMessage) {
            *errorMessage = QString("Invalid URL: %1").arg(rawUrl);
        }
        return false;
    }

    const QString scheme = url.scheme().toLower();
    if (scheme != QStringLiteral("http") && scheme != QStringLiteral("https")) {
        if (errorMessage) {
            *errorMessage = QString("Unsupported URL scheme: %1").arg(url.scheme());
        }
        return false;
    }

    return true;
}

void setBrowserHeaders(QNetworkRequest* request) {
    if (!request) {
        return;
    }

    request->setHeader(
        QNetworkRequest::UserAgentHeader,
        QStringLiteral("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
                       "(KHTML, like Gecko) Chrome/124.0 Safari/537.36"));
    request->setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,text/plain;q=0.8,*/*;q=0.7");
    request->setRawHeader("Accept-Language", "zh-CN,zh;q=0.9,en;q=0.8");
    request->setRawHeader("Cache-Control", "no-cache");
}

QString cleanHtmlPreview(const QByteArray& body) {
    QString text = QString::fromUtf8(body.left(4096));
    text.remove(QRegularExpression(QStringLiteral("<script[^>]*>.*?</script>"),
                                   QRegularExpression::CaseInsensitiveOption |
                                   QRegularExpression::DotMatchesEverythingOption));
    text.remove(QRegularExpression(QStringLiteral("<style[^>]*>.*?</style>"),
                                   QRegularExpression::CaseInsensitiveOption |
                                   QRegularExpression::DotMatchesEverythingOption));
    text.remove(QRegularExpression(QStringLiteral("<[^>]+>")));
    text.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
    text = text.trimmed();
    if (text.length() > 240) {
        text = text.left(240) + QStringLiteral("...");
    }
    return text;
}

QString readerUrlFor(const QString& url) {
    return QStringLiteral("https://r.jina.ai/%1").arg(url);
}

}

namespace clawpp {

NetworkTool::NetworkTool(QObject* parent)
    : ITool(parent)
    , m_networkManager(new QNetworkAccessManager(this)) {}

QString NetworkTool::name() const {
    return "network";
}

QString NetworkTool::description() const {
    return "Network operations: http_get, http_post, web_fetch";
}

QJsonObject NetworkTool::parameters() const {
    QJsonObject params;
    params["type"] = "object";
    
    QJsonObject properties;
    
    QJsonObject operation;
    operation["type"] = "string";
    operation["enum"] = QJsonArray{"http_get", "http_post", "web_fetch"};
    operation["description"] = "The network operation to perform";
    properties["operation"] = operation;
    
    QJsonObject url;
    url["type"] = "string";
    url["description"] = "The URL for the request";
    properties["url"] = url;
    
    QJsonObject headers;
    headers["type"] = "object";
    headers["description"] = "HTTP headers (optional)";
    properties["headers"] = headers;
    
    QJsonObject body;
    body["type"] = "string";
    body["description"] = "Request body for POST (optional)";
    properties["body"] = body;
    
    QJsonObject contentType;
    contentType["type"] = "string";
    contentType["description"] = "Content-Type header (default: application/json)";
    contentType["default"] = "application/json";
    properties["content_type"] = contentType;
    
    QJsonObject timeout;
    timeout["type"] = "integer";
    timeout["description"] = "Timeout in milliseconds (default: 30000)";
    timeout["default"] = 30000;
    properties["timeout"] = timeout;
    
    params["properties"] = properties;
    params["required"] = QJsonArray{"operation", "url"};
    
    return params;
}

PermissionLevel NetworkTool::permissionLevel() const {
    return PermissionLevel::Moderate;
}

ToolResult NetworkTool::execute(const QJsonObject& args) {
    ToolResult result;
    
    QString operation = args["operation"].toString();
    QString url = args["url"].toString().trimmed();
    
    if (operation.isEmpty() || url.isEmpty()) {
        result.success = false;
        result.content = "Error: operation and url are required";
        return result;
    }
    
    int timeoutMs = qBound(1000, args["timeout"].toInt(30000), 300000);

    QString validationError;
    if (!validateHttpUrl(url, &validationError)) {
        result.success = false;
        result.content = validationError;
        return result;
    }
    
    if (operation == "http_get") {
        return httpGet(url, args["headers"].toObject(), timeoutMs);
    } else if (operation == "http_post") {
        return httpPost(url, args["body"].toString(), 
                       args["headers"].toObject(),
                       args["content_type"].toString("application/json"),
                       timeoutMs);
    } else if (operation == "web_fetch") {
        return webFetch(url, timeoutMs);
    } else {
        result.success = false;
        result.content = QString("Error: Unknown operation: %1").arg(operation);
        return result;
    }
}

ToolResult NetworkTool::httpGet(const QString& url, const QJsonObject& headers, int timeoutMs) {
    ToolResult result;
    
    QNetworkRequest request{QUrl(url)};
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, 
                        QNetworkRequest::NoLessSafeRedirectPolicy);
    
    for (auto it = headers.begin(); it != headers.end(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());
    }
    
    QNetworkReply* reply = m_networkManager->get(request);
    
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    
    timer.start(timeoutMs);
    loop.exec();

    if (!timer.isActive()) {
        reply->abort();
        result.success = false;
        result.content = QString("Request timed out after %1 ms").arg(timeoutMs);
        result.metadata["timeout"] = true;
        result.metadata["url"] = url;
        reply->deleteLater();
        return result;
    }

    timer.stop();

    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray responseBody = reply->readAll();

    if (reply->error() == QNetworkReply::NoError && statusCode >= 200 && statusCode < 400) {
        result.success = true;
        result.content = QString::fromUtf8(responseBody);
    } else {
        if (statusCode == 401 || statusCode == 403 || statusCode == 429) {
            reply->deleteLater();
            ToolResult fallback = webFetch(url, timeoutMs);
            if (fallback.success) {
                fallback.metadata["original_operation"] = QStringLiteral("http_get");
                fallback.metadata["original_status_code"] = statusCode;
                return fallback;
            }
            fallback.content = QStringLiteral("HTTP GET failed (%1); reader fallback also failed: %2")
                .arg(statusCode > 0 ? QString::number(statusCode) : QStringLiteral("no-status"),
                     fallback.content);
            fallback.metadata["original_operation"] = QStringLiteral("http_get");
            fallback.metadata["original_status_code"] = statusCode;
            return fallback;
        }

        result.success = false;
        result.content = QString("HTTP GET failed (%1): %2")
            .arg(statusCode > 0 ? QString::number(statusCode) : QStringLiteral("no-status"), reply->errorString());
        if (!responseBody.isEmpty()) {
            const QString preview = cleanHtmlPreview(responseBody);
            if (!preview.isEmpty()) {
                result.content += QStringLiteral("\nResponse preview: ") + preview;
            }
        }
    }

    result.metadata["status_code"] = statusCode;
    result.metadata["url"] = url;
    
    reply->deleteLater();
    return result;
}

ToolResult NetworkTool::httpPost(const QString& url, const QString& body,
                    const QJsonObject& headers, const QString& contentType,
                    int timeoutMs) {
    ToolResult result;
    
    QNetworkRequest request{QUrl(url)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                        QNetworkRequest::NoLessSafeRedirectPolicy);
    
    for (auto it = headers.begin(); it != headers.end(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());
    }
    
    QNetworkReply* reply = m_networkManager->post(request, body.toUtf8());
    
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    
    timer.start(timeoutMs);
    loop.exec();

    if (!timer.isActive()) {
        reply->abort();
        result.success = false;
        result.content = QString("Request timed out after %1 ms").arg(timeoutMs);
        result.metadata["timeout"] = true;
        result.metadata["url"] = url;
        reply->deleteLater();
        return result;
    }

    timer.stop();

    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray responseBody = reply->readAll();

    if (reply->error() == QNetworkReply::NoError && statusCode >= 200 && statusCode < 400) {
        result.success = true;
        result.content = QString::fromUtf8(responseBody);
    } else {
        result.success = false;
        result.content = QString("HTTP POST failed (%1): %2")
            .arg(statusCode > 0 ? QString::number(statusCode) : QStringLiteral("no-status"), reply->errorString());
        if (!responseBody.isEmpty()) {
            result.content += "\n" + QString::fromUtf8(responseBody.left(512));
        }
    }

    result.metadata["status_code"] = statusCode;
    result.metadata["url"] = url;
    
    reply->deleteLater();
    return result;
}

ToolResult NetworkTool::webFetch(const QString& url, int timeoutMs) {
    ToolResult result;
    
    QNetworkRequest request{QUrl(url)};
    setBrowserHeaders(&request);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                        QNetworkRequest::NoLessSafeRedirectPolicy);
    
    QNetworkReply* reply = m_networkManager->get(request);
    
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    
    timer.start(timeoutMs);
    loop.exec();

    if (!timer.isActive()) {
        reply->abort();
        result.success = false;
        result.content = QString("Request timed out after %1 ms").arg(timeoutMs);
        result.metadata["timeout"] = true;
        result.metadata["url"] = url;
        reply->deleteLater();
        return result;
    }

    timer.stop();

    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray responseBody = reply->readAll();

    auto parseReadableText = [](const QByteArray& body) {
            QString html = QString::fromUtf8(body);
            
            html.remove(QRegularExpression("<script[^>]*>.*?</script>", 
                        QRegularExpression::CaseInsensitiveOption | 
                        QRegularExpression::DotMatchesEverythingOption));
            html.remove(QRegularExpression("<style[^>]*>.*?</style>",
                        QRegularExpression::CaseInsensitiveOption |
                        QRegularExpression::DotMatchesEverythingOption));
            html.remove(QRegularExpression("<[^>]+>"));
            html.replace(QRegularExpression("\\s+"), " ");
            html = html.trimmed();
            
            if (html.length() > 10000) {
                html = html.left(10000) + "... [truncated]";
            }

            return html;
    };

    if (reply->error() == QNetworkReply::NoError && statusCode >= 200 && statusCode < 400) {
        const QString text = parseReadableText(responseBody);
        result.success = true;
        result.content = text;
        result.metadata["url"] = url;
        result.metadata["status_code"] = statusCode;
        result.metadata["truncated"] = text.contains(QStringLiteral("[truncated]"));
    } else {
        const bool shouldTryReader = (statusCode == 401 || statusCode == 403 || statusCode == 429);
        const QString directError = reply->errorString();
        reply->deleteLater();

        if (shouldTryReader) {
            const QString fallbackUrl = readerUrlFor(url);
            QNetworkRequest fallbackRequest{QUrl(fallbackUrl)};
            setBrowserHeaders(&fallbackRequest);
            fallbackRequest.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                                         QNetworkRequest::NoLessSafeRedirectPolicy);

            QNetworkReply* fallbackReply = m_networkManager->get(fallbackRequest);
            QEventLoop fallbackLoop;
            QTimer fallbackTimer;
            fallbackTimer.setSingleShot(true);
            connect(fallbackReply, &QNetworkReply::finished, &fallbackLoop, &QEventLoop::quit);
            connect(&fallbackTimer, &QTimer::timeout, &fallbackLoop, &QEventLoop::quit);

            fallbackTimer.start(timeoutMs);
            fallbackLoop.exec();

            if (!fallbackTimer.isActive()) {
                fallbackReply->abort();
                result.success = false;
                result.content = QStringLiteral("Web fetch failed: target site returned %1 and reader fallback timed out.")
                    .arg(statusCode);
                result.metadata["timeout"] = true;
                result.metadata["url"] = url;
                result.metadata["fallback_url"] = fallbackUrl;
                fallbackReply->deleteLater();
                return result;
            }

            fallbackTimer.stop();
            const int fallbackStatus = fallbackReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            const QByteArray fallbackBody = fallbackReply->readAll();
            if (fallbackReply->error() == QNetworkReply::NoError && fallbackStatus >= 200 && fallbackStatus < 400) {
                QString text = QString::fromUtf8(fallbackBody).trimmed();
                if (text.length() > 10000) {
                    text = text.left(10000) + QStringLiteral("... [truncated]");
                }
                result.success = true;
                result.content = text;
                result.metadata["url"] = url;
                result.metadata["fallback_url"] = fallbackUrl;
                result.metadata["status_code"] = statusCode;
                result.metadata["fallback_status_code"] = fallbackStatus;
                result.metadata["truncated"] = text.contains(QStringLiteral("[truncated]"));
                fallbackReply->deleteLater();
                return result;
            }

            const QString fallbackError = fallbackReply->errorString();
            fallbackReply->deleteLater();
            result.success = false;
            result.content = QStringLiteral("Web fetch failed: target site returned %1 (%2), reader fallback returned %3 (%4).")
                .arg(statusCode > 0 ? QString::number(statusCode) : QStringLiteral("no-status"),
                     directError,
                     fallbackStatus > 0 ? QString::number(fallbackStatus) : QStringLiteral("no-status"),
                     fallbackError);
            result.metadata["url"] = url;
            result.metadata["fallback_url"] = fallbackUrl;
            result.metadata["status_code"] = statusCode;
            result.metadata["fallback_status_code"] = fallbackStatus;
            return result;
        }

        result.success = false;
        result.content = QString("Web fetch failed (%1): %2")
            .arg(statusCode > 0 ? QString::number(statusCode) : QStringLiteral("no-status"), directError);
        const QString preview = cleanHtmlPreview(responseBody);
        if (!preview.isEmpty()) {
            result.content += QStringLiteral("\nResponse preview: ") + preview;
        }
        result.metadata["url"] = url;
        result.metadata["status_code"] = statusCode;
        return result;
    }
    
    reply->deleteLater();
    return result;
}

}
