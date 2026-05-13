# Claw++ 项目 6 分钟精讲稿

> 适合录屏讲解，节奏紧凑。每节都有「展示代码」提示，直接在 VS Code 里 `Ctrl+P` 跳转到对应位置。
> 文字内容已设计为口语化、和画面同步的节奏。

---

## 0. 开场（约 30 秒）

大家好，我今天讲解的项目叫 **Claw++**——一个用 Qt/C++ 从零构建的 **AI Agent 桌面应用**。

它不是简单的聊天窗口。它的核心是 **ReAct 推理引擎**：模型会先思考，再决定调用工具（读文件、执行命令、搜代码、发网络请求），然后观察结果，继续下一轮推理，直到完成任务。

同时它有一套完整的工程系统：双层上下文压缩、权限控制、SQLite 持久化、技能插件系统、Telegram/飞书接入。

接下来我从 **架构、消息流、ReAct 核心、上下文压缩、安全设计** 五个方面来拆解。

---

## 1. 架构一览（约 1 分钟）

> 🖥️ 展示文件：`src/qml/qml_backend.cpp`

项目整体是六层架构。我们来看初始化代码。

**我们来看这段**，`QmlBackend` 的构造函数（L680）：

```cpp
QmlBackend::QmlBackend(QObject* parent)
    : QObject(parent)
    , m_sessionsModel(new SessionListModel(this))
    , m_messagesModel(new MessageListModel(this))
    , m_sessionManager(new SessionManager(this))
    , m_agentService(new AgentService(this))
    , m_providerManager(new ProviderManager(this))
    , m_memoryManager(new MemoryManager(backendWorkspaceRoot(), this))
    , m_externalPlatformManager(new ExternalPlatformManager(this))
    , m_templateLoader(new TemplateLoader(backendWorkspaceRoot(), backendKernelRoot(), this))
```

可以看到：SessionManager 管会话、AgentService 管 Agent 编排、ProviderManager 管模型、MemoryManager 管记忆、ExternalPlatformManager 管 Telegram/飞书。所有核心模块在初始化时就注入了。

**再往下看**，工具注册（L662-L678）：

```cpp
void QmlBackend::registerTools() {
    auto& registry = ToolRegistry::instance();
    registry.registerTool(new ReadFileTool());
    registry.registerTool(new WriteFileTool());
    registry.registerTool(new ListDirectoryTool());
    registry.registerTool(new EditFileTool());
    registry.registerTool(new SearchFilesTool());
    registry.registerTool(new CompactTool());
    auto* shellTool = new ShellTool();
    shellTool->setWorkingDirectory(QDir::currentPath());
    registry.registerTool(shellTool);
    registry.registerTool(new NetworkTool());
    registry.registerTool(new SystemTool());
    registry.registerTool(new SubagentTool());
}
```

10 个工具，涵盖文件、Shell、网络、搜索、子代理、上下文压缩。CompactTool 比较特殊，我们后面讲压缩时会看到。

所以这六层是：UI 层（QML）→ 桥接层（QmlBackend）→ 编排层（AgentService/SessionManager）→ Agent 核心层（ReactAgentCore）→ 基础能力层（Provider/Tool/Memory/Permission）→ 基础设施层（SQLite/日志/网络）。

---

## 2. 消息流核心（约 1.5 分钟）

> 🖥️ 展示文件：`src/agent/react_agent_core.cpp`

**我们来看一条消息从用户输入到最终回复，系统内部是怎么跑起来的**。

用户输入 → QML → `QmlBackend::sendMessage()` → `AgentService` 判断模式、匹配技能、构造 system prompt → 投递到 `ReactAgentCore` 线程。

**我们来看这段核心代码**，`processIteration()`（L1058）：

```cpp
void ReactAgentCore::processIteration() {
    if (finalizeIfStoppedOrExceeded()) {
        return;
    }
    m_iteration++;
    MessageList messages = buildIterationMessages();
    ChatOptions options;
    options.model = m_providerConfig.model;
    options.temperature = m_providerConfig.temperature;
    options.maxTokens = m_providerConfig.maxTokens;
    options.stream = m_provider->supportsStreaming();
    if (m_toolsEnabled && m_provider->supportsTools()) {
        QJsonArray scopedTools;
        for (const QString& toolName : m_runtimeToolNames) {
            ITool* tool = ToolRegistry::instance().getTool(toolName.trimmed());
            if (tool) { scopedTools.append(tool->toJson()); }
        }
        options.tools = scopedTools;
```

关键点：每次迭代前检查是否要停止、构造消息列表、根据 runtime tool names 动态构造可用工具列表发给模型。然后 `m_provider->chatStream()` 以流式方式请求。

模型返回 chunk → `handleStreamChunk()` 解析内容和 tool_calls → QmlBackend 更新 UI。

如果模型调了工具 → `executeToolCalls()` 执行 → 结果写回消息列表 → 继续下一轮 `processIteration()`。

最终 `finalizeResponse()` 统一落地 assistant 消息 → conversationUpdated → SessionManager 写入 SQLite → UI 同步。

这就是 ReAct 循环的核心链路：**思考 → 行动 → 观察 → 再思考**，直到得出结论。

---

## 3. 两层上下文压缩（约 1 分钟）

> 🖥️ 展示文件：`src/agent/react_agent_core.cpp`

Agent 调工具时上下文膨胀很快，必须压缩。

**我们来看这段压缩保护逻辑**，`protectLatestToolBatch()`（L2022）：

```cpp
bool ReactAgentCore::protectLatestToolBatch(
    const MessageList& messages, QSet<int>* protectedIndexes) const {
    int lastAssistantToolCallIndex = -1;
    QSet<QString> expectedToolIds;
    for (int i = messages.size() - 1; i >= 0; --i) {
        const Message& message = messages.at(i);
        if (message.role == MessageRole::Assistant && !message.toolCalls.isEmpty()) {
            lastAssistantToolCallIndex = i;
            for (const ToolCallData& call : message.toolCalls) {
                if (!call.id.trimmed().isEmpty())
                    expectedToolIds.insert(call.id.trimmed());
            }
            break;
        }
    }
    // ... 保护最后一批 tool call 及其对应的 tool 结果
```

**这段代码非常关键**——它找到最后一条 assistant 消息中所有 tool_call id，然后保护对应 tool 结果不被折叠。如果最后一批结果被压缩掉，模型下一轮就看不到刚拿到的数据，会出错。

压缩分两层：
1. **MicroCompact**：把旧工具结果替换为一句占位摘要，零模型调用成本
2. **Full Compact**：如果 MicroCompact 后仍然超限，存档完整消息 → 调模型生成结构化摘要 → 用摘要替换旧历史

**我们来看摘要提示词**（L2175）：

```cpp
QString ReactAgentCore::buildFullCompactPrompt(...) {
    lines.append(QStringLiteral("必须保留以下五部分："));
    lines.append(QStringLiteral("1. 当前目标"));
    lines.append(QStringLiteral("2. 重要发现与决策"));
    lines.append(QStringLiteral("3. 读过和修改过的文件"));
    lines.append(QStringLiteral("4. 剩余工作"));
    lines.append(QStringLiteral("5. 用户约束"));
```

五部分结构化摘要，让压缩后的历史依然保持工作上下文。同时 `RecentFiles` 追踪最近读取的文件，压缩后也能快速恢复。

---

## 4. 权限与安全（约 30 秒）

> 🖥️ 展示文件：`src/permission/permission_manager.cpp`

Agent 能执行 Shell、改文件，所以必须有安全边界。

**我们来看权限检查**（L50）：

```cpp
PermissionResult PermissionManager::checkPermission(
    const QString& sessionId, const ToolCall& toolCall) {
    QString key = sessionId + "::" + toolCall.name + ":" + toolCall.path;
    if (m_cache.contains(key)) { return *m_cache.object(key); }
    if (isWhitelisted(sessionId, toolCall))
        return PermissionResult::Allowed;
    for (const auto& rule : m_rules) {
        if (matchesPattern(rule.pattern, toolCall))
            return actionToResult(rule.action);
    }
    return actionToResult(m_config.defaultAction);
}
```

三级判断：白名单 → 规则匹配 → 默认动作。此外 `BashAnalyzer` 对 Shell 命令做风险评分，高危命令直接阻止，中风险要求用户确认。权限结果还做了缓存，避免重复弹窗。

---

## 5. 项目亮点与收尾（约 1 分钟）

> 🖥️ 最后切到项目目录整体看一眼

最后总结几个关键亮点：

1. **真正的 Agent**，不是简单聊天。ReAct 推理 + 10 种工具 + 多轮迭代。
2. **上下文压缩系统完善**——两层策略 + 保护最新结果 + 结构化摘要 + RecentFiles 追踪 + 存档回溯。
3. **安全边界明确**——权限管理 + Shell 风险评分，不是"让模型随便跑命令"。
4. **工程架构清爽**——六层分层、独立线程、消息落盘 SQLite、Telegram/飞书接入。
5. **扩展性好**——Provider 即插式、技能系统热加载、工具注册制、模板系统、IFW 打包安装。

项目规模：70+ C++ 文件、15+ QML 组件、约 12,000+ 行代码，纯 C++ 编译，安装包不到 60MB。

以上就是 Claw++ 的核心讲解，谢谢大家！

---

## 代码导航速查表

| 环节 | 文件 | 行号 |
|------|------|------|
| 工具注册 | `src/qml/qml_backend.cpp` | 662-678 |
| 核心对象初始化 | `src/qml/qml_backend.cpp` | 680-701 |
| ReAct 迭代 | `src/agent/react_agent_core.cpp` | 1058-1084 |
| 流式处理 | `src/agent/react_agent_core.cpp` | 1273+ |
| 工具执行 | `src/agent/react_agent_core.cpp` | 1424-1591 |
| 最终落地 | `src/agent/react_agent_core.cpp` | 1995-2014 |
| 保护最新工具结果 | `src/agent/react_agent_core.cpp` | 2022-2059 |
| Full Compact 存档 | `src/agent/react_agent_core.cpp` | 2138-2166 |
| 结构化摘要提示词 | `src/agent/react_agent_core.cpp` | 2175-2197 |
| 权限检查 | `src/permission/permission_manager.cpp` | 50-74 |
| SQLite 初始化 | `src/infrastructure/database/database_manager.cpp` | 53-95 |