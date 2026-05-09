#ifndef CLAWPP_TOOL_I_TOOL_H
#define CLAWPP_TOOL_I_TOOL_H

#include <QObject>
#include <QJsonObject>
#include "common/types.h"

namespace clawpp {

/**
 * @class ITool
 * @brief 工具接口类
 * 
 * 定义所有工具实现的基本接口
 * 每个工具都应该实现此接口以被 Agent 调用
 */
class ITool : public QObject {
    Q_OBJECT

public:
    explicit ITool(QObject* parent = nullptr)
        : QObject(parent) {}
    
    virtual ~ITool() = default;
    
    /**
     * @brief 获取工具名称
     * @return 工具名称
     */
    virtual QString name() const = 0;
    
    /**
     * @brief 获取工具描述
     * @return 工具描述
     */
    virtual QString description() const = 0;
    
    /**
     * @brief 获取工具参数定义
     * @return JSON 格式的参数定义
     */
    virtual QJsonObject parameters() const = 0;
    
    /**
     * @brief 获取权限级别
     * @return 权限级别
     */
    virtual PermissionLevel permissionLevel() const = 0;

    /**
     * @brief 工具是否允许并发执行
     * @return 是否允许并发
     *
     * 默认按权限级别推断：安全工具可并发，其余工具默认串行。
     */
    virtual bool canRunInParallel() const { return permissionLevel() == PermissionLevel::Safe; }

    /**
     * @brief 工具是否需要执行前确认
     * @return 是否需要确认
     *
     * 默认按权限级别推断：中高风险工具需要确认。
     */
    virtual bool needsConfirmation() const {
        return permissionLevel() != PermissionLevel::Safe;
    }
    
    /**
     * @brief 执行工具
     * @param args 工具参数
     * @return 执行结果
     */
    virtual ToolResult execute(const QJsonObject& args) = 0;
    
    QJsonObject toJson() const;  ///< 转换为 JSON 对象
};

}

#endif
