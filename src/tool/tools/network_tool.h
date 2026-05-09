#ifndef CLAWPP_TOOL_TOOLS_NETWORK_TOOL_H
#define CLAWPP_TOOL_TOOLS_NETWORK_TOOL_H

#include "tool/i_tool.h"
#include <QNetworkAccessManager>

namespace clawpp {

/**
 * @class NetworkTool
 * @brief 提供同步网络请求能力（GET/POST/抓取网页文本）。
 */
class NetworkTool : public ITool {
    Q_OBJECT

public:
    explicit NetworkTool(QObject* parent = nullptr);
    
    QString name() const override;
    QString description() const override;
    QJsonObject parameters() const override;
    PermissionLevel permissionLevel() const override;
    ToolResult execute(const QJsonObject& args) override;

private:
    /// 同步 GET。
    ToolResult httpGet(const QString& url, const QJsonObject& headers, int timeoutMs);
    /// 同步 POST。
    ToolResult httpPost(const QString& url, const QString& body,
                        const QJsonObject& headers, const QString& contentType,
                        int timeoutMs);
    /// 拉取网页并做基础文本清洗。
    ToolResult webFetch(const QString& url, int timeoutMs);
    
    QNetworkAccessManager* m_networkManager;
};

}

#endif
