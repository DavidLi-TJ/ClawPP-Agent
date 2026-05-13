#include "common/types.h"

namespace clawpp {

Message::Message(MessageRole r, const QString& c)
    : role(r), content(c), timestamp(QDateTime::currentDateTime()) {}

QJsonObject Message::toJson() const {
    QJsonObject obj;
    switch (role) {
        case MessageRole::User: obj["role"] = "user"; break;
        case MessageRole::Assistant: obj["role"] = "assistant"; break;
        case MessageRole::System: obj["role"] = "system"; break;
        case MessageRole::Tool: obj["role"] = "tool"; break;
    }
    
    const bool needsNullableContent =
        role == MessageRole::Assistant
        && !toolCalls.isEmpty()
        && content.isEmpty();
    if (!content.isEmpty()) {
        obj["content"] = content;
    } else if (needsNullableContent) {
        // DeepSeek/OpenAI-compatible tool-call messages may require an explicit nullable content field.
        obj["content"] = QJsonValue(QJsonValue::Null);
    }

    if (!reasoningContent.isEmpty()) {
        obj["reasoning_content"] = reasoningContent;
    }
    
    if (!name.isEmpty()) {
        obj["name"] = name;
    }
    
    if (!toolCallId.isEmpty()) {
        obj["tool_call_id"] = toolCallId;
    }
    
    if (!toolCalls.isEmpty()) {
        QJsonArray callsArray;
        for (const ToolCallData& tc : toolCalls) {
            QJsonObject callObj;
            callObj["id"] = tc.id;
            callObj["type"] = tc.type;
            QJsonObject funcObj;
            funcObj["name"] = tc.name;
            funcObj["arguments"] = tc.arguments;
            callObj["function"] = funcObj;
            callsArray.append(callObj);
        }
        obj["tool_calls"] = callsArray;
    }

    if (!metadata.isEmpty()) {
        obj["metadata"] = metadata;
    }
    
    return obj;
}

Message Message::fromJson(const QJsonObject& obj) {
    Message msg;
    QString roleStr = obj["role"].toString();
    if (roleStr == "user") msg.role = MessageRole::User;
    else if (roleStr == "assistant") msg.role = MessageRole::Assistant;
    else if (roleStr == "system") msg.role = MessageRole::System;
    else msg.role = MessageRole::Tool;
    
    msg.content = obj["content"].toString();
    msg.reasoningContent = obj["reasoning_content"].toString();
    msg.name = obj["name"].toString();
    msg.toolCallId = obj["tool_call_id"].toString();
    msg.timestamp = QDateTime::fromString(obj["timestamp"].toString());
    msg.metadata = obj["metadata"].toObject();
    
    if (obj.contains("tool_calls")) {
        QJsonArray callsArray = obj["tool_calls"].toArray();
        for (const QJsonValue& val : callsArray) {
            QJsonObject callObj = val.toObject();
            ToolCallData tc;
            tc.id = callObj["id"].toString();
            tc.type = callObj["type"].toString("function");
            QJsonObject funcObj = callObj["function"].toObject();
            tc.name = funcObj["name"].toString();
            tc.arguments = funcObj["arguments"].toString();
            msg.toolCalls.append(tc);
        }
    }
    
    return msg;
}

QJsonObject CompactRecord::toJson() const {
    QJsonObject obj;
    obj["index"] = index;
    obj["trigger"] = trigger;
    obj["focus"] = focus;
    obj["archive_path"] = archivePath;
    obj["summary"] = summary;
    obj["created_at"] = createdAt.toString(Qt::ISODate);
    QJsonArray files;
    for (const QString& file : recentFiles) {
        files.append(file);
    }
    obj["recent_files"] = files;
    return obj;
}

CompactRecord CompactRecord::fromJson(const QJsonObject& obj) {
    CompactRecord record;
    record.index = obj["index"].toInt();
    record.trigger = obj["trigger"].toString();
    record.focus = obj["focus"].toString();
    record.archivePath = obj["archive_path"].toString();
    record.summary = obj["summary"].toString();
    record.createdAt = QDateTime::fromString(obj["created_at"].toString(), Qt::ISODate);
    const QJsonArray files = obj["recent_files"].toArray();
    for (const QJsonValue& value : files) {
        record.recentFiles.append(value.toString());
    }
    return record;
}

QJsonObject CompactState::toJson() const {
    QJsonObject obj;
    obj["compact_count"] = compactCount;
    obj["last_summary"] = lastSummary;
    obj["last_archive_path"] = lastArchivePath;
    QJsonArray files;
    for (const QString& file : recentFiles) {
        files.append(file);
    }
    obj["recent_files"] = files;
    QJsonArray records;
    for (const CompactRecord& record : history) {
        records.append(record.toJson());
    }
    obj["history"] = records;
    return obj;
}

CompactState CompactState::fromJson(const QJsonObject& obj) {
    CompactState state;
    state.compactCount = obj["compact_count"].toInt();
    state.lastSummary = obj["last_summary"].toString();
    state.lastArchivePath = obj["last_archive_path"].toString();
    const QJsonArray files = obj["recent_files"].toArray();
    for (const QJsonValue& value : files) {
        state.recentFiles.append(value.toString());
    }
    const QJsonArray records = obj["history"].toArray();
    for (const QJsonValue& value : records) {
        if (value.isObject()) {
            state.history.append(CompactRecord::fromJson(value.toObject()));
        }
    }
    return state;
}

QJsonObject ResponseFormat::toJson() const {
    QJsonObject obj;
    switch (type) {
        case Text: obj["type"] = "text"; break;
        case JsonObject: obj["type"] = "json_object"; break;
        case JsonSchema:
            obj["type"] = "json_schema";
            obj["json_schema"] = jsonSchema;
            break;
    }
    return obj;
}

ResponseFormat ResponseFormat::jsonSchemaFormat(const QJsonObject& schema) {
    ResponseFormat fmt;
    fmt.type = JsonSchema;
    fmt.jsonSchema = schema;
    return fmt;
}

QJsonObject ProviderConfig::toJson() const {
    QJsonObject obj;
    obj["name"] = name;
    obj["type"] = type;
    obj["api_key"] = apiKey;
    obj["base_url"] = baseUrl;
    obj["model"] = model;
    obj["temperature"] = temperature;
    obj["max_tokens"] = maxTokens;
    obj["timeout_ms"] = timeoutMs;
    return obj;
}

ProviderConfig ProviderConfig::fromJson(const QJsonObject& obj) {
    ProviderConfig config;
    config.name = obj["name"].toString();
    config.type = obj["type"].toString("openai");
    config.apiKey = obj["api_key"].toString();
    config.baseUrl = obj["base_url"].toString();
    config.model = obj["model"].toString("gpt-4o-mini");
    config.temperature = obj["temperature"].toDouble(0.7);
    config.maxTokens = obj["max_tokens"].toInt(4096);
    config.timeoutMs = obj["timeout_ms"].toInt(60000);
    return config;
}

MemoryConfig MemoryConfig::fromJson(const QJsonObject& obj) {
    MemoryConfig config;
    config.tokenThreshold = obj["token_threshold"].toInt(4000);
    config.keepFirst = obj["keep_first"].toInt(3);
    config.keepLast = obj["keep_last"].toInt(5);
    config.maxMessages = obj["max_messages"].toInt(100);
    config.summaryRatio = obj["summary_ratio"].toDouble(0.3);
    config.retrievalTopK = obj["retrieval_top_k"].toInt(6);
    config.embeddingDimension = obj["embedding_dimension"].toInt(64);
    config.optimizationThreshold = obj["optimization_threshold"].toDouble(0.92);
    return config;
}

AppConfig AppConfig::fromJson(const QJsonObject& obj) {
    AppConfig config;
    config.theme = obj["theme"].toString("light");
    config.language = obj["language"].toString("zh_CN");
    config.autoSave = obj["auto_save"].toBool(true);
    config.autoSaveInterval = obj["auto_save_interval"].toInt(30);
    return config;
}

}
