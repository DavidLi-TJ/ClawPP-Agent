#include "subagent_tool.h"

namespace clawpp {

SubagentTool::SubagentTool(QObject* parent)
    : ITool(parent) {}

QString SubagentTool::name() const {
    return "start_subagent";
}

QString SubagentTool::description() const {
    return "Spawn a delegated sub-agent for an isolated task and return its result";
}

QJsonObject SubagentTool::parameters() const {
    QJsonObject params;
    params["type"] = "object";

    QJsonObject properties;

    QJsonObject agentType;
    agentType["type"] = "string";
    agentType["description"] = "The kind of sub-agent to create, for example analysis or review";
    agentType["default"] = "analysis";
    properties["agent_type"] = agentType;

    QJsonObject task;
    task["type"] = "string";
    task["description"] = "The task delegated to the sub-agent";
    properties["task"] = task;

    QJsonObject context;
    context["type"] = "string";
    context["description"] = "Additional context passed to the sub-agent";
    properties["context"] = context;

    QJsonObject goal;
    goal["type"] = "string";
    goal["description"] = "Desired output format or success criteria";
    properties["goal"] = goal;

    QJsonObject maxIterations;
    maxIterations["type"] = "integer";
    maxIterations["description"] = "Requested iteration limit for the sub-agent";
    maxIterations["default"] = 8;
    properties["max_iterations"] = maxIterations;

    params["properties"] = properties;
    params["required"] = QJsonArray{"task"};
    return params;
}

PermissionLevel SubagentTool::permissionLevel() const {
    return PermissionLevel::Moderate;
}

bool SubagentTool::canRunInParallel() const {
    // 子代理通常是远程 I/O 密集型任务，允许批量并行以支持工作流分支执行。
    return true;
}

ToolResult SubagentTool::execute(const QJsonObject& args) {
    ToolResult result;
    result.success = false;
    // 真实执行在 ToolExecutor::executeSubagent，工具本体仅保留 schema 与说明。
    result.content = QStringLiteral("Subagent execution is handled by ToolExecutor.");
    Q_UNUSED(args);
    return result;
}

}
