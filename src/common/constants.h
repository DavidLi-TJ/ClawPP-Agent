#ifndef CLAWPP_COMMON_CONSTANTS_H
#define CLAWPP_COMMON_CONSTANTS_H

#include <QString>

/**
 * @namespace clawpp
 * @brief Claw++ 项目命名空间
 */
namespace clawpp {

/**
 * @namespace constants
 * @brief 常量定义命名空间
 * 包含项目中使用的所有常量
 */
namespace constants {

constexpr const char* APP_NAME = "Claw++";           ///< 应用名称
constexpr const char* APP_VERSION = "1.0.0";         ///< 应用版本

constexpr const char* DEFAULT_CONFIG_FILE = "config/default_config.json";  ///< 默认配置文件
constexpr const char* USER_CONFIG_DIR = ".clawpp";   ///< 用户配置目录
constexpr const char* USER_CONFIG_FILE = "config.json";  ///< 用户配置文件

constexpr const char* DEFAULT_BASE_URL = "https://api.openai.com/v1";  ///< 默认 API 地址
constexpr const char* DEFAULT_MODEL = "gpt-5.4-mini"; ///< 默认模型

constexpr int DEFAULT_TIMEOUT_MS = 60000;            ///< 默认超时时间（毫秒）
constexpr int DEFAULT_MAX_TOKENS = 2048;             ///< 默认最大 token 数
constexpr double DEFAULT_TEMPERATURE = 0.7;          ///< 默认温度参数

constexpr const char* MIME_TYPE_JSON = "application/json";  ///< JSON MIME 类型
constexpr const char* MIME_TYPE_SSE = "text/event-stream";  ///< SSE 流 MIME 类型

constexpr int MAX_MESSAGE_HISTORY = 100;             ///< 最大消息历史数
constexpr int MAX_ITERATIONS = 10;                   ///< 最大迭代次数
constexpr int MAX_NO_TOOL_STREAK = 2;                ///< 连续无工具调用轮次阈值
constexpr int PERMISSION_TIMEOUT_MS = 60000;         ///< 权限超时时间（毫秒）
constexpr int SUBAGENT_TIMEOUT_MS = 120000;          ///< 子代理执行超时时间（毫秒）

constexpr int MEMORY_TOKEN_THRESHOLD = 4000;         ///< 内存 token 阈值
constexpr int MEMORY_KEEP_FIRST = 3;                 ///< 内存保留开头数
constexpr int MEMORY_KEEP_LAST = 5;                  ///< 内存保留结尾数

constexpr const char* SESSION_COPY_SUFFIX = "Copy"; ///< 会话复制名称后缀

}

}

#endif
