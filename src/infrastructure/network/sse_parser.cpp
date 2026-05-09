#include "sse_parser.h"
#include <QJsonDocument>
#include <QJsonArray>

namespace clawpp {

SseParser::SseParser(QObject* parent)
    : QObject(parent)
    , m_isDone(false) {}

void SseParser::reset() {
    m_buffer.clear();
    m_isDone = false;
}

QList<StreamChunk> SseParser::parse(const QByteArray& data) {
    QList<StreamChunk> chunks;
    m_buffer.append(data);
    
    // 逐行消费缓冲区，处理增量网络包与半包场景。
    while (true) {
        int newlinePos = m_buffer.indexOf('\n');
        if (newlinePos == -1) break;
        
        QByteArray line = m_buffer.left(newlinePos);
        m_buffer.remove(0, newlinePos + 1);
        
        QString lineStr = QString::fromUtf8(line).trimmed();
        if (lineStr.isEmpty()) continue;
        
        if (lineStr.startsWith("data: ")) {
            QString dataStr = lineStr.mid(6);
            
            if (dataStr == "[DONE]") {
                m_isDone = true;
                emit done();
                break;
            }
            
            // data 行承载真实 JSON payload，提取为统一 StreamChunk。
            StreamChunk chunk = extractChunk(dataStr);
            if (!chunk.content.isEmpty() || !chunk.reasoningContent.isEmpty() 
                || !chunk.toolCalls.isEmpty() || chunk.hasUsage) {
                chunks.append(chunk);
                emit chunkReceived(chunk);
            }
        }
    }
    
    return chunks;
}

bool SseParser::isDone() const {
    return m_isDone;
}

StreamChunk SseParser::extractChunk(const QString& dataStr) {
    StreamChunk chunk;
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(dataStr.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) return chunk;
    
    QJsonObject obj = doc.object();
    QJsonArray choices = obj["choices"].toArray();
    if (choices.isEmpty()) return chunk;
    
    QJsonObject choice = choices[0].toObject();
    
    QString finishReason = choice["finish_reason"].toString();
    if (!finishReason.isEmpty()) {
        chunk.finishReason = finishReason;
    }
    
    QJsonObject delta = choice["delta"].toObject();
    chunk.content = delta["content"].toString();
    
    if (delta.contains("reasoning_content")) {
        chunk.reasoningContent = delta["reasoning_content"].toString();
    }
    
    if (delta.contains("tool_calls")) {
        QJsonArray toolCallsArray = delta["tool_calls"].toArray();
        for (const QJsonValue& val : toolCallsArray) {
            QJsonObject tcObj = val.toObject();
            ToolCallDelta tc;
            tc.index = tcObj["index"].toInt(-1);
            tc.id = tcObj["id"].toString();
            tc.type = tcObj["type"].toString("function");
            
            if (tcObj.contains("function")) {
                QJsonObject funcObj = tcObj["function"].toObject();
                tc.name = funcObj["name"].toString();
                tc.arguments = funcObj["arguments"].toString();
            }
            
            chunk.toolCalls.append(tc);
        }
    }
    
    if (obj.contains("usage")) {
        QJsonObject usage = obj["usage"].toObject();
        chunk.promptTokens = usage["prompt_tokens"].toInt();
        chunk.completionTokens = usage["completion_tokens"].toInt();
        chunk.totalTokens = usage["total_tokens"].toInt();
        chunk.hasUsage = true;
    }
    
    return chunk;
}

}
