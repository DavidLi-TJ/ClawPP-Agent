#ifndef CLAWPP_COMMON_TYPES_H
#define CLAWPP_COMMON_TYPES_H

#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QFileInfo>
#include <functional>
#include <QStringList>

/**
 * @namespace clawpp
 * @brief Claw++ 项目的命名空间
 * 包含所有核心类型、类和功能
 */
namespace clawpp {

/**
 * @enum MessageRole
 * @brief 消息角色枚举
 * 定义对话中消息的发送者类型
 */
enum class MessageRole {
    User,       ///< 用户消息
    Assistant,  ///< 助手消息
    System,     ///< 系统消息
    Tool        ///< 工具调用结果消息
};

/**
 * @struct ToolCallData
 * @brief 工具调用数据（用于消息中的工具调用）
 */
struct ToolCallData {
    QString id;                   ///< 调用 ID
    QString type = "function";    ///< 调用类型
    QString name;                 ///< 函数名称
    QString arguments;            ///< 参数 JSON 字符串
};

/**
 * @struct Message
 * @brief 对话消息结构体
 * 表示单次对话的基本单元
 */
struct Message {
    MessageRole role;           ///< 消息角色
    QString content;            ///< 消息内容
    QString reasoningContent;   ///< 推理过程内容（如 DeepSeek thinking mode）
    QDateTime timestamp;        ///< 时间戳
    QString name;               ///< 发送者名称
    QString toolCallId;         ///< 工具调用 ID（如果是工具消息）
    QList<ToolCallData> toolCalls;  ///< 工具调用列表（助手消息）
    QJsonObject metadata;       ///< 运行期/展示层元数据
    
    Message() = default;
    Message(MessageRole r, const QString& c);
    
    QJsonObject toJson() const;
    static Message fromJson(const QJsonObject& obj);
};

using MessageList = QList<Message>;

struct CompactRecord {
    int index = 0;                      ///< 第几次压缩
    QString trigger;                    ///< 触发来源：auto/manual
    QString focus;                      ///< 手动 compact 的 focus
    QString archivePath;                ///< 压缩前存档路径
    QString summary;                    ///< 上次摘要内容
    QStringList recentFiles;            ///< 压缩时附带的最近文件
    QDateTime createdAt;                ///< 记录时间

    QJsonObject toJson() const;
    static CompactRecord fromJson(const QJsonObject& obj);
};

struct CompactState {
    int compactCount = 0;               ///< 会话内压缩次数
    QString lastSummary;                ///< 最近一次摘要
    QString lastArchivePath;            ///< 最近一次存档
    QStringList recentFiles;            ///< 最近 read_file 访问的文件，FIFO，最多 5 个
    QList<CompactRecord> history;       ///< 压缩历史

    QJsonObject toJson() const;
    static CompactState fromJson(const QJsonObject& obj);
};

/**
 * @struct ResponseFormat
 * @brief 响应格式配置
 * 定义 LLM 响应的格式类型
 */
struct ResponseFormat {
    enum Type { Text, JsonObject, JsonSchema };  ///< 响应格式类型：文本、JSON 对象、JSON Schema
    Type type = Text;
    QJsonObject jsonSchema;
    
    QJsonObject toJson() const;
    static ResponseFormat jsonSchemaFormat(const QJsonObject& schema);
};

/**
 * @struct ToolCallDelta
 * @brief 流式响应中的工具调用增量
 */
struct ToolCallDelta {
    int index = -1;              ///< 调用索引
    QString id;                  ///< 调用 ID
    QString type;                ///< 调用类型
    QString name;                ///< 函数名称
    QString arguments;           ///< 参数 JSON 字符串（增量）
};

/**
 * @struct StreamChunk
 * @brief 流式响应数据块
 * 表示流式响应中的单个数据块
 */
struct StreamChunk {
    QString content;              ///< 内容文本
    QString reasoningContent;     ///< 推理过程内容
    QString finishReason;         ///< 结束原因
    QList<ToolCallDelta> toolCalls; ///< 工具调用增量
    int promptTokens = 0;         ///< 提示词 token 数
    int completionTokens = 0;     ///< 完成 token 数
    int totalTokens = 0;          ///< 总 token 数
    bool hasUsage = false;        ///< 是否包含使用量信息
    
    bool isComplete() const { return !finishReason.isEmpty(); }  ///< 判断是否完成
};

using StreamCallback = std::function<void(const StreamChunk& chunk)>;  ///< 流式回调函数类型
using ErrorCallback = std::function<void(const QString& error)>;       ///< 错误回调函数类型

/**
 * @struct ProviderConfig
 * @brief LLM 提供者配置
 * 配置连接到 LLM 服务所需的参数
 */
struct ProviderConfig {
    QString name;                 ///< 提供者名称
    QString type = "openai";      ///< 提供者类型
    QString apiKey;               ///< API 密钥
    QString baseUrl;              ///< API 基础 URL
    QString model = "gpt-5.4-mini";///< 模型名称
    double temperature = 0.7;     ///< 温度参数（创造性）
    int maxTokens = 4096;         ///< 最大 token 数
    int timeoutMs = 60000;        ///< 超时时间（毫秒）
    
    QJsonObject toJson() const;
    static ProviderConfig fromJson(const QJsonObject& obj);
};

/**
 * @struct ChatOptions
 * @brief 聊天选项配置
 * 配置单次聊天请求的参数
 */
struct ChatOptions {
    QString model;                ///< 使用的模型
    double temperature = 0.7;     ///< 温度参数
    int maxTokens = 4096;         ///< 最大 token 数
    QString systemPrompt;         ///< 系统提示词
    QStringList stopSequences;    ///< 停止序列
    ResponseFormat responseFormat;///< 响应格式
    bool stream = true;           ///< 是否使用流式响应
    QJsonArray tools;             ///< 可用工具列表
};

/**
 * @enum PermissionLevel
 * @brief 权限级别枚举
 * 定义工具操作的危险程度
 */
enum class PermissionLevel {
    Safe,       ///< 安全操作
    Moderate,   ///< 中等风险
    Dangerous   ///< 危险操作
};

/**
 * @struct ToolCall
 * @brief 工具调用结构体
 * 表示对某个工具的调用请求
 */
struct ToolCall {
    QString id;                   ///< 调用 ID
    QString name;                 ///< 工具名称
    QString path;                 ///< 工具路径
    QJsonObject arguments;        ///< 调用参数
};

/**
 * @struct ToolResult
 * @brief 工具执行结果结构体
 * 表示工具执行后的返回结果
 */
struct ToolResult {
    QString toolCallId = "";      ///< 对应的工具调用 ID
    bool success = false;         ///< 是否成功
    QString content = "";         ///< 结果内容
    QJsonObject metadata;         ///< 元数据

    ToolResult() = default;
    ToolResult(const QString& id, bool ok, const QString& text)
        : toolCallId(id)
        , success(ok)
        , content(text) {}
};

/**
 * @enum PermissionResult
 * @brief 权限检查结果枚举
 */
enum class PermissionResult {
    Allowed,          ///< 允许
    Denied,           ///< 拒绝
    NeedConfirmation  ///< 需要确认
};

/**
 * @struct PermissionRule
 * @brief 权限规则结构体
 * 定义单个权限规则的配置
 */
struct PermissionRule {
    enum class Action { Allow, Deny, Ask };  ///< 规则动作：允许、拒绝、询问
    Action action = Action::Ask;
    QString pattern;              ///< 匹配模式
    QStringList tools;            ///< 适用的工具列表
    QString toolPattern;          ///< 工具匹配模式
    QString argumentPattern;      ///< 参数匹配模式
};

/**
 * @struct PermissionConfig
 * @brief 权限配置结构体
 * 定义整个权限系统的配置
 */
struct PermissionConfig {
    PermissionRule::Action defaultAction = PermissionRule::Action::Ask;  ///< 默认动作
    QList<PermissionRule> rules;    ///< 规则列表
    int timeoutSeconds = 60;        ///< 超时时间（秒）
};

/**
 * @struct Session
 * @brief 会话结构体
 * 表示一个完整的对话会话
 */
struct Session {
    QString id;                   ///< 会话 ID
    QString name;                 ///< 会话名称
    QDateTime createdAt;          ///< 创建时间
    QDateTime updatedAt;          ///< 更新时间
    MessageList messages;         ///< 消息列表
    bool isPinned = false;        ///< 是否置顶
    enum Status { Active, Paused, Archived } status = Active;  ///< 会话状态
};

/**
 * @struct MemoryConfig
 * @brief 内存配置结构体
 * 配置对话内存管理的参数
 */
struct MemoryConfig {
    int tokenThreshold = 4000;    ///< token 阈值（触发压缩）
    int keepFirst = 3;            ///< 保留开头的消息数
    int keepLast = 5;             ///< 保留结尾的消息数
    int maxMessages = 100;        ///< 最大消息数
    double summaryRatio = 0.3;    ///< 摘要比例
    int retrievalTopK = 6;        ///< 文件式记忆检索返回条数
    int embeddingDimension = 64;  ///< 兼容字段：当前文件式记忆模式不依赖向量检索
    double optimizationThreshold = 0.92; ///< 兼容字段：当前文件式记忆模式不做向量去重
    
    static MemoryConfig fromJson(const QJsonObject& obj);
};

/**
 * @struct AppConfig
 * @brief 应用配置结构体
 * 整个应用的配置信息
 */
struct AppConfig {
    QString theme = "light";      ///< 主题
    QString language = "zh_CN";   ///< 语言
    bool autoSave = true;         ///< 是否自动保存
    int autoSaveInterval = 30;    ///< 自动保存间隔（秒）
    ProviderConfig provider;      ///< 提供者配置
    MemoryConfig memory;          ///< 内存配置
    PermissionConfig permission;  ///< 权限配置
    
    static AppConfig fromJson(const QJsonObject& obj);
};

}
#endif
