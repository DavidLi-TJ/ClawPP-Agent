#include "system_tool.h"
#include <QSysInfo>
#include <QProcessEnvironment>
#include <QDateTime>
#include <QHostInfo>

namespace clawpp {

SystemTool::SystemTool(QObject* parent)
    : ITool(parent) {}

QString SystemTool::name() const {
    return "system";
}

QString SystemTool::description() const {
    return "System operations: get_env, set_env, get_current_time, get_system_info";
}

QJsonObject SystemTool::parameters() const {
    QJsonObject params;
    params["type"] = "object";
    
    QJsonObject properties;
    
    QJsonObject operation;
    operation["type"] = "string";
    operation["enum"] = QJsonArray{"get_env", "set_env", "get_current_time", "get_system_info"};
    operation["description"] = "The system operation to perform";
    properties["operation"] = operation;
    
    QJsonObject name;
    name["type"] = "string";
    name["description"] = "Environment variable name (for get_env, set_env)";
    properties["name"] = name;
    
    QJsonObject value;
    value["type"] = "string";
    value["description"] = "Environment variable value (for set_env)";
    properties["value"] = value;
    
    QJsonObject format;
    format["type"] = "string";
    format["description"] = "Time format string (for get_current_time, default: ISO)";
    format["default"] = "iso";
    properties["format"] = format;
    
    params["properties"] = properties;
    params["required"] = QJsonArray{"operation"};
    
    return params;
}

PermissionLevel SystemTool::permissionLevel() const {
    return PermissionLevel::Safe;
}

ToolResult SystemTool::execute(const QJsonObject& args) {
    ToolResult result;
    
    QString operation = args["operation"].toString();
    
    if (operation.isEmpty()) {
        result.success = false;
        result.content = "Error: operation is required";
        return result;
    }
    
    // 单一入口分发到各子操作，保证参数校验逻辑集中。
    if (operation == "get_env") {
        return getEnv(args["name"].toString());
    } else if (operation == "set_env") {
        return setEnv(args["name"].toString(), args["value"].toString());
    } else if (operation == "get_current_time") {
        return getCurrentTime(args["format"].toString("iso"));
    } else if (operation == "get_system_info") {
        return getSystemInfo();
    } else {
        result.success = false;
        result.content = QString("Error: Unknown operation: %1").arg(operation);
        return result;
    }
}

ToolResult SystemTool::getEnv(const QString& name) {
    ToolResult result;
    
    if (name.isEmpty()) {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        QJsonObject envObj;
        for (const QString& key : env.keys()) {
            envObj[key] = env.value(key);
        }
        result.success = true;
        result.content = QString("Environment variables (%1 items)").arg(env.keys().size());
        result.metadata["environment"] = envObj;
    } else {
        QString value = qEnvironmentVariable(qPrintable(name));
        if (!value.isNull()) {
            result.success = true;
            result.content = QString("%1 = %2").arg(name, value);
            result.metadata["name"] = name;
            result.metadata["value"] = value;
        } else {
            result.success = false;
            result.content = QString("Environment variable not found: %1").arg(name);
        }
    }
    
    return result;
}

ToolResult SystemTool::setEnv(const QString& name, const QString& value) {
    ToolResult result;
    
    if (name.isEmpty()) {
        result.success = false;
        result.content = "Error: name is required for set_env";
        return result;
    }
    
    bool success = qputenv(qPrintable(name), value.toUtf8());
    
    if (success) {
        result.success = true;
        result.content = QString("Set environment variable: %1 = %2").arg(name, value);
        result.metadata["name"] = name;
        result.metadata["value"] = value;
    } else {
        result.success = false;
        result.content = QString("Failed to set environment variable: %1").arg(name);
    }
    
    return result;
}

ToolResult SystemTool::getCurrentTime(const QString& format) {
    ToolResult result;
    
    QDateTime now = QDateTime::currentDateTime();
    
    QString timeStr;
    if (format == "iso" || format.isEmpty()) {
        timeStr = now.toString(Qt::ISODate);
    } else if (format == "unix") {
        timeStr = QString::number(now.toSecsSinceEpoch());
    } else {
        timeStr = now.toString(format);
    }
    
    result.success = true;
    result.content = QString("Current time: %1").arg(timeStr);
    result.metadata["iso"] = now.toString(Qt::ISODate);
    result.metadata["unix"] = now.toSecsSinceEpoch();
    result.metadata["formatted"] = timeStr;
    result.metadata["timezone"] = now.timeZoneAbbreviation();
    
    return result;
}

ToolResult SystemTool::getSystemInfo() {
    ToolResult result;
    
    QJsonObject info;

    // 仅收集无侵入、可公开的系统信息。
    info["os_name"] = QSysInfo::prettyProductName();
    info["os_type"] = QSysInfo::productType();
    info["os_version"] = QSysInfo::productVersion();
    info["kernel_type"] = QSysInfo::kernelType();
    info["kernel_version"] = QSysInfo::kernelVersion();
    info["cpu_architecture"] = QSysInfo::currentCpuArchitecture();
    info["build_abi"] = QSysInfo::buildAbi();
    info["machine_hostname"] = QHostInfo::localHostName();
    info["machine_domain"] = QHostInfo::localDomainName();
    
    QStringList infoLines;
    infoLines.append(QString("OS: %1").arg(info["os_name"].toString()));
    infoLines.append(QString("Kernel: %1 %2").arg(
        info["kernel_type"].toString(),
        info["kernel_version"].toString()));
    infoLines.append(QString("CPU Architecture: %1").arg(
        info["cpu_architecture"].toString()));
    infoLines.append(QString("Hostname: %1").arg(
        info["machine_hostname"].toString()));
    
    result.success = true;
    result.content = "System Information:\n" + infoLines.join("\n");
    result.metadata["system_info"] = info;
    
    return result;
}

}
