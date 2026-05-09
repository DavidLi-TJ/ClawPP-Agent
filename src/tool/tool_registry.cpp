#include "tool_registry.h"
#include "i_tool.h"

namespace clawpp {

/**
 * @brief 获取 ToolRegistry 单例实例
 * @return 工具注册表实例
 * 
 * 使用局部静态变量实现线程安全的单例模式
 */
ToolRegistry& ToolRegistry::instance() {
    static ToolRegistry instance;
    return instance;
}

/**
 * @brief 注册工具
 * @param tool 工具指针（如果为空则忽略）
 * 
 * 将工具添加到内部映射表，以工具名称为键
 */
void ToolRegistry::registerTool(ITool* tool) {
    if (tool) {
        m_tools[tool->name()] = tool;
    }
}

/**
 * @brief 注销工具
 * @param name 工具名称
 */
void ToolRegistry::unregisterTool(const QString& name) {
    m_tools.remove(name);
}

/**
 * @brief 获取工具
 * @param name 工具名称
 * @return 工具指针（不存在则返回 nullptr）
 */
ITool* ToolRegistry::getTool(const QString& name) const {
    return m_tools.value(name, nullptr);
}

/**
 * @brief 检查工具是否存在
 * @param name 工具名称
 * @return 是否存在
 */
bool ToolRegistry::hasTool(const QString& name) const {
    return m_tools.contains(name);
}

/**
 * @brief 列出所有工具名称
 * @return 工具名称列表
 */
QStringList ToolRegistry::listTools() const {
    return m_tools.keys();
}

/**
 * @brief 获取所有工具的 Schema
 * @return JSON 数组，包含所有工具的参数定义
 * 
 * 用于向 LLM 描述可用工具
 */
QJsonArray ToolRegistry::toolsSchema() const {
    QJsonArray arr;
    for (auto* tool : m_tools) {
        arr.append(tool->toJson());
    }
    return arr;
}

/**
 * @brief 获取所有工具的文本描述
 * @return 工具描述字符串
 * 
 * 用于系统提示词中描述可用工具
 */
QString ToolRegistry::toolsDescription() const {
    QString desc;
    for (auto* tool : m_tools) {
        desc += QString("- %1: %2\n").arg(tool->name(), tool->description());
    }
    return desc;
}

}
