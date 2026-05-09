#ifndef CLAWPP_COMMON_MODEL_CONTEXT_LIMITS_H
#define CLAWPP_COMMON_MODEL_CONTEXT_LIMITS_H

#include <QRegularExpression>
#include <QtGlobal>

#include "common/types.h"

namespace clawpp {
namespace modelctx {

inline int parseExplicitContextHint(const QString& modelLower) {
    static const QRegularExpression hintRe(QStringLiteral("(\\d+(?:\\.\\d+)?)\\s*([km])\\b"));
    QRegularExpressionMatchIterator it = hintRe.globalMatch(modelLower);

    int best = 0;
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        bool ok = false;
        const double number = match.captured(1).toDouble(&ok);
        if (!ok || number <= 0.0) {
            continue;
        }
        const QString unit = match.captured(2);
        const int tokens = (unit == QStringLiteral("m"))
            ? static_cast<int>(number * 1000000.0)
            : static_cast<int>(number * 1000.0);
        best = qMax(best, tokens);
    }

    return best;
}

inline int inferModelContextLimit(const ProviderConfig& config) {
    const QString model = config.model.trimmed().toLower();
    const QString provider = config.type.trimmed().toLower();
    if (model.isEmpty()) {
        return qMax(128000, config.maxTokens);
    }

    const int explicitHint = parseExplicitContextHint(model);
    if (explicitHint > 0) {
        return explicitHint;
    }

    if (model.contains(QStringLiteral("claude"))) return 200000;
    if (model.contains(QStringLiteral("gemini-2.5"))) return 1000000;
    if (model.contains(QStringLiteral("gemini-1.5-pro"))) return 1000000;
    if (model.contains(QStringLiteral("gemini-1.5-flash"))) return 1000000;
    if (model.contains(QStringLiteral("gemini"))) return 128000;
    if (model.contains(QStringLiteral("gpt-5")) || model.contains(QStringLiteral("codex"))) return 256000;
    if (model.contains(QStringLiteral("gpt-4o")) || model.contains(QStringLiteral("gpt-4.1"))
        || model.contains(QStringLiteral("o1")) || model.contains(QStringLiteral("o3"))
        || model.contains(QStringLiteral("o4"))) {
        return 128000;
    }
    if (model.contains(QStringLiteral("glm-4.5-air"))) return 256000;
    if (model.contains(QStringLiteral("glm-4.5"))) return 128000;
    if (model.contains(QStringLiteral("glm-4"))) return 128000;
    if (model.contains(QStringLiteral("glm"))) return 32000;
    if (model.contains(QStringLiteral("deepseek")) || provider.contains(QStringLiteral("deepseek"))) return 1000000;
    if (model.contains(QStringLiteral("qwen-long"))) return 1000000;

    return qMax(128000, config.maxTokens);
}

} // namespace modelctx
} // namespace clawpp

#endif
