#ifndef CLAWPP_TOOL_TOOL_REGISTRY_H
#define CLAWPP_TOOL_TOOL_REGISTRY_H

#include <QObject>
#include <QMap>
#include "common/types.h"

namespace clawpp {

class ITool;

/**
 * @class ToolRegistry
 * @brief 工具注册表
 * 
 * 单例模式管理所有已注册的工具
 * 提供工具的注册、查询和描述功能
 */
class ToolRegistry : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     * @return 工具注册表实例
     */
    static ToolRegistry& instance();
    
    void registerTool(ITool* tool);                   ///< 注册工具
    void unregisterTool(const QString& name);         ///< 注销工具
    ITool* getTool(const QString& name) const;        ///< 获取工具
    bool hasTool(const QString& name) const;          ///< 检查工具是否存在
    QStringList listTools() const;                    ///< 列出所有工具
    QJsonArray toolsSchema() const;                   ///< 获取工具 Schema
    QString toolsDescription() const;                 ///< 获取工具描述

private:
    ToolRegistry() = default;
    ToolRegistry(const ToolRegistry&) = delete;
    ToolRegistry& operator=(const ToolRegistry&) = delete;
    
    QMap<QString, ITool*> m_tools;                    ///< 工具映射表
};

}

#endif
