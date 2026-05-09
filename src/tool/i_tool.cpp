#include "i_tool.h"

namespace clawpp {

QJsonObject ITool::toJson() const {
    // 转为 OpenAI tools 所需的 function schema 结构。
    QJsonObject obj;
    obj["type"] = "function";
    QJsonObject func;
    func["name"] = name();
    func["description"] = description();
    func["parameters"] = parameters();
    func["permission_level"] = static_cast<int>(permissionLevel());
    func["needs_confirmation"] = needsConfirmation();
    func["can_run_in_parallel"] = canRunInParallel();
    obj["function"] = func;
    return obj;
}

}
