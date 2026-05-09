#include "tool_executor.h"
#include "tool_registry.h"
#include "i_tool.h"
#include "permission/permission_manager.h"
#include "permission/bash_analyzer.h"
#include "agent/react_agent_core.h"
#include "common/constants.h"

#include <QEventLoop>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QSharedPointer>
#include <QTimer>
#include <QStandardPaths>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>

namespace {

thread_local int g_blockingLoopDepth = 0;

class ScopedBlockingLoopDepth {
public:
    ScopedBlockingLoopDepth() {
        ++g_blockingLoopDepth;
    }

    ~ScopedBlockingLoopDepth() {
        --g_blockingLoopDepth;
    }
};

}

namespace clawpp {

namespace {
constexpr int MAX_TOOL_OUTPUT_CHARS = 100000;
constexpr int SUMMARY_HEAD_CHARS = 320;
constexpr int SUMMARY_TAIL_CHARS = 160;

QString firstNonEmptyString(const QJsonObject& args, const QStringList& keys) {
    for (const QString& key : keys) {
        const QString value = args.value(key).toString().trimmed();
        if (!value.isEmpty()) {
            return value;
        }
    }
    return QString();
}

void normalizeToolCallArguments(ToolCall* call) {
    if (!call) {
        return;
    }

    const QString toolName = call->name.trimmed().toLower();
    if (toolName.isEmpty()) {
        return;
    }

    if (call->path.trimmed().isEmpty()) {
        const QString pathFromArgs = call->arguments.value(QStringLiteral("path")).toString().trimmed();
        if (!pathFromArgs.isEmpty()) {
            call->path = pathFromArgs;
        }
    }

    const bool pathSensitiveTool =
        toolName == QStringLiteral("read_file")
        || toolName == QStringLiteral("write_file")
        || toolName == QStringLiteral("edit_file")
        || toolName == QStringLiteral("list_directory")
        || toolName == QStringLiteral("search_files")
        || toolName == QStringLiteral("filesystem");

    if (pathSensitiveTool) {
        QString path = call->arguments.value(QStringLiteral("path")).toString().trimmed();
        if (path.isEmpty()) {
            path = firstNonEmptyString(call->arguments, {
                QStringLiteral("file"),
                QStringLiteral("file_path"),
                QStringLiteral("filepath"),
                QStringLiteral("directory"),
                QStringLiteral("dir"),
                QStringLiteral("folder"),
                QStringLiteral("target"),
                QStringLiteral("target_path"),
                QStringLiteral("source"),
                QStringLiteral("source_path")
            });
        }
        if (path.isEmpty()) {
            path = call->path.trimmed();
        }
        if (path.isEmpty() && call->arguments.size() == 1) {
            const QJsonObject::const_iterator it = call->arguments.constBegin();
            path = it.value().toString().trimmed();
        }
        if (path.isEmpty() &&
            (toolName == QStringLiteral("list_directory")
             || toolName == QStringLiteral("search_files"))) {
            path = QDir::currentPath();
            call->arguments.insert(QStringLiteral("_path_defaulted"), true);
        }
        if (!path.isEmpty()) {
            call->arguments.insert(QStringLiteral("path"), path);
            call->path = path;
        }
    }

    if (toolName == QStringLiteral("shell")) {
        QString command = call->arguments.value(QStringLiteral("command")).toString().trimmed();
        if (command.isEmpty()) {
            command = firstNonEmptyString(call->arguments, {
                QStringLiteral("cmd"),
                QStringLiteral("script"),
                QStringLiteral("input"),
                QStringLiteral("value")
            });
        }
        if (command.isEmpty() && call->arguments.size() == 1) {
            const QJsonObject::const_iterator it = call->arguments.constBegin();
            command = it.value().toString().trimmed();
        }
        if (!command.isEmpty()) {
            call->arguments.insert(QStringLiteral("command"), command);
        }
    }
}
}

ToolExecutor::ToolExecutor(ToolRegistry* registry,
                           PermissionManager* permissionManager,
                           QObject* parent)
    : QObject(parent)
    , m_registry(registry)
    , m_permissionManager(permissionManager)
    , m_provider(nullptr)
    , m_autoApproveConfirmations(false) {}

void ToolExecutor::setRegistry(ToolRegistry* registry) {
    m_registry = registry;
}

void ToolExecutor::setPermissionManager(PermissionManager* manager) {
    m_permissionManager = manager;
}

void ToolExecutor::setProvider(ILLMProvider* provider) {
    m_provider = provider;
}

void ToolExecutor::setProviderConfig(const ProviderConfig& config) {
    m_providerConfig = config;
}

void ToolExecutor::setSessionId(const QString& sessionId) {
    m_sessionId = sessionId;
}

QString ToolExecutor::sessionId() const {
    return m_sessionId;
}

void ToolExecutor::setAutoApproveConfirmations(bool enabled) {
    m_autoApproveConfirmations = enabled;
}

ToolResult ToolExecutor::execute(const ToolCall& toolCall) {
    if (toolCall.name == QStringLiteral("start_subagent")) {
        ToolResult subagentResult = executeSubagent(toolCall);
        emit toolCompleted(toolCall, subagentResult);
        return subagentResult;
    }

    if (!m_registry) {
        return ToolResult(toolCall.id, false, QStringLiteral("Tool registry not configured"));
    }
    
ITool* tool = m_registry->getTool(toolCall.name);
    
    if (!tool) {
        const QString requested = toolCall.name.trimmed().toLower();
        if (requested == QStringLiteral("brainstorming")
            || requested == QStringLiteral("brainstorm")
            || requested == QStringLiteral("planning")
            || requested == QStringLiteral("analysis")
            || requested == QStringLiteral("creative_thinking")) {
            ToolResult result(
                toolCall.id,
                true,
                QStringLiteral("No external tool is needed for brainstorming. Do the creative thinking internally and answer the user directly."));
            result.metadata[QStringLiteral("virtual_tool")] = true;
            emit toolCompleted(toolCall, result);
            return result;
        }

        const QString available = m_registry ? m_registry->listTools().join(QStringLiteral(", ")) : QString();
        ToolResult result(toolCall.id, false, QString("Tool not found: %1").arg(toolCall.name));
        if (!available.isEmpty()) {
            result.content += QStringLiteral(". Available tools: %1").arg(available);
        }
        emit toolCompleted(toolCall, result);
        return result;
    }
    
    ToolCall reviewedCall = toolCall;
    normalizeToolCallArguments(&reviewedCall);
    bool shellRiskBlocked = false;
    if (reviewedCall.name == QStringLiteral("shell")) {
        const QString command = reviewedCall.arguments.value(QStringLiteral("command")).toString();
        if (!command.trimmed().isEmpty()) {
            BashAnalyzer analyzer;
            const QVector<SafetyIssue> issues = analyzer.analyze(command);
            const int riskLevel = analyzer.getRiskLevel(command);
            QStringList issueLines;
            for (const SafetyIssue& issue : issues) {
                issueLines.append(QStringLiteral("[%1] %2").arg(issue.severity, issue.message));
            }

            reviewedCall.arguments.insert(QStringLiteral("_risk_level"), riskLevel);
            reviewedCall.arguments.insert(QStringLiteral("_risk_summary"), issueLines.join(QStringLiteral("; ")));
            shellRiskBlocked = (riskLevel >= 4);
            reviewedCall.arguments.insert(QStringLiteral("_risk_blocked"), shellRiskBlocked);

            if (shellRiskBlocked) {
                ToolResult result(toolCall.id, false, QStringLiteral("Shell command blocked by safety policy (critical risk)"));
                result.metadata[QStringLiteral("blocked")] = true;
                result.metadata[QStringLiteral("risk_level")] = riskLevel;
                result.metadata[QStringLiteral("risk_summary")] = issueLines.join(QStringLiteral("; "));
                emit toolCompleted(reviewedCall, result);
                return result;
            }
        }
    }

    if (m_permissionManager) {
        PermissionResult permResult = m_permissionManager->checkPermission(m_sessionId, reviewedCall);
        if (permResult == PermissionResult::Allowed && tool->needsConfirmation()) {
            permResult = PermissionResult::NeedConfirmation;
        }

        if (reviewedCall.name == QStringLiteral("shell")) {
            const int riskLevel = reviewedCall.arguments.value(QStringLiteral("_risk_level")).toInt(0);
            if (riskLevel >= 2 && permResult == PermissionResult::Allowed) {
                permResult = PermissionResult::NeedConfirmation;
            }
        }
        
        if (permResult == PermissionResult::Denied) {
            ToolResult result(toolCall.id, false, QStringLiteral("Permission denied"));
            emit toolCompleted(reviewedCall, result);
            return result;
        }
        
        if (permResult == PermissionResult::NeedConfirmation) {
            if (m_autoApproveConfirmations) {
                permResult = PermissionResult::Allowed;
            }
        }

        if (permResult == PermissionResult::NeedConfirmation) {
            if (g_blockingLoopDepth > 0) {
                return ToolResult(toolCall.id,
                                  false,
                                  QStringLiteral("Permission confirmation is not supported in nested blocking execution"));
            }

            QEventLoop loop;
            QPointer<QEventLoop> loopPtr(&loop);
            QSharedPointer<bool> confirmed(new bool(false));
            QSharedPointer<bool> answered(new bool(false));

            QTimer timeoutTimer;
            timeoutTimer.setSingleShot(true);
            timeoutTimer.setInterval(constants::PERMISSION_TIMEOUT_MS);

            QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, [answered, confirmed, loopPtr]() {
                *answered = true;
                *confirmed = false;
                if (loopPtr) {
                    loopPtr->quit();
                }
            });

            timeoutTimer.start();
            emit permissionRequest(reviewedCall, [answered, confirmed, loopPtr](bool result) {
                if (*answered) {
                    return;
                }

                *confirmed = result;
                *answered = true;
                if (loopPtr) {
                    loopPtr->quit();
                }
            });

            if (!*answered) {
                ScopedBlockingLoopDepth loopGuard;
                loop.exec();
            }

            if (!*confirmed) {
                ToolResult result(toolCall.id, false, QStringLiteral("Permission denied by user"));
                if (reviewedCall.arguments.contains(QStringLiteral("_risk_level"))) {
                    result.metadata[QStringLiteral("risk_level")] = reviewedCall.arguments.value(QStringLiteral("_risk_level")).toInt();
                    result.metadata[QStringLiteral("risk_summary")] = reviewedCall.arguments.value(QStringLiteral("_risk_summary")).toString();
                }
                emit toolCompleted(reviewedCall, result);
                return result;
            }
        }
    }
    
    try {
        ToolResult result = tool->execute(reviewedCall.arguments);
        if (result.toolCallId.isEmpty()) {
            result.toolCallId = reviewedCall.id;
        }
        applyOutputLimit(result, reviewedCall);
        result.metadata[QStringLiteral("tool_name")] = reviewedCall.name;
        result.metadata[QStringLiteral("permission_level")] = static_cast<int>(tool->permissionLevel());
        result.metadata[QStringLiteral("needs_confirmation")] = tool->needsConfirmation();
        result.metadata[QStringLiteral("can_run_in_parallel")] = tool->canRunInParallel();
        if (reviewedCall.arguments.contains(QStringLiteral("_risk_level"))) {
            result.metadata[QStringLiteral("risk_level")] = reviewedCall.arguments.value(QStringLiteral("_risk_level")).toInt();
            result.metadata[QStringLiteral("risk_summary")] = reviewedCall.arguments.value(QStringLiteral("_risk_summary")).toString();
        }
        emit toolCompleted(reviewedCall, result);
        return result;
    } catch (const std::exception& e) {
        ToolResult result(toolCall.id, false, QString("Execution error: %1").arg(e.what()));
        emit toolCompleted(toolCall, result);
        return result;
    } catch (...) {
        ToolResult result(toolCall.id, false, QStringLiteral("Execution error: unknown exception"));
        emit toolCompleted(toolCall, result);
        return result;
    }
}

void ToolExecutor::applyOutputLimit(ToolResult& result, const ToolCall& toolCall) const {
    if (result.content.size() <= MAX_TOOL_OUTPUT_CHARS) {
        return;
    }

    QString cacheRoot = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (cacheRoot.isEmpty()) {
        cacheRoot = QDir::tempPath() + QStringLiteral("/clawpp");
    }
    QDir dir(cacheRoot);
    dir.mkpath(QStringLiteral("tool_cache"));
    const QString filePath = dir.filePath(QStringLiteral("tool_cache/%1_%2.txt")
        .arg(toolCall.name.isEmpty() ? QStringLiteral("tool") : toolCall.name)
        .arg(toolCall.id.isEmpty() ? QStringLiteral("unknown") : toolCall.id));

    QFile outFile(filePath);
    bool persisted = false;
    if (outFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        outFile.write(result.content.toUtf8());
        outFile.close();
        persisted = true;
    }

    const QString summary = buildOutputSummary(result.content, MAX_TOOL_OUTPUT_CHARS);
    const QString persistedText = persisted
        ? QStringLiteral("完整输出已保存到：%1").arg(QDir::toNativeSeparators(filePath))
        : QStringLiteral("完整输出保存失败，请重试或缩小读取范围。");

    result.metadata[QStringLiteral("output_truncated")] = true;
    result.metadata[QStringLiteral("output_original_size")] = result.content.size();
    result.metadata[QStringLiteral("output_limit")] = MAX_TOOL_OUTPUT_CHARS;
    result.metadata[QStringLiteral("output_cache_path")] = persisted ? QDir::toNativeSeparators(filePath) : QString();

    result.content = QStringLiteral(
        "[Tool output truncated]\n"
        "原始长度：%1 字符（上限：%2）\n"
        "%3\n\n"
        "摘要：\n%4")
        .arg(result.metadata.value(QStringLiteral("output_original_size")).toInt())
        .arg(MAX_TOOL_OUTPUT_CHARS)
        .arg(persistedText)
        .arg(summary);
}

QString ToolExecutor::buildOutputSummary(const QString& content, int maxChars) const {
    const QString normalized = content;
    if (normalized.size() <= maxChars) {
        return normalized;
    }
    const QString head = normalized.left(SUMMARY_HEAD_CHARS);
    const QString tail = normalized.right(SUMMARY_TAIL_CHARS);
    const int omitted = normalized.size() - SUMMARY_HEAD_CHARS - SUMMARY_TAIL_CHARS;
    return QStringLiteral("%1\n\n...[中间省略 %2 字符]...\n\n%3")
        .arg(head)
        .arg(qMax(0, omitted))
        .arg(tail);
}

QVector<ToolResult> ToolExecutor::executeBatch(const QVector<ToolCall>& toolCalls) {
    if (toolCalls.isEmpty()) {
        return QVector<ToolResult>();
    }

    // 分类工具：Safe（可并行）vs Risky（需串行）
    QVector<QPair<int, ToolCall>> safeTools;   // <原始索引, ToolCall>
    QVector<QPair<int, ToolCall>> riskyTools;
    
    for (int i = 0; i < toolCalls.size(); ++i) {
        const ToolCall& tc = toolCalls[i];
        
        if (!m_registry) {
            riskyTools.append(qMakePair(i, tc));
            continue;
        }
        
        ITool* tool = m_registry->getTool(tc.name);
        if (!tool || !tool->canRunInParallel()) {
            riskyTools.append(qMakePair(i, tc));
        } else {
            safeTools.append(qMakePair(i, tc));
        }
    }

    // 结果数组（按原始顺序）
    QVector<ToolResult> results(toolCalls.size());

    // 并行执行 Safe 工具
    if (!safeTools.isEmpty()) {
        QVector<QFuture<ToolResult>> futures;
        
        for (const auto& [index, tc] : safeTools) {
            QFuture<ToolResult> future = QtConcurrent::run([this, tc]() {
                return execute(tc);
            });
            futures.append(future);
        }

        // 等待所有并行任务完成
        for (int i = 0; i < safeTools.size(); ++i) {
            int originalIndex = safeTools[i].first;
            results[originalIndex] = futures[i].result();
        }
    }

    // 串行执行 Risky 工具
    for (const auto& [index, tc] : riskyTools) {
        results[index] = execute(tc);
    }

    return results;
}

QString ToolExecutor::buildSubagentPrompt(const QJsonObject& args) const {
    QString agentType = args.value("agent_type").toString("analysis");
    QString task = args.value("task").toString();
    QString context = args.value("context").toString();
    QString goal = args.value("goal").toString();

    QStringList parts;
    parts.append(QString("# Subagent Role: %1").arg(agentType));
    parts.append("You are a delegated sub-agent inside Claw++.");
    parts.append("Work independently, use tools only when necessary, and return a concise answer.");

    if (!goal.isEmpty()) {
        parts.append(QString("## Goal\n%1").arg(goal));
    }

    if (!context.isEmpty()) {
        parts.append(QString("## Context\n%1").arg(context));
    }

    parts.append(QString("## Task\n%1").arg(task));
    return parts.join("\n\n");
}

ToolResult ToolExecutor::executeSubagent(const ToolCall& toolCall) {
    ToolResult result;
    result.toolCallId = toolCall.id;

    if (g_blockingLoopDepth > 0) {
        result.success = false;
        result.content = QStringLiteral("Nested subagent execution is not supported in blocking mode");
        return result;
    }

    if (!m_provider) {
        result.success = false;
        result.content = "Subagent execution requires a configured provider";
        return result;
    }

    const QString task = toolCall.arguments.value("task").toString().trimmed();
    if (task.isEmpty()) {
        result.success = false;
        result.content = "Subagent task is required";
        return result;
    }

    ToolExecutor subExecutor(m_registry, m_permissionManager);
    subExecutor.setProvider(m_provider);
    subExecutor.setProviderConfig(m_providerConfig);
    subExecutor.setSessionId(m_sessionId.isEmpty() ? QStringLiteral("subagent") : m_sessionId + QStringLiteral("::subagent"));
    // 继承父执行器自动确认策略，避免子代理在阻塞执行中因权限确认超时被误判为用户拒绝。
    subExecutor.setAutoApproveConfirmations(m_autoApproveConfirmations);

    ReactAgentCore subCore;
    subCore.setProvider(m_provider);
    subCore.setExecutor(&subExecutor);
    subCore.setProviderConfig(m_providerConfig);
    subCore.setMemory(nullptr);
    subCore.setContext(MessageList());
    subCore.setSystemPrompt(buildSubagentPrompt(toolCall.arguments));
    if (m_registry) {
        QStringList toolNames = m_registry->listTools();
        toolNames.removeAll(QStringLiteral("start_subagent"));
        subCore.setRuntimeToolNames(toolNames);
    }

    QString finalOutput;
    QString errorOutput;
    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    timeoutTimer.setInterval(constants::SUBAGENT_TIMEOUT_MS);

    QObject::connect(&subCore, &ReactAgentCore::completed, &loop, [&](const QString& response) {
        finalOutput = response;
        loop.quit();
    });
    QObject::connect(&subCore, &ReactAgentCore::errorOccurred, &loop, [&](const QString& error) {
        errorOutput = error;
        loop.quit();
    });
    QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, [&]() {
        errorOutput = QStringLiteral("Subagent timed out");
        subCore.stop();
        if (m_provider) {
            m_provider->abort();
        }
        loop.quit();
    });

    subCore.run(task);
    timeoutTimer.start();
    if (errorOutput.isEmpty() && finalOutput.isEmpty()) {
        ScopedBlockingLoopDepth loopGuard;
        loop.exec();
    }
    timeoutTimer.stop();

    if (!errorOutput.isEmpty()) {
        result.success = false;
        result.content = QString("Subagent error: %1").arg(errorOutput);
        return result;
    }

    result.success = true;
    result.content = finalOutput.isEmpty() ? QStringLiteral("Subagent completed") : finalOutput;
    result.metadata[QStringLiteral("tool")]= QStringLiteral("start_subagent");
    result.metadata[QStringLiteral("session_id")] = m_sessionId;
    result.metadata[QStringLiteral("subagent_session_id")] = subExecutor.sessionId();
    return result;
}

}
