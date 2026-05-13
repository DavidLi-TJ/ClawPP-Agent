#include "react_agent_core.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QSet>
#include <QStandardPaths>
#include <QThread>
#include <QTextStream>
#include <QtConcurrent/QtConcurrent>
#include <memory>

#include "common/constants.h"
#include "common/model_context_limits.h"
#include "tool/tool_registry.h"
#include "tool/i_tool.h"
#include "infrastructure/logging/logger.h"

namespace clawpp {

namespace {

QString jsonValueToString(const QJsonValue& value) {
    if (value.isString()) {
        return value.toString();
    }
    if (value.isBool()) {
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    }
    if (value.isDouble()) {
        return QString::number(value.toDouble(), 'g', 15);
    }
    if (value.isArray()) {
        return QString::fromUtf8(QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact));
    }
    if (value.isObject()) {
        return QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));
    }
    return QString();
}

QString normalizeTaggedValue(QString value) {
    value = value.trimmed();
    if (value.startsWith(QStringLiteral("```"))) {
        const int firstNewline = value.indexOf('\n');
        if (firstNewline >= 0) {
            value = value.mid(firstNewline + 1);
            const int fenceEnd = value.lastIndexOf(QStringLiteral("```"));
            if (fenceEnd >= 0) {
                value = value.left(fenceEnd);
            }
        }
    }

    if ((value.startsWith('"') && value.endsWith('"'))
        || (value.startsWith('\'') && value.endsWith('\''))) {
        value = value.mid(1, value.size() - 2);
    }

    return value.trimmed();
}

QString buildToolCallSignature(const ToolCall& toolCall) {
    return toolCall.name.trimmed().toLower()
        + QStringLiteral("|")
        + QString::fromUtf8(QJsonDocument(toolCall.arguments).toJson(QJsonDocument::Compact));
}

void normalizeToolArgsForCall(const QString& toolName, const QString& rawCallBody, QJsonObject* args);

bool looksLikePlanningPreamble(const QString& text) {
    const QString normalized = text.trimmed();
    if (normalized.isEmpty()) {
        return false;
    }

    static const QRegularExpression preambleRe(
        QStringLiteral(
            R"((?i)^(我来帮你|我来帮您|我将帮你|我将帮您|让我来|现在让我|下面我来|I will|Let me|I'll)\b)"));
    return preambleRe.match(normalized).hasMatch();
}

bool isDeterministicToolFailure(const ToolResult& result) {
    if (result.success) {
        return false;
    }

    const QString lower = result.content.trimmed().toLower();
    return lower.contains(QStringLiteral("invalid url"))
        || lower.contains(QStringLiteral("unsupported url scheme"))
        || lower.contains(QStringLiteral("http get failed (404)"))
        || lower.contains(QStringLiteral("http post failed (404)"))
        || lower.contains(QStringLiteral("not found"))
        || lower.contains(QStringLiteral("no such file"))
        || lower.contains(QStringLiteral("cannot find the file"))
        || lower.contains(QStringLiteral("path not found"))
        || lower.contains(QStringLiteral("invalid tool arguments json"))
        || lower.contains(QStringLiteral("permission denied"))
        || lower.contains(QStringLiteral("tool not found"))
        || lower.contains(QStringLiteral("unsupported operation"))
        || lower.contains(QStringLiteral("error: unknown operation"));
}

QString deterministicFailureSummary(const ToolResult& result) {
    QString summary = result.content.simplified();
    if (summary.size() > 160) {
        summary = summary.left(160) + QStringLiteral("...");
    }
    return summary;
}

int overlapSuffixPrefixLength(const QString& left, const QString& right) {
    const int maxOverlap = qMin(left.size(), right.size());
    for (int len = maxOverlap; len > 0; --len) {
        if (left.right(len) == right.left(len)) {
            return len;
        }
    }
    return 0;
}

int commonPrefixLength(const QString& left, const QString& right) {
    const int limit = qMin(left.size(), right.size());
    int i = 0;
    while (i < limit && left.at(i) == right.at(i)) {
        ++i;
    }
    return i;
}

bool isToolIdentifierChar(QChar ch) {
    return ch.isLetterOrNumber() || ch == QChar('_') || ch == QChar('-') || ch == QChar('.');
}

bool startsWithKnownToolName(const QString& fragment) {
    if (fragment.trimmed().isEmpty()) {
        return false;
    }

    const QStringList toolNames = ToolRegistry::instance().listTools();
    for (const QString& toolName : toolNames) {
        if (toolName.startsWith(fragment, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

int findMatchingParen(const QString& text, int openIndex) {
    if (openIndex < 0 || openIndex >= text.size() || text.at(openIndex) != QChar('(')) {
        return -1;
    }

    int depth = 0;
    bool inSingleQuote = false;
    bool inDoubleQuote = false;
    bool escaping = false;

    for (int i = openIndex; i < text.size(); ++i) {
        const QChar ch = text.at(i);
        if (escaping) {
            escaping = false;
            continue;
        }

        if ((inSingleQuote || inDoubleQuote) && ch == QChar('\\')) {
            escaping = true;
            continue;
        }

        if (!inDoubleQuote && ch == QChar('\'')) {
            inSingleQuote = !inSingleQuote;
            continue;
        }
        if (!inSingleQuote && ch == QChar('"')) {
            inDoubleQuote = !inDoubleQuote;
            continue;
        }
        if (inSingleQuote || inDoubleQuote) {
            continue;
        }

        if (ch == QChar('(')) {
            ++depth;
        } else if (ch == QChar(')')) {
            --depth;
            if (depth == 0) {
                return i;
            }
        }
    }

    return -1;
}

int findTopLevelChar(const QString& text, QChar target) {
    int parenDepth = 0;
    int braceDepth = 0;
    int bracketDepth = 0;
    bool inSingleQuote = false;
    bool inDoubleQuote = false;
    bool escaping = false;

    for (int i = 0; i < text.size(); ++i) {
        const QChar ch = text.at(i);
        if (escaping) {
            escaping = false;
            continue;
        }

        if ((inSingleQuote || inDoubleQuote) && ch == QChar('\\')) {
            escaping = true;
            continue;
        }

        if (!inDoubleQuote && ch == QChar('\'')) {
            inSingleQuote = !inSingleQuote;
            continue;
        }
        if (!inSingleQuote && ch == QChar('"')) {
            inDoubleQuote = !inDoubleQuote;
            continue;
        }
        if (inSingleQuote || inDoubleQuote) {
            continue;
        }

        if (ch == QChar('(')) {
            ++parenDepth;
            continue;
        }
        if (ch == QChar(')')) {
            parenDepth = qMax(0, parenDepth - 1);
            continue;
        }
        if (ch == QChar('{')) {
            ++braceDepth;
            continue;
        }
        if (ch == QChar('}')) {
            braceDepth = qMax(0, braceDepth - 1);
            continue;
        }
        if (ch == QChar('[')) {
            ++bracketDepth;
            continue;
        }
        if (ch == QChar(']')) {
            bracketDepth = qMax(0, bracketDepth - 1);
            continue;
        }

        if (parenDepth == 0 && braceDepth == 0 && bracketDepth == 0 && ch == target) {
            return i;
        }
    }

    return -1;
}

QStringList splitTopLevelArguments(const QString& text) {
    QStringList parts;
    int start = 0;
    while (start <= text.size()) {
        const QString remaining = text.mid(start);
        const int commaOffset = findTopLevelChar(remaining, QChar(','));
        if (commaOffset < 0) {
            const QString tail = remaining.trimmed();
            if (!tail.isEmpty()) {
                parts.append(tail);
            }
            break;
        }

        const QString part = remaining.left(commaOffset).trimmed();
        if (!part.isEmpty()) {
            parts.append(part);
        }
        start += commaOffset + 1;
    }
    return parts;
}

QJsonValue parseInlineToolValue(QString value) {
    value = value.trimmed();
    if (value.isEmpty()) {
        return QJsonValue();
    }

    if ((value.startsWith('"') && value.endsWith('"'))
        || (value.startsWith('\'') && value.endsWith('\''))) {
        return QJsonValue(normalizeTaggedValue(value));
    }

    if (value.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0) {
        return QJsonValue(true);
    }
    if (value.compare(QStringLiteral("false"), Qt::CaseInsensitive) == 0) {
        return QJsonValue(false);
    }
    if (value.compare(QStringLiteral("null"), Qt::CaseInsensitive) == 0) {
        return QJsonValue(QJsonValue::Null);
    }

    bool ok = false;
    const double numeric = value.toDouble(&ok);
    if (ok) {
        return QJsonValue(numeric);
    }

    if ((value.startsWith('{') && value.endsWith('}'))
        || (value.startsWith('[') && value.endsWith(']'))) {
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(value.toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError) {
            if (doc.isObject()) {
                return doc.object();
            }
            if (doc.isArray()) {
                return doc.array();
            }
        }
    }

    return QJsonValue(normalizeTaggedValue(value));
}

bool parseInlineToolArguments(const QString& rawArgs, QJsonObject* args) {
    if (!args) {
        return false;
    }

    const QString trimmed = rawArgs.trimmed();
    if (trimmed.isEmpty()) {
        return true;
    }

    const QStringList parts = splitTopLevelArguments(trimmed);
    if (parts.isEmpty()) {
        return false;
    }

    for (const QString& part : parts) {
        const int colonIndex = findTopLevelChar(part, QChar(':'));
        if (colonIndex <= 0) {
            return false;
        }

        QString key = part.left(colonIndex).trimmed();
        QString value = part.mid(colonIndex + 1).trimmed();
        if (key.isEmpty() || value.isEmpty()) {
            return false;
        }

        if ((key.startsWith('"') && key.endsWith('"'))
            || (key.startsWith('\'') && key.endsWith('\''))) {
            key = normalizeTaggedValue(key);
        }

        if (key.isEmpty()) {
            return false;
        }

        args->insert(key, parseInlineToolValue(value));
    }

    return true;
}

QList<CorePendingToolCall> parseInlineToolCalls(const QString& text) {
    QList<CorePendingToolCall> parsed;
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return parsed;
    }

    int index = 0;
    while (index < trimmed.size()) {
        while (index < trimmed.size() && trimmed.at(index).isSpace()) {
            ++index;
        }
        if (index >= trimmed.size()) {
            break;
        }

        const int nameStart = index;
        while (index < trimmed.size() && isToolIdentifierChar(trimmed.at(index))) {
            ++index;
        }
        if (index == nameStart) {
            parsed.clear();
            return parsed;
        }

        const QString toolName = trimmed.mid(nameStart, index - nameStart).trimmed();
        if (!ToolRegistry::instance().hasTool(toolName)) {
            parsed.clear();
            return parsed;
        }

        while (index < trimmed.size() && trimmed.at(index).isSpace()) {
            ++index;
        }
        if (index >= trimmed.size() || trimmed.at(index) != QChar('(')) {
            parsed.clear();
            return parsed;
        }

        const int closeIndex = findMatchingParen(trimmed, index);
        if (closeIndex < 0) {
            parsed.clear();
            return parsed;
        }

        const QString rawArgs = trimmed.mid(index + 1, closeIndex - index - 1);
        QJsonObject args;
        if (!parseInlineToolArguments(rawArgs, &args)) {
            parsed.clear();
            return parsed;
        }

        normalizeToolArgsForCall(toolName, rawArgs, &args);

        CorePendingToolCall call;
        call.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        call.name = toolName;
        call.arguments = QString::fromUtf8(QJsonDocument(args).toJson(QJsonDocument::Compact));
        parsed.append(call);

        index = closeIndex + 1;
        while (index < trimmed.size() && trimmed.at(index).isSpace()) {
            ++index;
        }
    }

    return parsed;
}

int leadingInlineToolCallCutIndex(const QString& text) {
    int index = 0;
    while (index < text.size() && text.at(index).isSpace()) {
        ++index;
    }
    if (index >= text.size()) {
        return -1;
    }

    const int nameStart = index;
    while (index < text.size() && isToolIdentifierChar(text.at(index))) {
        ++index;
    }
    if (index == nameStart) {
        return -1;
    }

    const QString fragment = text.mid(nameStart, index - nameStart);
    const bool exactTool = ToolRegistry::instance().hasTool(fragment);
    const bool likelyToolPrefix = fragment.contains(QChar('_')) && startsWithKnownToolName(fragment);
    if (!exactTool && !likelyToolPrefix) {
        return -1;
    }

    while (index < text.size() && text.at(index).isSpace()) {
        ++index;
    }
    if (index >= text.size()) {
        return nameStart;
    }

    if (text.at(index) == QChar('(')) {
        return nameStart;
    }

    return -1;
}

QString sanitizeVisibleAssistantStreamContent(QString text) {
    text.remove(QRegularExpression(QStringLiteral("(?is)<tool_call>[\\s\\S]*?</tool_call>")));
    text.remove(QRegularExpression(QStringLiteral("(?is)</?arg_(?:key|value)>")));

    const int xmlStart = text.indexOf(QStringLiteral("<tool_call>"), 0, Qt::CaseInsensitive);
    if (xmlStart >= 0) {
        text = text.left(xmlStart);
    }

    if (!parseInlineToolCalls(text).isEmpty()) {
        return QString();
    }

    const int inlineCallStart = leadingInlineToolCallCutIndex(text);
    if (inlineCallStart >= 0) {
        text = text.left(inlineCallStart);
    }

    return text;
}

Message buildAssistantToolCallMessage(const QList<CorePendingToolCall>& pendingToolCalls) {
    Message assistantMsg(MessageRole::Assistant, QString());
    QSet<QString> seenIds;

    for (const CorePendingToolCall& pending : pendingToolCalls) {
        const QString toolName = pending.name.trimmed();
        const QString toolId = pending.id.trimmed();
        if (toolName.isEmpty() || toolId.isEmpty() || seenIds.contains(toolId)) {
            continue;
        }

        ToolCallData call;
        call.id = toolId;
        call.type = QStringLiteral("function");
        call.name = toolName;
        call.arguments = pending.arguments.trimmed().isEmpty()
            ? QStringLiteral("{}")
            : pending.arguments.trimmed();
        assistantMsg.toolCalls.append(call);
        seenIds.insert(toolId);
    }

    return assistantMsg;
}

MessageList normalizeProviderMessageSequence(const MessageList& messages) {
    MessageList sanitized;
    sanitized.reserve(messages.size());

    MessageList pendingGroup;
    QSet<QString> pendingToolIds;
    QSet<QString> resolvedToolIds;

    auto flushPendingGroup = [&](bool keep) mutable {
        if (keep) {
            sanitized.append(pendingGroup);
        }
        pendingGroup.clear();
        pendingToolIds.clear();
        resolvedToolIds.clear();
    };

    for (const Message& rawMessage : messages) {
        Message message = rawMessage;
        bool reprocessCurrent = false;

        do {
            reprocessCurrent = false;

            if (pendingToolIds.isEmpty()) {
                const bool invalidAssistantMessage =
                    message.role == MessageRole::Assistant
                    && message.content.trimmed().isEmpty()
                    && message.toolCalls.isEmpty();
                if (invalidAssistantMessage) {
                    break;
                }

                if (message.role == MessageRole::Tool) {
                    break;
                }

                if (message.role == MessageRole::Assistant && !message.toolCalls.isEmpty()) {
                    Message toolCallMessage = message;
                    toolCallMessage.toolCalls.clear();

                    QSet<QString> nextPendingIds;
                    for (const ToolCallData& call : message.toolCalls) {
                        const QString callId = call.id.trimmed();
                        if (callId.isEmpty() || nextPendingIds.contains(callId)) {
                            continue;
                        }
                        toolCallMessage.toolCalls.append(call);
                        nextPendingIds.insert(callId);
                    }

                    if (toolCallMessage.toolCalls.isEmpty()) {
                        if (!toolCallMessage.content.trimmed().isEmpty()) {
                            sanitized.append(toolCallMessage);
                        }
                        break;
                    }

                    pendingGroup.clear();
                    pendingGroup.append(toolCallMessage);
                    pendingToolIds = nextPendingIds;
                    resolvedToolIds.clear();
                    break;
                }

                if (!sanitized.isEmpty()
                    && sanitized.last().role == MessageRole::Assistant
                    && message.role == MessageRole::Assistant
                    && sanitized.last().toolCalls.isEmpty()
                    && message.toolCalls.isEmpty()
                    && !message.content.trimmed().isEmpty()
                    && sanitized.last().content.trimmed() == message.content.trimmed()) {
                    break;
                }

                sanitized.append(message);
                break;
            }

            if (message.role == MessageRole::Tool) {
                const QString toolCallId = message.toolCallId.trimmed();
                if (toolCallId.isEmpty()
                    || !pendingToolIds.contains(toolCallId)
                    || resolvedToolIds.contains(toolCallId)) {
                    break;
                }

                pendingGroup.append(message);
                resolvedToolIds.insert(toolCallId);
                if (resolvedToolIds.size() == pendingToolIds.size()) {
                    flushPendingGroup(true);
                }
                break;
            }

            flushPendingGroup(false);
            reprocessCurrent = true;
        } while (reprocessCurrent);
    }

    if (!pendingToolIds.isEmpty()) {
        flushPendingGroup(false);
    }

    return sanitized;
}

QString stripInternalNoise(const QString& text) {
    QStringList kept;
    QSet<QString> seen;
    const QStringList lines = text.split('\n');
    for (QString line : lines) {
        const QString simplified = line.simplified();
        if (simplified.isEmpty()) {
            kept.append(line);
            continue;
        }

        const QString lower = simplified.toLower();
        if (lower.contains(QStringLiteral("tool execution blocked"))
            || lower.contains(QStringLiteral("tool not found"))
            || lower.contains(QStringLiteral("repetitive call detected"))
            || lower.contains(QStringLiteral("recent tool result"))
            || lower.contains(QStringLiteral("reader fallback timed out"))
            || lower.contains(QStringLiteral("target site returned 403"))) {
            continue;
        }

        if (!parseInlineToolCalls(simplified).isEmpty()) {
            continue;
        }

        const bool boilerplate = simplified.startsWith(QStringLiteral("我来帮您创作"))
            || simplified.startsWith(QStringLiteral("我来为您创作"))
            || simplified.startsWith(QStringLiteral("首先让我"))
            || simplified.startsWith(QStringLiteral("让我先"))
            || simplified.startsWith(QStringLiteral("根据当前项目状态"));
        if (boilerplate && seen.contains(simplified)) {
            continue;
        }

        seen.insert(simplified);
        kept.append(line);
    }

    QString cleaned = kept.join('\n').trimmed();
    cleaned.replace(QRegularExpression(QStringLiteral("\\n{3,}")), QStringLiteral("\n\n"));
    return cleaned;
}

void upsertToolArg(QJsonObject* args, const QString& key, const QString& rawValue) {
    if (!args) {
        return;
    }
    const QString normalizedKey = key.trimmed();
    if (normalizedKey.isEmpty()) {
        return;
    }
    const QString normalizedValue = normalizeTaggedValue(rawValue);
    if (normalizedValue.isEmpty()) {
        return;
    }
    args->insert(normalizedKey, normalizedValue);
}

void extractJsonArgs(const QString& text, QJsonObject* args) {
    if (!args) {
        return;
    }

    const int firstBrace = text.indexOf('{');
    const int lastBrace = text.lastIndexOf('}');
    if (firstBrace < 0 || lastBrace <= firstBrace) {
        return;
    }

    QJsonParseError parseError;
    const QString candidate = text.mid(firstBrace, lastBrace - firstBrace + 1).trimmed();
    const QJsonDocument doc = QJsonDocument::fromJson(candidate.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return;
    }

    const QJsonObject obj = doc.object();
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        upsertToolArg(args, it.key(), jsonValueToString(it.value()));
    }
}

QJsonObject parseTaggedArguments(const QString& text) {
    QJsonObject args;

    const QRegularExpression nestedToolArgRe(
        QStringLiteral("(?is)<tool_call>\\s*([A-Za-z0-9_\\-\\.]+)\\s*<arg_value>\\s*([\\s\\S]*?)\\s*(?:</arg_value>\\s*)?</tool_call>"));
    QRegularExpressionMatchIterator nestedIt = nestedToolArgRe.globalMatch(text);
    while (nestedIt.hasNext()) {
        const QRegularExpressionMatch m = nestedIt.next();
        upsertToolArg(&args, m.captured(1), m.captured(2));
    }

    const QRegularExpression argKeyValueRe(
        QStringLiteral("(?is)<arg_key>\\s*([A-Za-z0-9_\\-\\.]+)\\s*</arg_key>\\s*<arg_value>\\s*([\\s\\S]*?)\\s*(?:</arg_value>)?"));
    QRegularExpressionMatchIterator argPairIt = argKeyValueRe.globalMatch(text);
    while (argPairIt.hasNext()) {
        const QRegularExpressionMatch m = argPairIt.next();
        upsertToolArg(&args, m.captured(1), m.captured(2));
    }

    const QRegularExpression argKeyRawRe(
        QStringLiteral("(?is)<arg_key>\\s*([A-Za-z0-9_\\-\\.]+)\\s*</arg_key>\\s*([^<\\n][\\s\\S]*?)(?=(?:<arg_key>|<tool_call>|</tool_call>|$))"));
    QRegularExpressionMatchIterator argRawIt = argKeyRawRe.globalMatch(text);
    while (argRawIt.hasNext()) {
        const QRegularExpressionMatch m = argRawIt.next();
        upsertToolArg(&args, m.captured(1), m.captured(2));
    }

    const QRegularExpression xmlArgRe(QStringLiteral("(?is)<([A-Za-z0-9_\\-\\.]+)>\\s*([\\s\\S]*?)\\s*</\\1>"));
    QRegularExpressionMatchIterator xmlIt = xmlArgRe.globalMatch(text);
    while (xmlIt.hasNext()) {
        const QRegularExpressionMatch m = xmlIt.next();
        const QString key = m.captured(1).trimmed();
        if (key.compare(QStringLiteral("tool_call"), Qt::CaseInsensitive) == 0
            || key.compare(QStringLiteral("arg_key"), Qt::CaseInsensitive) == 0
            || key.compare(QStringLiteral("arg_value"), Qt::CaseInsensitive) == 0) {
            continue;
        }
        upsertToolArg(&args, key, m.captured(2));
    }

    if (args.isEmpty()) {
        extractJsonArgs(text, &args);
    }

    return args;
}

QString firstRequiredToolArgName(const QString& toolName) {
    ITool* tool = ToolRegistry::instance().getTool(toolName);
    if (!tool) {
        return QString();
    }

    const QJsonArray required = tool->parameters().value(QStringLiteral("required")).toArray();
    for (const QJsonValue& value : required) {
        const QString key = value.toString().trimmed();
        if (!key.isEmpty()) {
            return key;
        }
    }
    return QString();
}

void normalizeToolArgsForCall(const QString& toolName, const QString& rawCallBody, QJsonObject* args) {
    if (!args) {
        return;
    }

    const QString requiredKey = firstRequiredToolArgName(toolName);
    if (requiredKey.isEmpty()) {
        return;
    }

    auto requiredValue = [&]() -> QString {
        return args->value(requiredKey).toString().trimmed();
    };

    // 兼容形如 <tool_call>list_directory<arg_value>...</arg_value></tool_call> 的写法，
    // 这类写法会被解析为 key=list_directory，需要映射到真正的必填参数名。
    if (requiredValue().isEmpty()) {
        const QString toolKeyValue = args->value(toolName).toString().trimmed();
        if (!toolKeyValue.isEmpty()) {
            args->insert(requiredKey, toolKeyValue);
            args->remove(toolName);
        }
    }

    if (requiredValue().isEmpty()) {
        const QRegularExpression loneArgRe(QStringLiteral("(?is)<arg_value>\\s*([\\s\\S]*?)\\s*(?:</arg_value>)?"));
        const QRegularExpressionMatch loneArgMatch = loneArgRe.match(rawCallBody);
        if (loneArgMatch.hasMatch()) {
            const QString loneValue = normalizeTaggedValue(loneArgMatch.captured(1));
            if (!loneValue.isEmpty()) {
                args->insert(requiredKey, loneValue);
            }
        }
    }

    if (requiredValue().isEmpty() && args->size() == 1) {
        const QJsonObject::const_iterator it = args->constBegin();
        const QString guessed = normalizeTaggedValue(jsonValueToString(it.value()));
        if (!guessed.isEmpty()) {
            args->insert(requiredKey, guessed);
        }
    }
}

QList<CorePendingToolCall> parseTaggedToolCalls(const QString& text) {
    QList<CorePendingToolCall> parsed;
    if (!text.contains(QStringLiteral("<tool_call>"), Qt::CaseInsensitive)) {
        return parsed;
    }

    const QRegularExpression callBlockRe(
        QStringLiteral("(?is)<tool_call>\\s*([A-Za-z0-9_\\-\\.]+)\\s*([\\s\\S]*?)</tool_call>"));
    QRegularExpressionMatchIterator callIt = callBlockRe.globalMatch(text);
    while (callIt.hasNext()) {
        const QRegularExpressionMatch m = callIt.next();
        const QString toolName = m.captured(1).trimmed();
        if (toolName.isEmpty() || !ToolRegistry::instance().hasTool(toolName)) {
            continue;
        }

        QJsonObject args = parseTaggedArguments(m.captured(2));
        normalizeToolArgsForCall(toolName, m.captured(2), &args);

        CorePendingToolCall call;
        call.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        call.name = toolName;
        call.arguments = QString::fromUtf8(QJsonDocument(args).toJson(QJsonDocument::Compact));
        parsed.append(call);
    }

    if (!parsed.isEmpty()) {
        return parsed;
    }

    const QRegularExpression firstToolRe(QStringLiteral("(?is)<tool_call>\\s*([A-Za-z0-9_\\-\\.]+)"));
    const QRegularExpressionMatch firstMatch = firstToolRe.match(text);
    const QString toolName = firstMatch.hasMatch() ? firstMatch.captured(1).trimmed() : QString();
    if (toolName.isEmpty()) {
        return parsed;
    }

    QJsonObject args = parseTaggedArguments(text);
    normalizeToolArgsForCall(toolName, text, &args);

    CorePendingToolCall call;
    call.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    call.name = toolName;
    call.arguments = QString::fromUtf8(QJsonDocument(args).toJson(QJsonDocument::Compact));
    parsed.append(call);
    return parsed;
}

int contextCompressionThreshold(const ProviderConfig& config) {
    const int contextLimit = modelctx::inferModelContextLimit(config);
    return qMax(1, static_cast<int>(contextLimit * 0.6));
}

bool isAuthLikeToolFailure(const QString& text) {
    const QString normalized = text.toLower();
    return normalized.contains(QStringLiteral("apikeyinvalid"))
        || normalized.contains(QStringLiteral("api key is invalid"))
        || normalized.contains(QStringLiteral("api key invalid"))
        || normalized.contains(QStringLiteral("unauthorized"))
        || normalized.contains(QStringLiteral("forbidden"))
        || normalized.contains(QStringLiteral("invalid token"))
        || normalized.contains(QStringLiteral("http get failed (401)"))
        || normalized.contains(QStringLiteral("http post failed (401)"))
        || normalized.contains(QStringLiteral("主机需要验证"))
        || normalized.contains(QStringLiteral("鉴权"))
        || normalized.contains(QStringLiteral("认证失败"));
}

QString sanitizeSessionKey(QString sessionId) {
    sessionId = sessionId.trimmed();
    if (sessionId.isEmpty()) {
        return QStringLiteral("default");
    }
    sessionId.replace(QRegularExpression(QStringLiteral(R"([^A-Za-z0-9_\-])")), QStringLiteral("_"));
    return sessionId;
}

QString compactStateRootPath(const QString& sessionId) {
    QString root = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (root.isEmpty()) {
        root = QDir::tempPath() + QStringLiteral("/clawpp");
    }
    return QDir(root).filePath(QStringLiteral("compact/%1").arg(sanitizeSessionKey(sessionId)));
}

QString compactStateFilePath(const QString& sessionId) {
    return QDir(compactStateRootPath(sessionId)).filePath(QStringLiteral("compact_state.json"));
}

QString compactArchiveDirPath(const QString& sessionId) {
    return QDir(compactStateRootPath(sessionId)).filePath(QStringLiteral("archives"));
}

QString messageRoleLabel(MessageRole role) {
    switch (role) {
        case MessageRole::User: return QStringLiteral("user");
        case MessageRole::Assistant: return QStringLiteral("assistant");
        case MessageRole::System: return QStringLiteral("system");
        case MessageRole::Tool: return QStringLiteral("tool");
    }
    return QStringLiteral("unknown");
}

}

ReactAgentCore::ReactAgentCore(QObject* parent)
    : IAgentCore(parent)
    , m_provider(nullptr)
    , m_memory(nullptr)
    , m_executor(nullptr)
    , m_providerConfig()
    , m_runGeneration(0)
    , m_iteration(0)
    , m_noToolIterationStreak(0)
    , m_retryCount(0)
    , m_lastContextTokens(0)
    , m_consecutiveToolFailureRounds(0)
    , m_postToolRecoveryStage(0)
    , m_lightweightMode(false)
    , m_toolsEnabled(true)
    , m_stopped(false)
    , m_hadToolFailure(false) {
    loadCompactState();
}

void ReactAgentCore::setProvider(ILLMProvider* provider) {
    m_provider = provider;
}

void ReactAgentCore::setMemory(IMemorySystem* memory) {
    m_memory = memory;
}

void ReactAgentCore::setExecutor(ToolExecutor* executor) {
    m_executor = executor;
}

void ReactAgentCore::setProviderConfig(const ProviderConfig& config) {
    m_providerConfig = config;
}

void ReactAgentCore::setSessionId(const QString& sessionId) {
    const QString normalized = sessionId.trimmed();
    if (m_sessionId == normalized) {
        return;
    }
    m_sessionId = normalized;
    m_compactState = CompactState();
    loadCompactState();
}

void ReactAgentCore::setRuntimeToolNames(const QStringList& toolNames) {
    m_runtimeToolNames = toolNames;
}

void ReactAgentCore::setRuntimeContext(const QString& runtimeContext) {
    m_runtimeContext = runtimeContext.trimmed();
}

void ReactAgentCore::setLightweightMode(bool enabled) {
    m_lightweightMode = enabled;
}

void ReactAgentCore::setToolsEnabled(bool enabled) {
    m_toolsEnabled = enabled;
}

void ReactAgentCore::run(const QString& input) {
    run(input, QString());
}

void ReactAgentCore::run(const QString& input, const QString& runtimeContext) {
    if (!m_provider) {
        LOG_ERROR(QStringLiteral("ReactAgentCore run failed: provider is null"));
        emit errorOccurred("No provider configured");
        return;
    }

    m_stopped = false;
    ++m_runGeneration;
    m_iteration = 0;
    m_currentThought.clear();
    m_currentContent.clear();
    m_currentReasoningContent.clear();
    m_pendingToolCalls.clear();
    m_preExecutingTools.clear();
    m_preExecutedResults.clear();
    m_parsedToolIds.clear();
    m_toolCallExecutionCounts.clear();
    m_deterministicFailedToolCalls.clear();
    m_noToolIterationStreak = 0;
    m_retryCount = 0;
    m_lastContextTokens = 0;
    m_consecutiveToolFailureRounds = 0;
    m_postToolRecoveryStage = 0;
    m_hadToolFailure = false;
    if (!runtimeContext.trimmed().isEmpty()) {
        m_runtimeContext = runtimeContext.trimmed();
    }

    m_messages = m_context;
    
    Message userMsg(MessageRole::User, input);
    m_messages.append(userMsg);
    if (m_memory) {
        m_memory->addMessage(userMsg);
    }

    emit conversationUpdated(m_messages);
    
    processIteration();
}

void ReactAgentCore::stop() {
    m_stopped = true;
    ++m_runGeneration;
    m_preExecutingTools.clear();
    m_preExecutedResults.clear();
    m_pendingToolCalls.clear();
    m_toolCallExecutionCounts.clear();
    m_deterministicFailedToolCalls.clear();
    if (m_provider) {
        m_provider->abort();
    }
}

void ReactAgentCore::setContext(const MessageList& messages) {
    m_context = messages;
    m_messages = messages;
}

void ReactAgentCore::setSystemPrompt(const QString& prompt) {
    m_systemPrompt = prompt;
}

void ReactAgentCore::processIteration() {
    if (finalizeIfStoppedOrExceeded()) {
        return;
    }
    
    m_iteration++;
    LOG_INFO(QStringLiteral("ReactAgentCore iteration started: %1").arg(m_iteration));
    m_parsedToolIds.clear();

    MessageList messages = buildIterationMessages();

    ChatOptions options;
    options.model = m_providerConfig.model;
    options.temperature = m_providerConfig.temperature;
    options.maxTokens = m_providerConfig.maxTokens;
    options.stream = m_provider->supportsStreaming();

    if (m_toolsEnabled && m_provider->supportsTools()) {
        if (!m_runtimeToolNames.isEmpty()) {
            QJsonArray scopedTools;
            for (const QString& toolName : m_runtimeToolNames) {
                ITool* tool = ToolRegistry::instance().getTool(toolName.trimmed());
                if (tool) {
                    scopedTools.append(tool->toJson());
                }
            }
            options.tools = scopedTools;
        } else {
            options.tools = ToolRegistry::instance().toolsSchema();
        }
    }

    const quint64 runGeneration = m_runGeneration;

    QPointer<ILLMProvider> provider(m_provider);
    if (!provider) {
        emit errorOccurred(QStringLiteral("Provider 已失效"));
        return;
    }

    if (!options.stream) {
        auto startNonStream = [this, provider, messages, options, runGeneration]() {
            if (!provider) {
                return;
            }
            provider->chat(messages, options,
                [this, runGeneration](const QString& response) {
                    auto handleResponse = [this, runGeneration, response]() {
                        if (m_stopped || runGeneration != m_runGeneration) {
                            return;
                        }
                        if (response.isEmpty()) {
                            handleStreamCompleted(runGeneration);
                            return;
                        }

                        auto payload = std::make_shared<QString>(response);
                        auto cursor = std::make_shared<int>(0);
                        auto pump = std::make_shared<std::function<void()>>();
                        *pump = [this, runGeneration, payload, cursor, pump]() {
                            if (m_stopped || runGeneration != m_runGeneration) {
                                return;
                            }
                            if (*cursor >= payload->size()) {
                                handleStreamCompleted(runGeneration);
                                return;
                            }
                            const int remaining = payload->size() - *cursor;
                            int chunkSize = 12;
                            if (remaining > 2400) {
                                chunkSize = 56;
                            } else if (remaining > 1400) {
                                chunkSize = 44;
                            } else if (remaining > 720) {
                                chunkSize = 32;
                            } else if (remaining > 280) {
                                chunkSize = 24;
                            } else if (remaining > 120) {
                                chunkSize = 18;
                            }
                            StreamChunk chunk;
                            chunk.content = payload->mid(*cursor, chunkSize);
                            *cursor += chunkSize;
                            handleStreamChunk(chunk, runGeneration);
                            int delayMs = 7;
                            if (!chunk.content.isEmpty()) {
                                const QChar tail = chunk.content.back();
                                if (tail == '\n') {
                                    delayMs = 16;
                                } else if (QStringLiteral(".,!?;:，。！？；：").contains(tail)) {
                                    delayMs = 11;
                                }
                            }
                            QTimer::singleShot(delayMs, this, [pump]() { (*pump)(); });
                        };
                        (*pump)();
                    };
                    if (this->thread() == QThread::currentThread()) {
                        handleResponse();
                    } else {
                        QMetaObject::invokeMethod(this, handleResponse, Qt::QueuedConnection);
                    }
                },
                [this, runGeneration](const QString& error) {
                    auto handleError = [this, runGeneration, error]() {
                        handleStreamError(error, runGeneration);
                    };
                    if (this->thread() == QThread::currentThread()) {
                        handleError();
                    } else {
                        QMetaObject::invokeMethod(this, handleError, Qt::QueuedConnection);
                    }
                }
            );
        };
        if (provider->thread() == QThread::currentThread()) {
            startNonStream();
        } else {
            QMetaObject::invokeMethod(provider, startNonStream, Qt::QueuedConnection);
        }
        return;
    }

    auto startStream = [this, provider, messages, options, runGeneration]() {
        if (!provider) {
            return;
        }
        provider->chatStream(messages, options,
            [this, runGeneration](const StreamChunk& chunk) {
                auto handleChunk = [this, runGeneration, chunk]() {
                    handleStreamChunk(chunk, runGeneration);
                };
                if (this->thread() == QThread::currentThread()) {
                    handleChunk();
                } else {
                    QMetaObject::invokeMethod(this, handleChunk, Qt::QueuedConnection);
                }
            },
            [this, runGeneration]() {
                auto handleComplete = [this, runGeneration]() {
                    handleStreamCompleted(runGeneration);
                };
                if (this->thread() == QThread::currentThread()) {
                    handleComplete();
                } else {
                    QMetaObject::invokeMethod(this, handleComplete, Qt::QueuedConnection);
                }
            },
            [this, runGeneration](const QString& error) {
                auto handleError = [this, runGeneration, error]() {
                    handleStreamError(error, runGeneration);
                };
                if (this->thread() == QThread::currentThread()) {
                    handleError();
                } else {
                    QMetaObject::invokeMethod(this, handleError, Qt::QueuedConnection);
                }
            }
        );
    };
    if (provider->thread() == QThread::currentThread()) {
        startStream();
    } else {
        QMetaObject::invokeMethod(provider, startStream, Qt::QueuedConnection);
    }
}

bool ReactAgentCore::finalizeIfStoppedOrExceeded() {
    // 使用 7 种继续策略评估当前状态
    ContinueReason reason = evaluateContinueReason();
    
    if (reason == ContinueReason::NormalCompletion) {
        // 正常情况，继续执行
        return false;
    }
    
    // 其他情况，处理继续策略
    handleContinueReason(reason);
    
    // 非 NormalCompletion 都已在 handleContinueReason 内部完成“终止或续跑”，
    // 当前这一层不应再继续向下执行，否则会出现重复迭代和状态错乱。
    return true;
}

MessageList ReactAgentCore::buildIterationMessages() const {
    MessageList messages;
    QString mergedSystemPrompt = m_systemPrompt;
    if (!m_runtimeContext.trimmed().isEmpty()) {
        if (!mergedSystemPrompt.trimmed().isEmpty()) {
            mergedSystemPrompt += QStringLiteral("\n\n");
        }
        mergedSystemPrompt += m_runtimeContext.trimmed();
    }
    if (!m_lightweightMode && m_memory) {
        const QStringList semanticMemory = m_memory->queryRelevantMemory(
            m_messages.isEmpty() ? QString() : m_messages.last().content, 6);
        if (!semanticMemory.isEmpty()) {
            mergedSystemPrompt += QStringLiteral("\n\n[Relevant Memory]\n- %1")
                .arg(semanticMemory.join(QStringLiteral("\n- ")));
        }
    }
    if (!mergedSystemPrompt.isEmpty()) {
        messages.append(Message(MessageRole::System, mergedSystemPrompt));
    }

    MessageList contextMessages = sanitizeMessagesForProvider(m_messages);
    compressMessagesForContext(contextMessages);
    messages.append(contextMessages);
    return messages;
}

MessageList ReactAgentCore::sanitizeMessagesForProvider(const MessageList& messages) {
    return normalizeProviderMessageSequence(messages);
}

void ReactAgentCore::handleStreamChunk(const StreamChunk& chunk, quint64 runGeneration) {
    if (m_stopped || runGeneration != m_runGeneration) {
        return;
    }

    const QString visibleBefore = sanitizeVisibleAssistantStreamContent(m_currentContent);

    if (!chunk.content.isEmpty()) {
        QString delta = chunk.content;
        if (m_currentContent == chunk.content || m_currentContent.endsWith(chunk.content)) {
            delta.clear();
        } else if (chunk.content.startsWith(m_currentContent)) {
            delta = chunk.content.mid(m_currentContent.size());
        } else {
            const int overlap = overlapSuffixPrefixLength(m_currentContent, chunk.content);
            if (overlap > 0) {
                delta = chunk.content.mid(overlap);
            }
        }

        if (!delta.isEmpty()) {
            m_currentThought += delta;
            m_currentContent += delta;
        }
    }

    if (!chunk.reasoningContent.isEmpty()) {
        QString delta = chunk.reasoningContent;
        if (m_currentReasoningContent == chunk.reasoningContent
            || m_currentReasoningContent.endsWith(chunk.reasoningContent)) {
            delta.clear();
        } else if (chunk.reasoningContent.startsWith(m_currentReasoningContent)) {
            delta = chunk.reasoningContent.mid(m_currentReasoningContent.size());
        } else {
            const int overlap = overlapSuffixPrefixLength(m_currentReasoningContent, chunk.reasoningContent);
            if (overlap > 0) {
                delta = chunk.reasoningContent.mid(overlap);
            }
        }

        if (!delta.isEmpty()) {
            m_currentReasoningContent += delta;
        }
    }

    StreamChunk visibleChunk = chunk;
    if (!chunk.content.isEmpty()) {
        const QString visibleAfter = sanitizeVisibleAssistantStreamContent(m_currentContent);
        QString visibleDelta;
        if (visibleAfter == visibleBefore || visibleBefore.endsWith(visibleAfter)) {
            visibleDelta.clear();
        } else if (visibleAfter.startsWith(visibleBefore)) {
            visibleDelta = visibleAfter.mid(visibleBefore.size());
        } else {
            const int prefix = commonPrefixLength(visibleBefore, visibleAfter);
            if (prefix > 0 && visibleAfter.size() >= prefix) {
                visibleDelta = visibleAfter.mid(prefix);
            } else {
                const int overlap = overlapSuffixPrefixLength(visibleBefore, visibleAfter);
                visibleDelta = overlap > 0 ? visibleAfter.mid(overlap) : visibleAfter;
            }
        }
        visibleChunk.content = visibleDelta;
    }

    emit responseChunk(visibleChunk);

    if (chunk.hasUsage) {
        emit usageReport(chunk.promptTokens, chunk.completionTokens, chunk.totalTokens);
    }

    for (const ToolCallDelta& tc : chunk.toolCalls) {
        if (tc.index < 0) {
            continue;
        }

        while (m_pendingToolCalls.size() <= tc.index) {
            m_pendingToolCalls.append(CorePendingToolCall{});
        }

        if (!tc.id.isEmpty()) {
            m_pendingToolCalls[tc.index].id = tc.id;
        }
        if (!tc.name.isEmpty()) {
            m_pendingToolCalls[tc.index].name = tc.name;
        }
        if (!tc.arguments.isEmpty()) {
            m_pendingToolCalls[tc.index].arguments += tc.arguments;
        }
        
        // 检测工具调用是否完整，如果完整则通知 UI
        const CorePendingToolCall& pending = m_pendingToolCalls[tc.index];
        if (isToolCallComplete(pending) && !m_parsedToolIds.contains(pending.id)) {
            m_parsedToolIds.insert(pending.id);
            ToolCall toolCall = createToolCall(pending);
            emit toolCallParsed(toolCall);
        }
    }
}

void ReactAgentCore::handleStreamCompleted(quint64 runGeneration) {
    if (m_stopped || runGeneration != m_runGeneration) {
        return;
    }
    
    // 流式完成后立即启动工具预执行（异步非阻塞）
    startToolPreExecution();
    
    processThought();
}

void ReactAgentCore::handleStreamError(const QString& error, quint64 runGeneration) {
    if (m_stopped || runGeneration != m_runGeneration) {
        return;
    }
    const QString normalizedError = error.trimmed().isEmpty()
        ? QStringLiteral("模型流式调用失败")
        : error;
    
    LOG_ERROR(QStringLiteral("ReactAgentCore stream error at iteration %1: %2")
        .arg(m_iteration)
        .arg(error.isEmpty() ? QStringLiteral("Unknown error") : error));
    emit errorOccurred(normalizedError);
    
    // 触发流式错误策略
    handleContinueReason(ContinueReason::StreamError);
}

void ReactAgentCore::processThought() {
    if (m_stopped) {
        finalizeResponse(m_currentContent.isEmpty() ? m_currentThought : m_currentContent);
        return;
    }

    if (hasFinalAnswerMarker(m_currentContent) || hasFinalAnswerMarker(m_currentThought)) {
        QString finalText = m_currentContent.isEmpty() ? m_currentThought : m_currentContent;
        finalText.remove(QRegularExpression(QStringLiteral("(?i)\\bfinal[_\\s-]*answer\\b\\s*[:：-]?")));
        finalText = finalText.trimmed();
        finalizeResponse(finalText);
        return;
    }
    
    bool hasNamedPendingTool = false;
    for (const CorePendingToolCall& pending : m_pendingToolCalls) {
        if (!pending.name.trimmed().isEmpty()) {
            hasNamedPendingTool = true;
            break;
        }
    }

    if (!hasNamedPendingTool) {
        const QList<CorePendingToolCall> taggedCalls = parseTaggedToolCalls(m_currentContent);
        if (!taggedCalls.isEmpty()) {
            for (const CorePendingToolCall& call : taggedCalls) {
                m_pendingToolCalls.append(call);
                if (!m_parsedToolIds.contains(call.id)) {
                    m_parsedToolIds.insert(call.id);
                    emit toolCallParsed(createToolCall(call));
                }
            }
            m_currentContent.remove(QRegularExpression(QStringLiteral("(?is)<tool_call>[\\s\\S]*?</tool_call>")));
            m_currentContent.remove(QRegularExpression(QStringLiteral("(?is)</?arg_(?:key|value)>")));
            m_currentContent = sanitizeVisibleAssistantStreamContent(m_currentContent);
            m_currentThought = m_currentContent;
        } else {
            const QList<CorePendingToolCall> inlineCalls = parseInlineToolCalls(m_currentContent);
            if (!inlineCalls.isEmpty()) {
                for (const CorePendingToolCall& call : inlineCalls) {
                    m_pendingToolCalls.append(call);
                    if (!m_parsedToolIds.contains(call.id)) {
                        m_parsedToolIds.insert(call.id);
                        emit toolCallParsed(createToolCall(call));
                    }
                }
                m_currentContent.clear();
                m_currentThought.clear();
            }
        }
    }

    if (!m_pendingToolCalls.isEmpty()) {
        m_noToolIterationStreak = 0;
        LOG_INFO(QStringLiteral("ReactAgentCore executing %1 pending tool call(s)").arg(m_pendingToolCalls.size()));
        executeToolCalls();
    } else {
        ++m_noToolIterationStreak;
        const QString trimmedContent = m_currentContent.trimmed();
        if (!trimmedContent.isEmpty()) {
            const bool planningOnlyWithTools =
                m_toolsEnabled
                && !hasRecentToolResult()
                && looksLikePlanningPreamble(trimmedContent);
            if (planningOnlyWithTools
                && m_noToolIterationStreak < (constants::MAX_NO_TOOL_STREAK + 1)) {
                LOG_INFO(QStringLiteral("ReactAgentCore detected planning preamble without tool call, forcing tool-first retry at iteration %1")
                    .arg(m_iteration));
                appendRecoveryMessage(QStringLiteral("不要继续口头说明。直接调用完成该请求所需的最少工具；若用户要求保存结果，完成写入后再给最终回答。"));
                emit conversationUpdated(m_messages);
                m_currentThought.clear();
                m_currentContent.clear();
                const quint64 runGeneration = m_runGeneration;
                QTimer::singleShot(60, this, [this, runGeneration]() {
                    if (m_stopped || runGeneration != m_runGeneration) {
                        return;
                    }
                    processIteration();
                });
                return;
            }

            if (m_lightweightMode) {
                LOG_INFO(QStringLiteral("ReactAgentCore lightweight mode completed in iteration %1").arg(m_iteration));
                finalizeResponse(trimmedContent);
                return;
            }
            const bool shouldContinue = shouldContinueForInterimContent(trimmedContent)
                || isLikelyTruncatedAssistantTurn(trimmedContent);
            if (shouldContinue
                && m_noToolIterationStreak < (constants::MAX_NO_TOOL_STREAK + 2)) {
                LOG_INFO(QStringLiteral("ReactAgentCore detected interim content, continue iteration %1 (streak=%2)")
                    .arg(m_iteration)
                    .arg(m_noToolIterationStreak));
                Message partialAssistant(MessageRole::Assistant, trimmedContent);
                m_messages.append(partialAssistant);
                if (m_memory) {
                    m_memory->addMessage(partialAssistant);
                }
                appendRecoveryMessage(QStringLiteral("继续从上文末尾生成，不要重复已输出内容。"));
                emit conversationUpdated(m_messages);
                m_currentThought.clear();
                m_currentContent.clear();
                m_noToolIterationStreak = 0;
            } else {
                LOG_INFO(QStringLiteral("ReactAgentCore completed with direct assistant content in iteration %1").arg(m_iteration));
                finalizeResponse(trimmedContent);
                return;
            }
        }

        if (trimmedContent.isEmpty() && hasRecentToolResult()) {
            if (m_postToolRecoveryStage <= 1) {
                ++m_postToolRecoveryStage;
                const bool shellAlreadyUsed = hasRecentToolNamed(QStringLiteral("shell"));
                QString recoveryPrompt = QStringLiteral("继续完成用户请求。如果还需要工具，请直接执行下一步必要操作；只有任务完成后再给最终回答。");
                if (!shellAlreadyUsed) {
                    recoveryPrompt = QStringLiteral("继续完成用户请求。如果你目前只创建或检查了文件，下一步应直接执行必要命令（例如编译、运行或验证），不要重复前导说明。任务真正完成后再给最终回答。");
                }
                appendRecoveryMessage(recoveryPrompt);
                emit conversationUpdated(m_messages);
                const quint64 runGeneration = m_runGeneration;
                QTimer::singleShot(60, this, [this, runGeneration]() {
                    if (m_stopped || runGeneration != m_runGeneration) {
                        return;
                    }
                    processIteration();
                });
                return;
            }

            if (m_postToolRecoveryStage == 2) {
                m_postToolRecoveryStage = 3;
                appendRecoveryMessage(QStringLiteral("不要重复计划。基于已有工具结果直接给出最终回答；除非绝对必要，不要再调用工具。"));
                m_toolsEnabled = false;
                emit conversationUpdated(m_messages);
                const quint64 runGeneration = m_runGeneration;
                QTimer::singleShot(60, this, [this, runGeneration]() {
                    if (m_stopped || runGeneration != m_runGeneration) {
                        return;
                    }
                    processIteration();
                });
                return;
            }
        }

        if (trimmedContent.isEmpty() && m_postToolRecoveryStage >= 3 && hasRecentToolResult()) {
            finalizeResponse(QString());
            return;
        }

        if (trimmedContent.isEmpty() && looksLikePlanningPreamble(m_currentThought)) {
            finalizeResponse(QString());
            return;
        }

        if (trimmedContent.isEmpty() && m_lightweightMode) {
            finalizeResponse(QString());
            return;
        }

        if (trimmedContent.isEmpty() && hasRecentToolResult() && m_postToolRecoveryStage > 0) {
            emit conversationUpdated(m_messages);
            const quint64 runGeneration = m_runGeneration;
            QTimer::singleShot(60, this, [this, runGeneration]() {
                if (m_stopped || runGeneration != m_runGeneration) {
                    return;
                }
                processIteration();
            });
            return;
        }

        if (shouldTerminateByNoToolStreak()) {
            LOG_INFO(QStringLiteral("ReactAgentCore terminated by no-tool streak: %1").arg(m_noToolIterationStreak));
            finalizeResponse(m_currentContent);
            return;
        }

        const quint64 runGeneration = m_runGeneration;
        QTimer::singleShot(80, this, [this, runGeneration]() {
            if (m_stopped || runGeneration != m_runGeneration) {
                return;
            }
            processIteration();
        });
    }
}

void ReactAgentCore::executeToolCalls() {
    if (m_pendingToolCalls.isEmpty()) {
        finalizeResponse(m_currentContent);
        return;
    }

    // 先等待所有预执行的工具完成
    checkPreExecutionResults();

    for (CorePendingToolCall& pendingToolCall : m_pendingToolCalls) {
        if (pendingToolCall.name.trimmed().isEmpty()) {
            continue;
        }
        if (pendingToolCall.id.trimmed().isEmpty()) {
            pendingToolCall.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        }
    }

    const Message assistantToolCallMsg = buildAssistantToolCallMessage(m_pendingToolCalls);
    if (!assistantToolCallMsg.toolCalls.isEmpty()) {
        Message storedAssistantToolCallMsg = assistantToolCallMsg;
        storedAssistantToolCallMsg.reasoningContent = m_currentReasoningContent;
        m_messages.append(storedAssistantToolCallMsg);
        if (m_memory) {
            m_memory->addMessage(storedAssistantToolCallMsg);
        }
    }

    // 准备工具调用列表
    QVector<ToolCall> validToolCalls;
    QVector<int> validToolIndexes;  // 记录有效工具在原始列表中的索引

    for (int i = 0; i < m_pendingToolCalls.size(); ++i) {
        const CorePendingToolCall& pendingToolCall = m_pendingToolCalls[i];
        
        if (pendingToolCall.name.isEmpty()) {
            LOG_WARN(QStringLiteral("Skipping pending tool call with empty name"));
            Message systemMsg(MessageRole::System, QStringLiteral("Tool execution failed: empty tool name"));
            m_messages.append(systemMsg);
            if (m_memory) {
                m_memory->addMessage(systemMsg);
            }
            continue;
        }

        ToolCall toolCall;
        toolCall.id = pendingToolCall.id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : pendingToolCall.id;
        toolCall.name = pendingToolCall.name;

        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(pendingToolCall.arguments.toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError && document.isObject()) {
            toolCall.arguments = document.object();
        } else if (!pendingToolCall.arguments.trimmed().isEmpty()) {
            LOG_WARN(QStringLiteral("Tool arguments JSON parse failed for %1: %2")
                         .arg(toolCall.name, parseError.errorString()));
            ToolResult invalidArgsResult(toolCall.id, false, QStringLiteral("Invalid tool arguments JSON"));
            invalidArgsResult.metadata[QStringLiteral("tool_name")] = toolCall.name;
            invalidArgsResult.metadata[QStringLiteral("parse_error")] = parseError.errorString();
            emit toolCallRequested(toolCall);

            Message toolMsg(MessageRole::Tool, invalidArgsResult.content);
            toolMsg.toolCallId = toolCall.id;
            toolMsg.name = toolCall.name;
            toolMsg.metadata = invalidArgsResult.metadata;
            m_messages.append(toolMsg);
            if (m_memory) {
                m_memory->addMessage(toolMsg);
            }
            continue;
        }

        const QString signature = buildToolCallSignature(toolCall);
        const QString deterministicFailure = m_deterministicFailedToolCalls.value(signature);
        if (!deterministicFailure.isEmpty()) {
            LOG_WARN(QStringLiteral("Blocking deterministic retry of tool '%1'").arg(toolCall.name));
            m_hadToolFailure = true;
            Message toolMsg(
                MessageRole::Tool,
                QStringLiteral("Tool execution blocked: deterministic failure already observed (%1). Change the arguments or choose a different method instead of repeating the same call.")
                    .arg(deterministicFailure));
            toolMsg.toolCallId = toolCall.id;
            toolMsg.name = toolCall.name;
            toolMsg.metadata[QStringLiteral("display_content")] =
                QStringLiteral("工具 %1 被阻止重复执行").arg(toolCall.name);
            toolMsg.metadata[QStringLiteral("internal")] = true;
            toolMsg.metadata[QStringLiteral("ui_hidden")] = true;
            m_messages.append(toolMsg);
            if (m_memory) {
                m_memory->addMessage(toolMsg);
            }
            continue;
        }

        const int executedCount = m_toolCallExecutionCounts.value(signature, 0);
        if (executedCount >= 3) {
            LOG_WARN(QStringLiteral("Blocking repetitive tool call '%1' after %2 executions")
                .arg(toolCall.name)
                .arg(executedCount));
            m_hadToolFailure = true;
            Message toolMsg(
                MessageRole::Tool,
                QStringLiteral("Tool execution blocked: repetitive call detected (%1). Please infer from previous result instead of repeating this call.")
                    .arg(toolCall.name));
            toolMsg.toolCallId = toolCall.id;
            toolMsg.name = toolCall.name;
            toolMsg.metadata[QStringLiteral("display_content")] =
                QStringLiteral("工具 %1 被阻止重复执行").arg(toolCall.name);
            toolMsg.metadata[QStringLiteral("internal")] = true;
            toolMsg.metadata[QStringLiteral("ui_hidden")] = true;
            m_messages.append(toolMsg);
            if (m_memory) {
                m_memory->addMessage(toolMsg);
            }
            continue;
        }

        m_toolCallExecutionCounts.insert(signature, executedCount + 1);
        emit toolCallRequested(toolCall);
        validToolCalls.append(toolCall);
        validToolIndexes.append(i);
    }

    if (!m_executor) {
        if (!validToolCalls.isEmpty()) {
            emit errorOccurred(QStringLiteral("No tool executor configured"));
            return;
        }

        // 没有可执行工具（例如全部参数无效）时，仍继续下一轮，保持原有循环行为。
        emit conversationUpdated(m_messages);
        m_pendingToolCalls.clear();
        m_currentThought.clear();
        m_currentContent.clear();
        const quint64 runGeneration = m_runGeneration;
        QTimer::singleShot(100, this, [this, runGeneration]() {
            if (m_stopped || runGeneration != m_runGeneration) {
                return;
            }
            processIteration();
        });
        return;
    }

    LOG_INFO(QStringLiteral("Executing %1 tools (with pre-execution cache)").arg(validToolCalls.size()));
    QVector<ToolResult> results;
    results.reserve(validToolCalls.size());
    QVector<ToolCall> remainingToolCalls;
    QVector<int> remainingIndexes;

    for (int i = 0; i < validToolCalls.size(); ++i) {
        const ToolCall& call = validToolCalls[i];
        if (m_preExecutedResults.contains(call.id)) {
            ToolResult cachedResult = m_preExecutedResults.take(call.id);
            if (!cachedResult.success) {
                // Pre-execution may have failed due to incomplete streaming args;
                // re-execute with complete final arguments
                remainingToolCalls.append(call);
                remainingIndexes.append(i);
                results.append(ToolResult(call.id, false, QStringLiteral("__placeholder__")));
            } else {
                results.append(cachedResult);
            }
        } else {
            remainingToolCalls.append(call);
            remainingIndexes.append(i);
            results.append(ToolResult(call.id, false, QStringLiteral("__placeholder__")));
        }
    }

    if (!remainingToolCalls.isEmpty()) {
        QVector<ToolResult> executed = m_executor->executeBatch(remainingToolCalls);
        for (int j = 0; j < executed.size(); ++j) {
            results[remainingIndexes[j]] = executed[j];
        }
    }

    // 处理结果
    int failedCount = 0;
    QStringList failureSummaries;
    for (int i = 0; i < results.size(); ++i) {
        const ToolResult& result = results[i];
        const ToolCall& toolCall = validToolCalls[i];
        
        LOG_INFO(QStringLiteral("Tool executed: %1, success=%2").arg(toolCall.name, result.success ? "true" : "false"));

        // 标记工具失败
        if (!result.success) {
            m_hadToolFailure = true;
            ++failedCount;
            const QString compact = result.content.simplified();
            failureSummaries.append(QStringLiteral("%1: %2")
                                        .arg(toolCall.name,
                                             compact.left(120) + (compact.size() > 120 ? QStringLiteral("...") : QString())));
            if (isDeterministicToolFailure(result)) {
                m_deterministicFailedToolCalls.insert(
                    buildToolCallSignature(toolCall),
                    deterministicFailureSummary(result));
            }
        }

        const QString toolContent = result.content.trimmed().isEmpty()
            ? (result.success ? QStringLiteral("Tool executed successfully") : QStringLiteral("Tool execution failed"))
            : result.content;
        Message toolMsg(MessageRole::Tool, toolContent);
        toolMsg.toolCallId = toolCall.id;
        toolMsg.name = toolCall.name;
        toolMsg.metadata = result.metadata;
        m_messages.append(toolMsg);

        if (toolCall.name == QStringLiteral("read_file")) {
            const QString recentFilePath = result.metadata.value(QStringLiteral("recent_file_path")).toString();
            if (!recentFilePath.isEmpty()) {
                recordRecentFile(recentFilePath);
            }
        }

        if (toolCall.name == QStringLiteral("compact") && result.success) {
            const QString focus = toolCall.arguments.value(QStringLiteral("focus")).toString();
            const bool compacted = performFullCompact(QStringLiteral("manual"), focus);
            if (!compacted) {
                Message compactError(MessageRole::System, QStringLiteral("Compact failed; keep current history unchanged."));
                m_messages.append(compactError);
            }
        }

        if (m_memory) {
            m_memory->addMessage(toolMsg);
        }
    }

    emit conversationUpdated(m_messages);

    if (!results.isEmpty() && failedCount == results.size()) {
        bool authFailure = true;
        for (const ToolResult& result : results) {
            authFailure = authFailure && isAuthLikeToolFailure(result.content);
        }
        if (authFailure) {
            appendRecoveryMessage(QStringLiteral("检测到工具鉴权失败，已停止自动重试。"));
            finalizeResponse(QStringLiteral("工具调用鉴权失败（例如 API Key 无效/过期）。请先更新工具相关凭据后重试。"));
            return;
        }
    }
 
    if (!results.isEmpty() && failedCount == results.size()) {
        ++m_consecutiveToolFailureRounds;
    } else if (failedCount == 0) {
        m_consecutiveToolFailureRounds = 0;
    } else {
        m_consecutiveToolFailureRounds = qMax(0, m_consecutiveToolFailureRounds - 1);
    }

    if (m_consecutiveToolFailureRounds >= 2) {
        const QString details = failureSummaries.isEmpty()
            ? QStringLiteral("未知错误")
            : failureSummaries.mid(0, 3).join(QStringLiteral(" | "));
        appendRecoveryMessage(QStringLiteral("工具连续失败，已停止自动重试。"));
        finalizeResponse(QStringLiteral("工具持续调用失败，已停止继续调用。\n请检查网络、权限或工具参数后重试。\n最近失败：%1").arg(details));
        return;
    }

    m_pendingToolCalls.clear();
    m_currentThought.clear();
    m_currentContent.clear();
    m_currentReasoningContent.clear();
    m_postToolRecoveryStage = 0;
    const quint64 runGeneration = m_runGeneration;
    QTimer::singleShot(100, this, [this, runGeneration]() {
        if (m_stopped || runGeneration != m_runGeneration) {
            return;
        }
        processIteration();
    });
}

bool ReactAgentCore::shouldTerminateByNoToolStreak() const {
    return m_noToolIterationStreak >= constants::MAX_NO_TOOL_STREAK;
}

bool ReactAgentCore::shouldContinueForInterimContent(const QString& text) const {
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    static const QRegularExpression continuationRegex(
        QStringLiteral("(?i)(继续查看|继续读取|继续调用|继续搜索|next,? I will use|let me call|let me search)"));
    if (continuationRegex.match(trimmed).hasMatch()) {
        return true;
    }

    if (trimmed.endsWith(QStringLiteral("："))
        || trimmed.endsWith(QStringLiteral(":"))
        || trimmed.endsWith(QStringLiteral("..."))
        || trimmed.endsWith(QStringLiteral("…"))) {
        return true;
    }

    return false;
}

bool ReactAgentCore::isLikelyTruncatedAssistantTurn(const QString& text) const {
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    const int fenceCount = trimmed.count(QStringLiteral("```"));
    if ((fenceCount % 2) != 0) {
        return true;
    }

    if (trimmed.endsWith(QStringLiteral("，"))
        || trimmed.endsWith(QStringLiteral(","))
        || trimmed.endsWith(QStringLiteral("、"))
        || trimmed.endsWith(QStringLiteral("（"))
        || trimmed.endsWith(QStringLiteral("("))
        || trimmed.endsWith(QStringLiteral("例如"))
        || trimmed.endsWith(QStringLiteral("比如"))
        || trimmed.endsWith(QStringLiteral("包括"))
        || trimmed.endsWith(QStringLiteral("如下"))
        || trimmed.endsWith(QStringLiteral("-"))) {
        return true;
    }

    // 长文本但结尾无自然终止符，通常是 hit max_tokens 的截断形态。
    if (trimmed.size() >= 360) {
        const QChar tail = trimmed.back();
        const QString endings = QStringLiteral(".!?。！？”’】]}");
        if (!endings.contains(tail)) {
            return true;
        }
    }

    return false;
}

bool ReactAgentCore::hasFinalAnswerMarker(const QString& text) const {
    static const QRegularExpression markerRegex(
        QStringLiteral("(?i)\\bfinal[_\\s-]*answer\\b|\\bfinal[_\\s-]*response\\b"));
    return markerRegex.match(text).hasMatch();
}

bool ReactAgentCore::hasRecentToolResult() const {
    for (auto it = m_messages.crbegin(); it != m_messages.crend(); ++it) {
        if (it->role == MessageRole::Assistant && !it->content.trimmed().isEmpty()) {
            return false;
        }
        if (it->role == MessageRole::Tool && !it->content.trimmed().isEmpty()) {
            return true;
        }
    }

    return false;
}

bool ReactAgentCore::hasRecentToolNamed(const QString& toolName, int maxLookback) const {
    const QString normalized = toolName.trimmed().toLower();
    if (normalized.isEmpty()) {
        return false;
    }

    int scanned = 0;
    for (auto it = m_messages.crbegin(); it != m_messages.crend() && scanned < maxLookback; ++it, ++scanned) {
        if (it->role == MessageRole::Assistant && !it->content.trimmed().isEmpty()) {
            break;
        }
        if (it->role == MessageRole::Tool && it->name.trimmed().toLower() == normalized) {
            return true;
        }
    }
    return false;
}

QString ReactAgentCore::buildRecentToolResultSummary(int maxItems, int maxCharsPerItem) const {
    QStringList snippets;
    for (auto it = m_messages.crbegin(); it != m_messages.crend(); ++it) {
        if (it->role != MessageRole::Tool) {
            if (!snippets.isEmpty() && it->role == MessageRole::Assistant) {
                break;
            }
            continue;
        }

        QString snippet = it->content.simplified();
        if (snippet.isEmpty()) {
            continue;
        }
        if (snippet.size() > maxCharsPerItem) {
            snippet = snippet.left(maxCharsPerItem) + QStringLiteral("...");
        }
        if (!it->name.trimmed().isEmpty()) {
            snippet = QStringLiteral("%1: %2").arg(it->name, snippet);
        }
        snippets.prepend(snippet);
        if (snippets.size() >= maxItems) {
            break;
        }
    }

    return snippets.join(QStringLiteral(" | "));
}

void ReactAgentCore::finalizeResponse(const QString& finalMessage) {
    QString normalizedMessage = stripInternalNoise(finalMessage);
    if (normalizedMessage.isEmpty()) {
        const QString latestToolSnippet = buildRecentToolResultSummary();
        if (!latestToolSnippet.isEmpty()) {
            normalizedMessage = QStringLiteral("模型未继续总结工具结果。最近工具结果：%1").arg(latestToolSnippet);
        } else {
            normalizedMessage = QStringLiteral("（模型返回空响应）");
        }
    }
    // 统一在收尾阶段落地 assistant 消息，保证 UI 与 memory 视图一致。
    Message assistantMsg(MessageRole::Assistant, normalizedMessage);
    assistantMsg.reasoningContent = m_currentReasoningContent;
    m_messages.append(assistantMsg);

    if (m_memory) {
        m_memory->addMessage(assistantMsg);
    }

    emit conversationUpdated(m_messages);
    emit completed(normalizedMessage);
}

void ReactAgentCore::compressMessagesForContext(MessageList& messages) const {
    applyMicroCompact(messages);
}

bool ReactAgentCore::protectLatestToolBatch(const MessageList& messages, QSet<int>* protectedIndexes) const {
    if (!protectedIndexes) {
        return false;
    }
    protectedIndexes->clear();

    int lastAssistantToolCallIndex = -1;
    QSet<QString> expectedToolIds;
    for (int i = messages.size() - 1; i >= 0; --i) {
        const Message& message = messages.at(i);
        if (message.role == MessageRole::Assistant && !message.toolCalls.isEmpty()) {
            lastAssistantToolCallIndex = i;
            for (const ToolCallData& call : message.toolCalls) {
                const QString id = call.id.trimmed();
                if (!id.isEmpty()) {
                    expectedToolIds.insert(id);
                }
            }
            break;
        }
    }

    if (lastAssistantToolCallIndex < 0) {
        return false;
    }

    protectedIndexes->insert(lastAssistantToolCallIndex);
    for (int i = lastAssistantToolCallIndex + 1; i < messages.size(); ++i) {
        const Message& message = messages.at(i);
        if (message.role != MessageRole::Tool) {
            continue;
        }
        if (expectedToolIds.isEmpty() || expectedToolIds.contains(message.toolCallId.trimmed())) {
            protectedIndexes->insert(i);
        }
    }
    return !protectedIndexes->isEmpty();
}

QString ReactAgentCore::buildToolPlaceholder(const Message& toolMessage) const {
    QString toolName = toolMessage.name.trimmed();
    if (toolName.isEmpty()) {
        toolName = QStringLiteral("tool");
    }
    QString head = toolMessage.content.simplified();
    if (head.size() > constants::MICRO_COMPACT_TOOL_HEAD_CHARS) {
        head = head.left(constants::MICRO_COMPACT_TOOL_HEAD_CHARS) + QStringLiteral("...");
    }
    return QStringLiteral("[旧工具结果已折叠：%1。摘要：%2]")
        .arg(toolName, head);
}

bool ReactAgentCore::applyMicroCompact(MessageList& messages) const {
    if (messages.isEmpty()) {
        return false;
    }

    QVector<int> toolIndexes;
    for (int i = 0; i < messages.size(); ++i) {
        if (messages.at(i).role == MessageRole::Tool) {
            toolIndexes.append(i);
        }
    }
    if (toolIndexes.size() <= constants::MICRO_COMPACT_KEEP_TOOL_RESULTS) {
        return false;
    }

    QSet<int> protectedIndexes;
    protectLatestToolBatch(messages, &protectedIndexes);

    QSet<int> keepIndexes = protectedIndexes;
    int kept = 0;
    for (int i = toolIndexes.size() - 1; i >= 0 && kept < constants::MICRO_COMPACT_KEEP_TOOL_RESULTS; --i) {
        const int index = toolIndexes.at(i);
        if (keepIndexes.contains(index)) {
            continue;
        }
        keepIndexes.insert(index);
        ++kept;
    }

    bool changed = false;
    for (int index : toolIndexes) {
        if (keepIndexes.contains(index)) {
            continue;
        }
        Message& toolMessage = messages[index];
        const QString placeholder = buildToolPlaceholder(toolMessage);
        if (toolMessage.content != placeholder) {
            toolMessage.content = placeholder;
            changed = true;
        }
    }
    return changed;
}

int ReactAgentCore::estimateTokens(const QString& text) const {
    int hanChars = 0;
    int latinOrDigitChars = 0;
    int otherVisibleChars = 0;
    for (const QChar& c : text) {
        if (c.script() == QChar::Script_Han) {
            ++hanChars;
        } else if (c.isLetterOrNumber()) {
            ++latinOrDigitChars;
        } else if (!c.isSpace() && !c.isPunct()) {
            ++otherVisibleChars;
        }
    }
    const int latinTokens = (latinOrDigitChars + 3) / 4;
    const int otherTokens = (otherVisibleChars + 1) / 2;
    return qMax(1, hanChars + latinTokens + otherTokens);
}

QString ReactAgentCore::archiveMessagesBeforeCompact() const {
    QDir dir(compactArchiveDirPath(m_sessionId));
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    const QString timestamp = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz"));
    const QString filePath = dir.filePath(QStringLiteral("compact_%1_%2.json")
        .arg(timestamp)
        .arg(m_compactState.compactCount + 1));

    QJsonArray messagesArray;
    for (const Message& message : m_messages) {
        messagesArray.append(message.toJson());
    }

    QJsonObject root;
    root["created_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    root["provider_model"] = m_providerConfig.model;
    root["messages"] = messagesArray;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        LOG_WARN(QStringLiteral("Failed to archive compact source messages: %1").arg(file.errorString()));
        return QString();
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();
    return QDir::toNativeSeparators(filePath);
}

QString ReactAgentCore::buildCompactSummaryRecentFilesSuffix() const {
    if (m_compactState.recentFiles.isEmpty()) {
        return QStringLiteral("\nRecentFiles: []");
    }
    return QStringLiteral("\nRecentFiles:\n- %1").arg(m_compactState.recentFiles.join(QStringLiteral("\n- ")));
}

QString ReactAgentCore::buildFullCompactPrompt(const MessageList& messages, const QString& focus) const {
    QStringList lines;
    lines.append(QStringLiteral("请将下面整段对话压缩成结构化摘要。"));
    lines.append(QStringLiteral("必须保留以下五部分，并使用这五个中文标题："));
    lines.append(QStringLiteral("1. 当前目标"));
    lines.append(QStringLiteral("2. 重要发现与决策"));
    lines.append(QStringLiteral("3. 读过和修改过的文件"));
    lines.append(QStringLiteral("4. 剩余工作"));
    lines.append(QStringLiteral("5. 用户约束"));
    lines.append(QStringLiteral("要求："));
    lines.append(QStringLiteral("- 摘要要足够让后续模型无缝继续工作。"));
    lines.append(QStringLiteral("- 不要输出无关寒暄。"));
    lines.append(QStringLiteral("- 如果 focus 非空，优先保留与该 focus 相关的上下文。"));
    lines.append(QStringLiteral("- 末尾附加 RecentFiles 列表。"));
    if (!focus.trimmed().isEmpty()) {
        lines.append(QStringLiteral("Focus: %1").arg(focus.trimmed()));
    }
    lines.append(QStringLiteral("\n对话如下："));

    for (const Message& message : messages) {
        QString content = message.content;
        if (message.role == MessageRole::Tool && content.size() > 1200) {
            content = content.left(1200) + QStringLiteral("\n...[tool output truncated for compact prompt]");
        }
        lines.append(QStringLiteral("[%1]\n%2")
            .arg(messageRoleLabel(message.role), content));
    }

    lines.append(buildCompactSummaryRecentFilesSuffix());
    return lines.join(QStringLiteral("\n\n"));
}

void ReactAgentCore::replaceMessagesWithCompactSummary(const QString& summary,
                                                       const MessageList& preservedTail,
                                                       const QString& archivePath,
                                                       const QString& trigger,
                                                       const QString& focus) {
    Message summaryMessage(
        MessageRole::System,
        QStringLiteral("[Compact Summary]\n%1%2")
            .arg(summary.trimmed(), buildCompactSummaryRecentFilesSuffix()));

    MessageList compacted;
    compacted.append(summaryMessage);
    compacted.append(preservedTail);
    m_messages = compacted;

    CompactRecord record;
    record.index = m_compactState.compactCount + 1;
    record.trigger = trigger;
    record.focus = focus;
    record.archivePath = archivePath;
    record.summary = summary.trimmed();
    record.recentFiles = m_compactState.recentFiles;
    record.createdAt = QDateTime::currentDateTimeUtc();

    m_compactState.compactCount = record.index;
    m_compactState.lastSummary = record.summary;
    m_compactState.lastArchivePath = archivePath;
    m_compactState.history.append(record);
    while (m_compactState.history.size() > 20) {
        m_compactState.history.removeFirst();
    }
    saveCompactState();
}

void ReactAgentCore::recordRecentFile(const QString& path) {
    const QString normalized = QDir::toNativeSeparators(path.trimmed());
    if (normalized.isEmpty()) {
        return;
    }
    m_compactState.recentFiles.removeAll(normalized);
    m_compactState.recentFiles.append(normalized);
    while (m_compactState.recentFiles.size() > constants::COMPACT_RECENT_FILES_LIMIT) {
        m_compactState.recentFiles.removeFirst();
    }
    saveCompactState();
}

void ReactAgentCore::loadCompactState() {
    QFile file(compactStateFilePath(m_sessionId));
    if (!file.exists()) {
        return;
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return;
    }
    m_compactState = CompactState::fromJson(doc.object());
}

void ReactAgentCore::saveCompactState() const {
    QDir dir(compactStateRootPath(m_sessionId));
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }
    QFile file(compactStateFilePath(m_sessionId));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return;
    }
    file.write(QJsonDocument(m_compactState.toJson()).toJson(QJsonDocument::Indented));
    file.close();
}

bool ReactAgentCore::performFullCompact(QString trigger, const QString& focus) {
    if (!m_provider) {
        return false;
    }

    const QString archivePath = archiveMessagesBeforeCompact();
    const MessageList sourceMessages = m_messages;
    QSet<int> protectedIndexes;
    protectLatestToolBatch(sourceMessages, &protectedIndexes);

    MessageList summarySource;
    MessageList preservedTail;
    for (int i = 0; i < sourceMessages.size(); ++i) {
        if (protectedIndexes.contains(i)) {
            preservedTail.append(sourceMessages.at(i));
        } else {
            summarySource.append(sourceMessages.at(i));
        }
    }

    const QString prompt = buildFullCompactPrompt(summarySource, focus);

    MessageList summaryMessages;
    summaryMessages.append(Message(MessageRole::User, prompt));

    ChatOptions options;
    options.model = m_providerConfig.model;
    options.temperature = 0.2;
    options.maxTokens = constants::COMPACT_SUMMARY_MAX_TOKENS;
    options.stream = false;

    QString summary;
    QString errorText;
    QEventLoop loop;

    QPointer<ILLMProvider> provider(m_provider);
    auto startCompactRequest = [this, provider, summaryMessages, options, &summary, &errorText, &loop]() {
        if (!provider) {
            errorText = QStringLiteral("Provider unavailable during compact");
            loop.quit();
            return;
        }

        provider->chat(summaryMessages, options,
            [this, &summary, &loop](const QString& response) {
                auto finish = [&summary, &loop, response]() {
                    summary = response.trimmed();
                    loop.quit();
                };
                if (this->thread() == QThread::currentThread()) {
                    finish();
                } else {
                    QMetaObject::invokeMethod(this, finish, Qt::QueuedConnection);
                }
            },
            [this, &errorText, &loop](const QString& error) {
                auto fail = [&errorText, &loop, error]() {
                    errorText = error;
                    loop.quit();
                };
                if (this->thread() == QThread::currentThread()) {
                    fail();
                } else {
                    QMetaObject::invokeMethod(this, fail, Qt::QueuedConnection);
                }
            });
    };

    if (provider->thread() == QThread::currentThread()) {
        startCompactRequest();
    } else {
        QMetaObject::invokeMethod(provider, startCompactRequest, Qt::QueuedConnection);
    }
    loop.exec();

    if (summary.isEmpty()) {
        LOG_WARN(QStringLiteral("Full compact summary failed: %1").arg(errorText));
        return false;
    }

    replaceMessagesWithCompactSummary(summary, preservedTail, archivePath, trigger, focus);
    emit conversationUpdated(m_messages);
    return true;
}

// ============================================================================
// 7 种继续策略实现
// ============================================================================

ContinueReason ReactAgentCore::evaluateContinueReason() {
    // 1. 用户主动停止
    if (m_stopped) {
        return ContinueReason::UserStop;
    }
    
    // 2. 达到最大迭代次数
    if (m_iteration >= constants::MAX_ITERATIONS) {
        return ContinueReason::MaxIterationsReached;
    }
    
    // 3. 上下文窗口超限
    int contextTokens = estimateContextTokens();
    const int threshold = contextCompressionThreshold(m_providerConfig);
    if (contextTokens > threshold) {  // 60% 阈值
        return ContinueReason::ContextOverflow;
    }
    
    // 4. 工具执行失败
    if (m_hadToolFailure) {
        return ContinueReason::ToolExecutionFailure;
    }
    
    // 5. 连续无工具调用
    if (shouldTerminateByNoToolStreak()) {
        return ContinueReason::NoToolStreak;
    }
    
    // 6. 正常完成
    return ContinueReason::NormalCompletion;
}

void ReactAgentCore::handleContinueReason(ContinueReason reason) {
    switch (reason) {
        case ContinueReason::UserStop:
            LOG_INFO(QStringLiteral("ReactAgentCore stopped by user at iteration %1").arg(m_iteration));
            finalizeResponse(m_currentContent.isEmpty() ? m_currentThought : m_currentContent);
            break;
            
        case ContinueReason::MaxIterationsReached:
            LOG_WARN(QStringLiteral("ReactAgentCore reached max iterations: %1").arg(constants::MAX_ITERATIONS));
            appendRecoveryMessage(QStringLiteral("已达到最大思考步骤，基于当前结果总结回答。"));
            finalizeResponse(m_currentContent.isEmpty() ? m_currentThought : m_currentContent);
            break;
            
        case ContinueReason::ContextOverflow:
            LOG_WARN(QStringLiteral("ReactAgentCore context overflow detected, compressing..."));
            {
                const int threshold = contextCompressionThreshold(m_providerConfig);
                const int beforeTokens = estimateContextTokens();
                const bool microChanged = applyMicroCompact(m_messages);
                const int afterMicroTokens = estimateContextTokens();
                const bool enoughAfterMicro = afterMicroTokens <= threshold;

                if (!enoughAfterMicro) {
                    const bool fullCompacted = performFullCompact(QStringLiteral("auto"));
                    const int afterFullTokens = estimateContextTokens();
                    m_lastContextTokens = afterFullTokens;
                    if (!fullCompacted || afterFullTokens >= beforeTokens || afterFullTokens > threshold) {
                        LOG_WARN(QStringLiteral("ReactAgentCore compact plateaued: before=%1 after=%2 threshold=%3")
                                     .arg(beforeTokens)
                                     .arg(afterFullTokens)
                                     .arg(threshold));
                        appendRecoveryMessage(QStringLiteral("[上下文过长，已压缩到极限，请清空部分历史后重试。]"));
                        emit conversationUpdated(m_messages);
                        finalizeResponse(m_currentContent.isEmpty()
                            ? QStringLiteral("上下文过长，已压缩到极限，请清空部分历史后重试。")
                            : m_currentContent);
                        break;
                    }
                } else {
                    m_lastContextTokens = afterMicroTokens;
                }

                if (microChanged || m_lastContextTokens < beforeTokens) {
                    appendRecoveryMessage(QStringLiteral("[系统已压缩上下文，继续处理...]"));
                    emit conversationUpdated(m_messages);
                }
            }
            processIteration();
            break;
            
        case ContinueReason::ToolExecutionFailure:
            LOG_WARN(QStringLiteral("ReactAgentCore tool failure detected at iteration %1").arg(m_iteration));
            if (canRetryAfterError()) {
                m_hadToolFailure = false;  // 重置标志
                m_retryCount++;
                appendRecoveryMessage(QStringLiteral("工具执行失败。不要用完全相同的工具参数盲重试；请更换参数、改用别的工具，或基于已有结果直接回答。"));
                emit conversationUpdated(m_messages);
                // 继续下一轮迭代，让模型尝试其他方案
                processIteration();
            } else {
                LOG_WARN(QStringLiteral("ReactAgentCore max retries reached, finalizing"));
                appendRecoveryMessage(QStringLiteral("多次尝试后仍有错误，基于当前结果总结回答。"));
                finalizeResponse(m_currentContent.isEmpty() ? m_currentThought : m_currentContent);
            }
            break;
            
        case ContinueReason::NoToolStreak:
            LOG_INFO(QStringLiteral("ReactAgentCore no-tool streak termination at iteration %1").arg(m_iteration));
            finalizeResponse(m_currentContent.isEmpty() ? m_currentThought : m_currentContent);
            break;
            
        case ContinueReason::NormalCompletion:
            // 正常完成，继续循环（如有工具调用）或结束（如有最终答案）
            break;
            
        case ContinueReason::StreamError:
            LOG_ERROR(QStringLiteral("ReactAgentCore stream error at iteration %1").arg(m_iteration));
            if (canRetryAfterError()) {
                m_retryCount++;
                appendRecoveryMessage(QStringLiteral("[网络错误，正在重试...]"));
                emit conversationUpdated(m_messages);
                // 重试当前迭代
                processIteration();
            } else {
                LOG_ERROR(QStringLiteral("ReactAgentCore max retries reached after stream error"));
                finalizeResponse(QStringLiteral("抱歉，由于网络问题多次重试失败，请稍后再试。"));
            }
            break;
    }
}

bool ReactAgentCore::canRetryAfterError() const {
    constexpr int MAX_RETRIES = 3;
    return m_retryCount < MAX_RETRIES;
}

void ReactAgentCore::appendRecoveryMessage(const QString& message) {
    Message sysMsg(MessageRole::System, message);
    sysMsg.metadata[QStringLiteral("internal")] = true;
    sysMsg.metadata[QStringLiteral("ui_hidden")] = true;
    m_messages.append(sysMsg);
    if (m_memory) {
        m_memory->addMessage(sysMsg);
    }
}

int ReactAgentCore::estimateContextTokens() const {
    int total = 0;
    total += estimateTokens(m_systemPrompt);
    for (const Message& msg : m_messages) {
        total += estimateTokens(msg.content);
        // 工具调用也占用 tokens
        for (const ToolCallData& tc : msg.toolCalls) {
            total += estimateTokens(tc.name);
            total += estimateTokens(tc.arguments);
        }
    }
    return total;
}

// ============================================================================
// 流式工具预执行实现
// ============================================================================

bool ReactAgentCore::isToolCallComplete(const CorePendingToolCall& pending) const {
    if (pending.id.isEmpty() || pending.name.isEmpty()) {
        return false;
    }
    
    // 尝试解析 JSON 参数，如果解析成功则认为完整
    QJsonParseError parseError;
    QJsonDocument::fromJson(pending.arguments.toUtf8(), &parseError);
    return parseError.error == QJsonParseError::NoError;
}

ToolCall ReactAgentCore::createToolCall(const CorePendingToolCall& pending) const {
    ToolCall toolCall;
    toolCall.id = pending.id;
    toolCall.name = pending.name;
    
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(pending.arguments.toUtf8(), &parseError);
    
    if (parseError.error == QJsonParseError::NoError && document.isObject()) {
        toolCall.arguments = document.object();
    } else {
        // 如果解析失败，创建空对象（后续执行时会报错）
        toolCall.arguments = QJsonObject();
    }
    
    return toolCall;
}

void ReactAgentCore::startToolPreExecution() {
    if (!m_executor) {
        return;
    }
    
    // 为所有完整的 pending tool calls 启动异步预执行
    for (const CorePendingToolCall& pending : m_pendingToolCalls) {
        if (!isToolCallComplete(pending)) {
            continue;
        }
        
        // 如果已经在预执行中或已有结果缓存，跳过
        if (m_preExecutingTools.contains(pending.id) || m_preExecutedResults.contains(pending.id)) {
            continue;
        }
        
        ToolCall toolCall = createToolCall(pending);
        
        // 启动异步工具执行（非阻塞）
        QFuture<ToolResult> future = QtConcurrent::run([this, toolCall]() {
            return m_executor->execute(toolCall);
        });
        
        m_preExecutingTools[pending.id] = future;
        emit toolExecutionStarted(pending.id);
        
        LOG_INFO(QStringLiteral("ReactAgentCore: Pre-executing tool '%1' (id: %2)")
            .arg(toolCall.name, toolCall.id));
    }
}

void ReactAgentCore::checkPreExecutionResults() {
    // 仅处理“当前仍待执行”的工具，避免无关预执行任务阻塞当前轮次。
    QSet<QString> relevantToolIds;
    for (const CorePendingToolCall& pending : m_pendingToolCalls) {
        if (isToolCallComplete(pending) && !pending.id.isEmpty()) {
            relevantToolIds.insert(pending.id);
        }
    }

    QStringList completedIds;
    for (auto it = m_preExecutingTools.begin(); it != m_preExecutingTools.end(); ++it) {
        const QString& toolId = it.key();
        QFuture<ToolResult>& future = it.value();
        if (!relevantToolIds.contains(toolId)) {
            // 无关工具不等待，若已完成则顺手清理，防止 map 膨胀。
            if (future.isFinished()) {
                completedIds.append(toolId);
            }
            continue;
        }

        if (future.isFinished()) {
            completedIds.append(toolId);
            m_preExecutedResults[toolId] = future.result();
            emit toolPreExecutionComplete(toolId);
            LOG_INFO(QStringLiteral("ReactAgentCore: Tool pre-execution completed for '%1'").arg(toolId));
        } else {
            // 等待未完成的工具（阻塞当前线程）
            future.waitForFinished();
            completedIds.append(toolId);
            m_preExecutedResults[toolId] = future.result();
            emit toolPreExecutionComplete(toolId);
            LOG_INFO(QStringLiteral("ReactAgentCore: Tool pre-execution waited and completed for '%1'").arg(toolId));
        }
    }
    
    // 清理已完成的预执行记录
    for (const QString& id : completedIds) {
        m_preExecutingTools.remove(id);
    }
}

}
