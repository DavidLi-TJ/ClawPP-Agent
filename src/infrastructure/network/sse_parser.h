#ifndef CLAWPP_INFRASTRUCTURE_NETWORK_SSE_PARSER_H
#define CLAWPP_INFRASTRUCTURE_NETWORK_SSE_PARSER_H

#include <QObject>
#include <QByteArray>
#include "common/types.h"

namespace clawpp {

/**
 * @class SseParser
 * @brief 将 OpenAI 风格的 SSE 文本流解析为结构化 StreamChunk。
 */
class SseParser : public QObject {
    Q_OBJECT

public:
    explicit SseParser(QObject* parent = nullptr);
    
    /// 清空内部缓冲并重置完成状态。
    void reset();
    /// 解析新增字节流，返回本轮可消费的 chunk 列表。
    QList<StreamChunk> parse(const QByteArray& data);
    bool isDone() const;

signals:
    void chunkReceived(const StreamChunk& chunk);
    void done();

private:
    StreamChunk extractChunk(const QString& dataStr);
    
    QByteArray m_buffer;
    bool m_isDone;
};

}

#endif
