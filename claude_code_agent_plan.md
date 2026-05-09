# Claw++ / Qt6 AI Agent 改造规划

> 目标：只吸收 Claude Code 的优秀工程思想，把 Claw++ 从“能跑的 AI 工具”优化成“结构清晰、可扩展、可维护的 Qt6 AI Agent”，**不追求完整复刻**，也不把复杂度拉到 Claude Code 级别。

---

## 1. 参考仓库分析结论

### 1.1 `instructkr/claude-code`

这个仓库的价值不在“源码重写本身”，而在它暴露出的工程侧重点：

- 强调 **分层**：CLI / Query Engine / Tool Runtime / Service / Memory
- 强调 **工具统一接口**：所有工具都走同一套注册、调度、权限、结果回传流程
- 强调 **流式执行**：模型输出、工具调用、结果回写是一个连续的事件链
- 强调 **会话/上下文管理**：长会话不是简单堆消息，而是有压缩、摘要、恢复机制

对 Claw++ 的启发：

- 不需要复制大量功能，但要把“Agent 核心循环”从 UI 里抽出来
- 工具系统要从“散装工具类”收敛成“统一协议 + 注册表 + 执行器”
- 会话状态、历史、上下文构建要服务于 Agent，而不是只服务于界面

### 1.2 `sanbuphy/claude-code-source-code`

这个仓库更偏向“源码拆解”和“模块枚举”，它的启发是：

- Claude Code 的核心不是某个单一类，而是一组 **可替换服务**
- 真正关键的是：
  - 入口层
  - 查询/推理引擎
  - 工具编排层
  - 权限层
  - 记忆与技能层
  - 服务层（provider / MCP / storage）

对 Claw++ 的启发：

- 不要把所有逻辑塞进 `main.cpp`、`QmlBackend` 或某个大类
- 要把“AI 请求、工具调用、历史记录、权限判断、结果渲染”拆成独立子系统
- 保持“模块边界清楚，依赖单向流动”

### 1.3 `Windy3f3f3f3f/how-claude-code-works`

这个仓库的总结最适合做“改造设计参考”，因为它总结的是 Claude Code 的工程方法论：

- **Streaming-first**：先流式，再考虑补充信息
- **Defense in depth**：权限不是单点判断，而是多层防线
- **Context engineering**：上下文不是越多越好，而是越有效越好
- **Tool orchestration**：工具执行要可并发、可序列化、可恢复
- **Extensibility**：技能、钩子、子代理是“扩展点”，不是核心复杂度本体

对 Claw++ 的启发：

- 保持“可扩展”，但优先做“少而稳”的核心能力
- 把复杂设计变成“可选增强”，不要默认全开

---

## 2. 建议的整体分层架构

下面是最适合你当前 Claw++ 的 **低复杂度 Qt6 分层架构**。

```text
UI 层
├─ QML / Widgets
├─ 会话面板、消息列表、日志视图、权限弹窗
└─ 只负责展示与用户输入

应用编排层
├─ AgentService
├─ SessionManager / SessionThread
├─ MessageRouter
└─ 负责把“输入 -> 推理 -> 工具 -> 输出”串起来

Agent 核心层
├─ ContextBuilder
├─ PromptAssembler
├─ ReAct / Tool Loop
├─ ResponseParser
└─ 负责决定下一步是回答、调用工具还是继续循环

工具运行层
├─ ToolRegistry
├─ ToolExecutor
├─ ToolResultNormalizer
├─ PermissionManager
└─ 负责统一执行工具并做权限与结果标准化

记忆与会话层
├─ ConversationHistory
├─ MemoryManager
├─ SessionStore
└─ 负责保存历史、摘要、最近文件、会话状态

基础设施层
├─ ProviderManager
├─ Anthropic / OpenAI / Gemini Provider
├─ HTTP / SSE / JSON 工具
├─ 文件存储 / config / logging
└─ 负责网络、配置、持久化、诊断；SQLite 仅作为可选会话归档
```

### 建议的依赖方向

只允许单向依赖，避免“互相调用打结”：

```text
UI -> Application -> Agent Core -> Tool Runtime -> Infrastructure
                     └-----------> Memory/Session
```

原则：

- UI 不直接调用 Provider
- Tool 不直接操作 UI
- Provider 不知道 Tool 的具体实现
- Permission 只做判定，不做业务编排

---

## 3. 如何实现：把 Claude Code 的优秀点迁移到 Claw++

### 3.1 先收敛成一个“Agent 主循环”

Claw++ 当前已经有不少基础模块，最该补的是 **统一的 Agent Loop**。

建议把核心流程固定为：

1. 用户输入消息
2. `ContextBuilder` 组装上下文
3. `Provider` 发起流式请求
4. 模型返回文本或工具调用
5. `ToolExecutor` 执行工具
6. 工具结果回写消息流
7. 进入下一轮，直到完成

这对应 Claude Code 的“think-act-observe”模式，但在你这里可以简化为：

- 不做复杂多代理调度
- 不做重型任务队列
- 只保留一个清晰的循环

### 3.2 把工具系统统一成一套协议

你现在的工具结构已经比较完整，但下一步要做的是“统一协议化”。

建议每个工具都满足：

- 输入：`QJsonObject` 或 `QVariantMap`
- 输出：统一结果对象
  - `success`
  - `data`
  - `error`
  - `meta`
- 元数据：
  - 名称
  - 描述
  - 参数 schema
  - 是否只读
  - 是否需要确认
  - 是否允许并发

但不要把协议做成重型框架。对 Claw++ 来说，最重要的是保留 Claude Code 那种清晰的工具生命周期：

1. 识别工具意图
2. 做最小必要校验
3. 需要时请求确认
4. 执行工具
5. 回传结构化结果
6. 决定是否继续下一轮

也就是说，工具系统要“统一”，但不要“过度抽象”。

这样可以直接得到 Claude Code 的几个优点：

- 工具注册一致
- 工具执行一致
- 权限判定一致
- UI 展示一致

### 3.3 做“轻量版权限防线”

Claude Code 的安全设计很强，但你不需要照搬全部，只保留实用部分即可。

建议保留 3 层：

1. **权限模式**
   - 只读
   - 交互确认
   - 全自动

2. **工具级判定**
   - 文件写入、删除、命令执行、网络访问、系统操作分别判定

3. **执行前确认**
   - 高风险操作弹窗确认
   - 给出明确参数和影响范围

这样就足够把“AI Agent 不可控”变成“可解释、可拦截、可审计”。

注意不要把权限层做成一堆难以维护的规则引擎。对当前项目来说，最实用的是：

- shell / 文件写入 / 删除 / 网络访问先分开
- 高风险操作必须显式确认
- 低风险读操作尽量不打断流式体验

### 3.4 做“上下文工程”，但别做过度压缩

Claude Code 强在长会话上下文管理。Claw++ 可以做一个简化版：

- 最近消息保留完整
- 旧消息压缩为摘要
- 最近编辑文件加入上下文
- 工具输出只保留关键片段
- 超长内容自动落盘，界面显示摘要和文件引用

推荐的最小实现：

- `ConversationHistory` 保存原始消息
- `ContextBuilder` 生成送给模型的上下文
- `MemoryManager` 维护“文件式记忆索引”

这里更接近 Claude Code 的思路：记忆不是先丢进数据库，而是优先来自项目文件、技能文档、会话摘要和最近编辑痕迹。检索路径主要靠 `glob`、`read_file`、`grep` 这类工具，而不是先做一层重型数据库查询。

不建议一开始就做复杂的多层上下文折叠算法，先做“可用且稳定”的版本。

不过也不要把上下文简化成“只读几个文件就完事”。更合理的做法是：

- 让 `ContextBuilder` 先收集当前任务最相关的文件和摘要
- 再用工具检索补齐缺失信息
- 最后把结果压缩进一次可控的模型请求

这样既保留 Claude Code 的方法论，也不会把实现做成过度工程化。

### 3.5 流式输出是体验核心

Claude Code 的“快感”本质上是流式链路做得好。

Claw++ 应该把流式拆成三个事件：

- `token`：模型增量文本
- `tool_call`：即将执行工具
- `tool_result`：工具执行结果

UI 层只负责订阅事件并刷新视图。

这样会带来两个明显收益：

- 用户更快看到反馈
- 调试更容易，日志更清晰

### 3.6 轻量技能系统

Claude Code 的 skills 思路值得借鉴，但不要做复杂市场或远程加载。

建议 Claw++ 只做本地版：

- `skills/` 目录下放 Markdown 或 JSON 技能说明
- 启动时扫描
- 作为提示词增强和工具调用建议来源
- 支持刷新，但不必做在线分发

它的价值是：

- 让 Agent 更懂项目
- 让常见任务固定成模板
- 不需要把规则写死在代码里

### 3.7 可选的“子代理”能力

Claude Code 的多代理系统很强，但对你当前项目来说，只建议保留最小化版本：

- 主 Agent 负责用户交互
- 子 Agent 只处理“可拆分的独立任务”
- 子 Agent 的结果以文本或结构化摘要返回

不要一开始就做：

- 协调器模式
- swarm 模式
- 多工作区并发编辑
- 远程协同

这些都属于第二阶段以后再考虑。

---

## 4. 工业级设计亮点：建议直接吸收的部分

### 4.1 统一工具协议

这是最值得迁移的核心设计。

收益：

- 新工具接入成本低
- 审计、日志、权限统一
- UI 和 Agent 都能稳定消费工具结果

### 4.2 Streaming-first

收益：

- 体验更顺滑
- 让长耗时任务更“像在工作”
- 更适合 Qt 的信号/槽模型

### 4.3 防御分层

建议保留而不是简化掉：

- 只读/可写区分
- 危险工具二次确认
- 命令类工具单独策略
- 网络类工具单独策略

### 4.4 上下文工程

收益：

- 更稳定的长对话体验
- 更低 token 浪费
- 更少“模型忘了刚才做什么”的问题

建议优先采用“文件式记忆”：

- `CLAUDE.md` / 项目说明 / 技能说明
- 最近编辑文件列表
- 会话摘要文件
- 关键命令与工具结果的文本归档

SQLite 不应成为核心检索层；如果保留，也只适合做可选的会话归档或历史索引，不要把它设计成主记忆引擎。

### 4.5 会话可恢复

建议支持：

- 重新打开会话
- 继续上次对话
- 恢复最近上下文
- 查看工具执行历史

这会显著提高一个桌面 AI Agent 的“工程感”。

### 4.6 结构化日志与追踪

建议为每次会话生成：

- session id
- request id
- tool call id
- provider trace

日志分层：

- UI 日志
- Agent 日志
- 工具日志
- 网络日志

这对 Qt 桌面端调试特别重要。

---

## 5. 可直接迁移到 Qt C++ AI Agent 的架构要点

结合你当前 `cpqclaw` 的代码结构，建议直接落地以下要点。

### 5.1 保持 `QmlBackend` 只做门面

当前 `QmlBackend` 适合作为 UI 与业务的桥。

建议职责限定为：

- 收发消息
- 暴露会话状态
- 发出信号给界面刷新
- 不直接处理推理细节

### 5.2 新增一个 `AgentOrchestrator`

这是最关键的新增核心。

职责：

- 接收用户输入
- 组装上下文
- 调用 provider
- 处理工具调用
- 维护循环状态
- 输出流式事件

它相当于 Claude Code 的 Query Engine + agent loop 的合体，但保持轻量。

### 5.3 把 `ToolRegistry` 升级为“元数据驱动”

现在的工具注册建议扩展成：

- `name`
- `description`
- `inputSchema`
- `outputSchema`
- `riskLevel`
- `readOnly`
- `canRunInParallel`

这样可以直接驱动：

- 提示词生成
- 权限策略
- UI 工具列表
- 自动文档

### 5.4 把 `PermissionManager` 做成独立策略层

不要把权限逻辑写死在工具里。

建议支持：

- 默认策略
- 工具白名单
- 用户一次性授权
- 会话级授权

这样未来扩展很容易。

### 5.5 用 `ConversationStore` + `MemoryManager` 做会话和摘要

建议从一开始就区分：

- 原始消息存储
- 运行上下文摘要
- 关键文件记忆
- 用户偏好记忆

实现上建议以文件为主：

- 原始消息与摘要优先落到本地文本/JSON 文件
- `CLAUDE.md`、`skills/`、项目说明作为知识入口
- `glob` + `read_file` + `grep` 作为主要检索通路
- SQLite 仅在你确实需要“可选归档/查询”时再接入

不要把所有内容都混进一个历史表，也不要把记忆层做成数据库中心化系统。

### 5.6 Provider 统一抽象

你已经有多 provider 结构，这一点很好，建议继续强化：

- 统一请求模型
- 统一流式接口
- 统一错误对象
- 统一 token 统计

这样后面切换模型提供方不会改动 Agent 核心。

### 5.7 工具执行异步化

Qt C++ 里建议：

- 长任务走 `QThread` / `QtConcurrent`
- 主线程只更新 UI
- 工具结果通过 signal 回传

这和 Claude Code 的事件驱动模型很契合。

---

## 6. 建议的模块映射：从 Claude Code 思路到 Claw++

| Claude Code 思路 | Claw++ 迁移方式 |
|---|---|
| Query Engine | `AgentOrchestrator` |
| Tool Registry | `ToolRegistry` 元数据化 |
| Streaming Tool Executor | `ToolExecutor` + 流式事件 |
| Context Engineering | `ContextBuilder` + `MemoryManager` |
| Permission & Security | `PermissionManager` + 风险分级 |
| Session History | `ConversationStore` + `SessionManager` |
| Skill System | 本地 `SkillManager` |
| Provider Abstraction | `ProviderManager` |
| Hook/Extensibility | 轻量策略回调，不做复杂插件市场 |

---

## 7. 改造路线图

> 说明（2026-04 当前代码状态）：Phase 1-3 已完成主干落地；Phase 4 已完成“轻量技能接入”基础能力；Phase 5 已完成最小测试与部分结构化日志，仍需在具备 `pwsh` 运行环境后做完整构建/测试回归。

### Phase 0：架构收口

目标：先把现有代码从“可工作”整理成“可演进”。

动作：

- 明确 `UI / Application / Agent / Tool / Memory / Infrastructure` 边界
- 让 `QmlBackend` 变薄
- 给 `AgentService` 加统一入口
- 补充基础日志和 session id

交付：

- 主流程清晰
- 业务入口统一
- 后续改造不再牵一发动全身

### Phase 1：Agent 核心循环

目标：形成一个稳定的“输入 -> 推理 -> 工具 -> 回写”闭环。

动作：

- 新增 `AgentOrchestrator`
- 接入流式输出
- 标准化工具调用事件
- 统一错误返回

状态：已落地（当前版本）

交付：

- Agent 主循环稳定
- UI 能实时看到增量输出
- 工具调用可追踪

### Phase 2：工具协议与权限

目标：让工具系统真正可控、可扩展。

动作：

- 工具元数据标准化
- 风险等级定义
- 高风险操作确认弹窗
- 工具执行统一日志

交付：

- 新工具接入更轻松
- 风险操作更安全
- 更接近 Claude Code 的工程质感

### Phase 3：上下文与记忆

目标：让长会话更稳。

动作：

- 历史摘要
- 最近文件恢复
- 工具输出裁剪
- 会话恢复
- 文件式记忆索引

交付：

- 长对话不容易失控
- 上下文更“聪明”

### Phase 4：轻量技能与子代理

目标：增强高级任务能力，但保持简单。

动作：

- 本地技能目录
- 技能模板提示词
- 简化子代理调用，但只保留“主 Agent + 可拆分子任务”这一条主线

交付：

- 常见任务更自动化
- 复杂任务可以拆分

### Phase 5：工程化加固

目标：让项目更像产品而不是 demo。

动作：

- 完善测试
- 补全错误码
- 结构化日志
- 性能观察

交付：

- 更易维护
- 更易排障
- 更适合后续迭代

### 7.1 节奏控制原则

为了避免“太简化”或“太复杂”，后续实现时建议坚持这几个节奏：

- 先做一个完整闭环，再加优化项
- 先做主路径，再补边界条件
- 先做少量稳定工具，再扩展工具元数据
- 先做文件式记忆，再考虑可选归档

这样改造出来的系统会更接近 Claude Code 的真实体验：流程清晰，但不会臃肿。

---

## 8. 对应到当前 `cpqclaw` 的代码落点

结合你现有工程结构，下面是最直接的映射关系。

### 8.1 已经具备的核心骨架

- `src/agent/react_agent_core.{h,cpp}`
  - 对应 Claude Code 的主 Agent Loop
  - 负责思考、调用工具、观察结果的循环

- `src/tool/i_tool.{h,cpp}`
  - 对应统一工具协议
  - 是后续工具元数据化的基础

- `src/tool/tool_registry.{h,cpp}`
  - 对应工具注册中心
  - 适合承接“统一工具生命周期”和工具分组

- `src/tool/tool_executor.{h,cpp}`
  - 对应工具执行器
  - 适合承接权限检查、结果标准化、异步执行

- `src/permission/permission_manager.{h,cpp}`
  - 对应轻量权限防线
  - 适合承接只读/确认/自动三档策略

- `src/agent/context_builder.{h,cpp}`
  - 对应上下文工程
  - 适合承接文件式记忆索引、最近文件、摘要拼装

- `src/memory/memory_manager.{h,cpp}`
  - 对应会话摘要与记忆索引
  - 建议保持“文件式记忆优先”，SQLite 只做可选归档

- `src/memory/conversation_history.{h,cpp}`
  - 对应会话历史存储
  - 适合保存原始消息与摘要引用

- `src/provider/provider_manager.{h,cpp}` 以及各 provider 实现
  - 对应模型提供方抽象
  - 适合统一流式接口与错误对象

- `src/application/agent_service.{h,cpp}` 与 `src/application/session_manager.{h,cpp}`
  - 对应应用编排层
  - 适合做 UI 与 Agent 核心之间的门面

- `src/ui/main_window.{h,cpp}` 与 QML/Widgets 相关文件
  - 对应展示层
  - 只负责展示、输入、确认和事件订阅

- `src/skill/skill_manager.{h,cpp}`
  - 对应轻量技能系统
  - 适合本地技能扫描和激活

### 8.2 最该优先改的几个文件

如果你想按最小风险推进，建议先改这几处：

1. `src/agent/react_agent_core.{h,cpp}`
   - 把主循环拆成“解析 / 执行 / 终止判断”三个明确阶段

2. `src/tool/tool_executor.{h,cpp}`
   - 把确认、执行、结果回传做成稳定事件流

3. `src/permission/permission_manager.{h,cpp}`
   - 明确三档权限与危险工具判定

4. `src/agent/context_builder.{h,cpp}`
   - 改成文件式记忆优先的上下文拼装器

5. `src/memory/memory_manager.{h,cpp}`
   - 让记忆索引只做轻量总结和文件引用，不把 SQLite 做成核心依赖

6. `src/provider/provider_manager.{h,cpp}` / provider 实现
   - 统一流式返回格式，减少 provider 差异对核心循环的影响

### 8.3 你现在最适合的改造顺序

1. 先让 `ReactAgentCore` 和 `ToolExecutor` 的事件流稳定
2. 再把 `PermissionManager` 和 `ContextBuilder` 调整到文件式记忆优先
3. 然后统一 provider 输出格式
4. 最后再补技能激活和更细的会话恢复

这样会得到一个“结构接近 Claude Code，但复杂度保持可控”的 Qt Agent。

---

## 9. 明确不建议现在做的内容

为了控制复杂度，以下内容建议暂时不做：

- 远程控制 / killswitch
- 插件市场
- 多人协作 swarm
- 复杂的远程 MCP 生态
- 大规模 feature flag 体系
- 超重型上下文折叠算法
- 完整的子代理编排平台

这些不是没价值，而是**对当前目标来说性价比不高**。

---

## 10. 最终建议

如果你的目标只是“把 Claude Code 的优秀点搬到 Claw++ 里”，那最合理的顺序是：

1. 先做 **Agent 主循环**
2. 再做 **统一工具协议**
3. 然后做 **权限与流式体验**
4. 接着做 **上下文与记忆**
5. 最后再做 **技能 / 子代理 / 高级扩展**

这样能保证：

- 改造成本可控
- 代码结构更清晰
- 功能迭代不失控
- 体验接近 Claude Code 的优点，但不会变成它的复杂度

---

## 10. 可直接开工的实施清单

下面这份清单是按你当前 `cpqclaw` 的代码结构拆出来的，目标是“先把骨架立住，再逐步增强”。

### 10.1 第一批：先把核心骨架立起来

1. **新增 AgentOrchestrator**
   - 位置建议：`src/application/`
   - 责任：
     - 接收用户输入
     - 组装上下文
     - 调用 provider
     - 处理工具调用
     - 产出流式事件

2. **收敛 QmlBackend**
   - 让它只做 UI 门面
   - 只暴露会话状态、发送消息、订阅事件
   - 不把业务逻辑塞回 QML 层

3. **统一消息/事件模型**
   - 建议补一组结构体或 `QJsonObject` 协议
   - 至少包含：
     - user message
     - assistant message
     - tool call
     - tool result
     - error event

4. **让 Provider 接口统一**
    - 现在已有多 provider，继续收拢成一致的请求/流式/错误接口
    - 这样 Agent 核心不会依赖具体厂商

当前状态：已落地（当前版本）

代码落点（已实现）：
- `src/application/agent_orchestrator.{h,cpp}`（轻量编排门面）
- `src/application/agent_service.{h,cpp}`（对外暴露 orchestrator）
- `src/ui/main_window.cpp`、`src/qml/qml_backend.cpp`（统一走 orchestrator）

### 10.2 第二批：工具系统标准化

5. **升级 ToolRegistry**
   - 给每个工具补 metadata
   - 统一工具输入输出格式
   - 标明 read-only / risky / confirm-required

6. **升级 ToolExecutor**
   - 所有工具走同一执行管线
   - 统一前置校验、错误包装、结果格式化

7. **接入 PermissionManager**
    - 把权限从“工具内部”移到“执行前策略层”
    - 对 shell、写文件、删除、网络访问先做最小化拦截

当前状态：已落地（当前版本）

代码落点（已实现）：
- `src/tool/i_tool.{h,cpp}`（元数据：并行/确认能力）
- `src/tool/tool_executor.{h,cpp}`（统一执行管线、权限前置、toolCompleted 事件）
- `src/permission/permission_manager.{h,cpp}`（策略层拦截）

### 10.3 第三批：上下文与记忆

8. **重做 ContextBuilder**
   - 组装 prompt 时只放“当前任务需要的信息”
   - 保留最近消息、最近文件、当前会话摘要

9. **增强 MemoryManager**
    - 原始历史单独存
    - 摘要单独存
    - 用户偏好单独存
    - 以文件式记忆索引为主，SQLite 仅作可选归档

10. **补会话恢复**
     - 支持重新打开继续对话
     - 支持查看工具历史

当前状态：已落地（当前版本）

代码落点（已实现）：
- `src/agent/context_builder.{h,cpp}`（上下文拼装与技能触发）
- `src/memory/memory_manager.{h,cpp}`（文件式记忆优先检索）
- `src/application/session_manager.{h,cpp}`（会话恢复与历史同步）

### 10.4 第四批：体验与工程化

11. **补流式 UI 事件**
    - QML/Widgets 实时显示 token
    - 工具调用和工具结果分段显示

12. **补结构化日志**
    - session id
    - request id
    - tool call id
    - provider error

13. **补最小测试**
     - 工具注册测试
     - 权限策略测试
     - 上下文构建测试
     - provider 接口测试

当前状态：已落地（当前版本）

已完成：
- QML/Widgets 工具事件可视化（系统提示 + 状态文本 + ToolCard 卡片化）
- Agent 思考过程可视化（Thinking / Action / Observation 分段样式）
- 流式工具预执行可视化（Pre-Execution 卡片 + 状态提示）
- request 级运行状态事件（`runStarted` / `runFinished` / `inputRejected`）
- 最小测试已纳入 `tests/test_core.cpp`（含 Agent 循环与错误分支新增用例）
- `pwsh` 环境下已完成完整构建与 `ctest` 回归

待完成：
- 结构化日志字段全链路统一（session/request/tool 三段关联）

### 10.5 推荐依赖顺序

```text
Provider 统一
    -> AgentOrchestrator
        -> ToolRegistry / ToolExecutor
            -> PermissionManager
                -> ContextBuilder / MemoryManager
                    -> UI 流式事件
```

这个顺序能保证每一步都能独立验收，不会一上来就改到全项目失控。

---

## 11. 补充改进建议（基于 2026-04 深度分析）

> 本节根据最新仓库分析（instructkr/claude-code、sanbuphy/claude-code-source-code、Windy3f3f3f3f/how-claude-code-works）与 Claw++ 当前实现对比，提炼出 **中等复杂度、高性价比** 的优化点。

---

### 11.1 **核心循环增强（优先级：高）**

#### ✅ 已实现基础
- ReAct 主循环（`ReactAgentCore`）
- 工具调用解析与执行
- 终止逻辑（无工具连续轮次、最大迭代保护）

#### 🔧 可优化点

**A. 原生函数调用支持（Function Calling）**
- **现状**：手动正则解析 LLM 输出中的工具调用标记（如 `<tool_use>` 等）
- **Claude Code 做法**：直接使用 Anthropic Messages API 的 `tools` 参数，返回结构化 `tool_use` 块
- **建议实现**：
  ```cpp
  // src/provider/i_llm_provider.h
  struct LLMRequest {
      QVector<Message> messages;
      QVector<ToolDefinition> tools;  // 新增：工具定义数组
      bool useNativeToolCalling = true;
  };
  
  // src/agent/react_agent_core.cpp
  void ReactAgentCore::prepareToolDefinitions() {
      auto tools = m_toolRegistry->getAllTools();
      for (auto tool : tools) {
          m_request.tools.append({
              .name = tool->name(),
              .description = tool->description(),
              .inputSchema = tool->schema()
          });
      }
  }
  ```
- **收益**：
  - 减少 Token 浪费（无需在 prompt 中长篇说明工具格式）
  - 提升解析可靠性（结构化 JSON vs 文本正则）
  - 更快的工具调用响应

**B. 流式工具预执行（Streaming Tool Pre-execution）**
- **现状**：等待完整 LLM 响应后才开始执行工具
- **Claude Code 做法**：在模型输出 `tool_use` 块的同时，边解析边启动工具（工具延迟 ~1s 可在模型生成 5-30s 窗口中隐藏）
- **建议实现**：
  ```cpp
  // src/agent/react_agent_core.cpp
  void ReactAgentCore::onStreamChunk(const StreamChunk& chunk) {
      if (chunk.type == ChunkType::ToolUse) {
          // 立即启动工具（异步非阻塞）
          QFuture<ToolResult> future = QtConcurrent::run([=]() {
              return m_executor->execute(chunk.toolName, chunk.arguments);
          });
          m_pendingTools[chunk.toolCallId] = future;
      }
      else if (chunk.type == ChunkType::Stop) {
          // 等待所有预执行工具完成
          for (auto& [id, future] : m_pendingTools) {
              future.waitForFinished();
          }
      }
  }
  ```
- **收益**：用户感知延迟降低 30-50%

**C. 7 种继续策略（Continue Sites）**
- **现状**：只有"停止"和"继续循环"两种状态
- **Claude Code 做法**：定义 7 种不同的继续原因，每种对应一种恢复路径
  - `MaxOutputTokensReached` → 切换到更大 token 限制模型重试
  - `ContextWindowExceeded` → 触发自动压缩再重试
  - `RateLimitError` → 指数退避重试
  - `ToolExecutionFailure` → 记录错误并继续（而非终止）
  - `UserInterrupt` → 保存状态待恢复
  - `NetworkError` → 本地缓存预测 + 后台重试
  - `NormalCompletion` → 正常结束
- **建议实现**（轻量版）：
  ```cpp
  enum class ContinueReason {
      NormalCompletion,
      MaxIterationsReached,
      TokenLimitReached,
      ContextOverflow,
      ToolFailure,
      UserStop
  };
  
  ContinueReason ReactAgentCore::shouldContinue() {
      if (m_userStopped) return ContinueReason::UserStop;
      if (m_iteration >= MAX_ITERATIONS) return ContinueReason::MaxIterationsReached;
      if (m_tokenCount > m_contextLimit) return ContinueReason::ContextOverflow;
      if (hasToolFailures()) return ContinueReason::ToolFailure;
      return ContinueReason::NormalCompletion;
  }
  
  void ReactAgentCore::handleContinue(ContinueReason reason) {
      switch(reason) {
          case ContextOverflow:
              m_contextBuilder->compress();  // 自动压缩
              return continueLoop();
          case ToolFailure:
              appendSystemMessage("Previous tool failed, attempting alternative approach...");
              return continueLoop();
          default:
              return finalize();
      }
  }
  ```
- **收益**：从"遇错即停"到"静默恢复"，用户体验平滑

---

### 11.2 **上下文工程（优先级：高）**

#### ✅ 已实现基础
- 自动压缩（保留头尾消息）
- Token 计数
- 历史记忆检索

#### 🔧 可优化点

**D. 四级渐进式压缩（Progressive Compression）**
- **现状**：一刀切压缩（直接删除中间消息）
- **Claude Code 做法**：分 4 级渐进处理
  1. **裁剪**（Trim）：截断旧工具输出的大块内容（如 `read_file` 结果只保留前 500 字符）
  2. **去重**（Dedupe）：合并重复的工具调用结果
  3. **折叠**（Fold）：标记不活跃消息段但不删除（可展开）
  4. **摘要**（Summarize）：启动子 Agent 对整段对话做语义摘要
- **建议实现**：
  ```cpp
  // src/memory/memory_manager.cpp
  void MemoryManager::smartCompress() {
      int savedTokens = 0;
      
      // Level 1: Trim large tool outputs
      savedTokens += trimOldToolResults();
      if (isWithinLimit()) return;
      
      // Level 2: Remove duplicate file reads
      savedTokens += deduplicateToolCalls();
      if (isWithinLimit()) return;
      
      // Level 3: Fold inactive conversation segments
      savedTokens += foldInactiveSegments();
      if (isWithinLimit()) return;
      
      // Level 4: Semantic summarization (expensive)
      summarizeOldConversation();
  }
  ```
- **收益**：每一级可能就释放足够空间，避免昂贵的摘要操作

**E. 压缩后自动恢复（Post-Compact Recovery）**
- **现状**：压缩后立即开始新推理
- **Claude Code 做法**：压缩后自动恢复最近编辑的 5 个文件内容到上下文
- **建议实现**：
  ```cpp
  void MemoryManager::recoverAfterCompression() {
      auto recentFiles = getRecentEditedFiles(5);
      for (const auto& file : recentFiles) {
          appendSystemMessage(QString("Recently edited: %1\n%2")
              .arg(file.path)
              .arg(file.excerpt));  // 只放前 20 行
      }
  }
  ```
- **收益**：防止模型"失忆"（不记得刚才在改哪个文件）

**F. Prompt 缓存策略（Prompt Caching）**
- **Claude Code 使用**：Anthropic 提供 5 分钟 Prompt 缓存，系统提示词标记为 `cache_control: {type: "ephemeral"}`
- **建议实现**：
  ```cpp
  // src/agent/context_builder.cpp
  Message ContextBuilder::buildSystemPrompt() {
      Message systemMsg;
      systemMsg.role = "system";
      systemMsg.content = assembleSystemPrompt();
      
      // Anthropic 专用缓存标记
      if (m_provider->type() == "anthropic") {
          systemMsg.metadata["cache_control"] = QJsonObject{
              {"type", "ephemeral"}
          };
      }
      return systemMsg;
  }
  ```
- **收益**：系统提示词占用 2000+ tokens，缓存命中可省 90% 费用

---

### 11.3 **工具系统增强（优先级：中）**

#### ✅ 已实现基础
- 10 个内置工具
- 工具元数据（并行、确认标志）
- 权限管理器

#### 🔧 可优化点

**G. 读工具自动并行（Auto-Parallel Read Tools）**
- **现状**：`canRunInParallel()` 方法已定义但未在 `ToolExecutor` 中使用
- **Claude Code 做法**：同一轮中所有 read-only 工具（`read_file`, `glob`, `grep`）并行执行
- **建议实现**：
  ```cpp
  // src/tool/tool_executor.cpp
  QVector<ToolResult> ToolExecutor::executeBatch(const QVector<ToolCall>& calls) {
      QVector<ToolCall> safeTools, riskyTools;
      
      for (const auto& call : calls) {
          auto tool = m_registry->getTool(call.name);
          if (tool->canRunInParallel()) {
              safeTools.append(call);
          } else {
              riskyTools.append(call);
          }
      }
      
      // Parallel execution of safe tools
      QVector<QFuture<ToolResult>> futures;
      for (const auto& call : safeTools) {
          futures.append(QtConcurrent::run([=]() {
              return execute(call.name, call.arguments);
          }));
      }
      
      QVector<ToolResult> results;
      for (auto& future : futures) {
          results.append(future.result());
      }
      
      // Serial execution of risky tools
      for (const auto& call : riskyTools) {
          results.append(execute(call.name, call.arguments));
      }
      
      return results;
  }
  ```
- **收益**：读取 5 个文件从串行 5 秒降至并行 1 秒

**H. 工具输出大小限制与自动摘要**
- **现状**：大文件内容全部塞入上下文
- **Claude Code 做法**：超过 100K 字符的工具输出自动写入临时文件，模型只拿到摘要和路径
- **建议实现**：
  ```cpp
  ToolResult ReadFileTool::execute(const QJsonObject& args) {
      QString content = readFileContent(args["path"].toString());
      
      if (content.size() > 100000) {
          QString tempPath = saveToCacheFile(content);
          return ToolResult{
              .success = true,
              .output = QString("File too large (%1 chars). Saved to %2\n"
                               "Summary: %3")
                        .arg(content.size())
                        .arg(tempPath)
                        .arg(summarize(content, 500))  // 前500字符摘要
          };
      }
      return ToolResult{.success = true, .output = content};
  }
  ```
- **收益**：防止单次读取大文件直接炸上下文

**I. 工具调用去重检测**
- **Claude Code 做法**：检测连续相同工具调用（如连续 3 次 `read_file("config.json")`），自动拒绝并提示"already read"
- **建议实现**：
  ```cpp
  bool ToolExecutor::isDuplicateCall(const QString& name, const QJsonObject& args) {
      QString callSignature = QString("%1:%2").arg(name, QJsonDocument(args).toJson());
      
      if (m_recentCalls.contains(callSignature)) {
          int count = m_recentCalls[callSignature];
          if (count >= 2) {
              emit toolCompleted(name, ToolResult{
                  .success = false,
                  .output = "Duplicate tool call detected. File already read in previous turn."
              });
              return true;
          }
      }
      m_recentCalls[callSignature]++;
      return false;
  }
  ```
- **收益**：防止模型陷入"重复读同一文件"的死循环

---

### 11.4 **安全与权限强化（优先级：中）**

#### ✅ 已实现基础
- 三级权限（Safe/Moderate/Dangerous）
- 用户确认弹窗
- PermissionManager 策略层

#### 🔧 可优化点

**J. Bash 命令深度分析（AST-based Safety Check）**
- **现状**：基于规则的黑名单匹配（如拦截 `rm -rf`）
- **Claude Code 做法**：使用 `tree-sitter` 语法树分析 Bash 命令，进行 23 项安全检查
  - 命令注入检测
  - 环境变量泄露检测
  - Glob 炸弹检测（`*/**/*`）
  - 重定向到危险位置（`> /dev/sda`）
  - 后台进程检测（`&`, `nohup`）
- **Qt C++ 实现建议**（轻量版）：
  ```cpp
  // src/permission/bash_analyzer.h
  class BashAnalyzer {
  public:
      struct SafetyIssue {
          QString type;     // "command_injection", "dangerous_path", etc
          QString severity; // "critical", "warning"
          QString message;
      };
      
      QVector<SafetyIssue> analyze(const QString& command) {
          QVector<SafetyIssue> issues;
          
          // Check 1: Dangerous commands
          if (containsAny(command, {"rm -rf /", "dd if=", "mkfs", ":(){ :|:& };:"})) {
              issues.append({
                  .type = "dangerous_command",
                  .severity = "critical",
                  .message = "Potentially destructive operation detected"
              });
          }
          
          // Check 2: Command injection patterns
          if (command.contains(QRegularExpression(R"([\$`]\(.*\)|[;&|].*)"))) {
              issues.append({
                  .type = "command_injection",
                  .severity = "warning",
                  .message = "Command chaining or substitution detected"
              });
          }
          
          // Check 3: Sensitive paths
          QStringList sensitivePaths = {"/", "/etc", "/usr", "/bin", "/boot"};
          for (const auto& path : sensitivePaths) {
              if (command.contains(path)) {
                  issues.append({
                      .type = "sensitive_path",
                      .severity = "warning",
                      .message = QString("Operation on system path: %1").arg(path)
                  });
              }
          }
          
          return issues;
      }
  };
  ```
- **收益**：从"黑名单拦截"升级到"行为分析"，更难绕过

**K. 竞速确认机制（Race-safe Confirmation）**
- **Claude Code 做法**：用户确认弹窗有 200ms 防抖保护，防止键盘连击导致误确认
- **建议实现**：
  ```cpp
  // src/ui/permission_dialog.cpp
  PermissionDialog::PermissionDialog(QWidget* parent) : QDialog(parent) {
      // 延迟启用确认按钮
      m_confirmButton->setEnabled(false);
      QTimer::singleShot(200, [this]() {
          m_confirmButton->setEnabled(true);
      });
  }
  ```
- **收益**：防止用户在打字时误按回车确认危险操作

---

### 11.5 **用户体验优化（优先级：中低）**

#### 🔧 可优化点

**L. 启动性能优化（9 阶段并行启动）**
- **Claude Code 做法**：9 个不相关的初始化任务并行执行，关键路径压到 235ms
  - 并行：加载配置、初始化 provider、加载工具、读取会话历史
  - 串行：GUI 初始化 → 主窗口显示
- **建议实现**：
  ```cpp
  // src/main.cpp
  void Application::asyncInit() {
      QVector<QFuture<void>> initTasks;
      
      initTasks.append(QtConcurrent::run([]() {
          ProviderManager::instance()->loadProviders();
      }));
      initTasks.append(QtConcurrent::run([]() {
          ToolRegistry::instance()->registerBuiltinTools();
      }));
      initTasks.append(QtConcurrent::run([]() {
          SessionManager::instance()->loadRecentSessions();
      }));
      
      // 等待所有任务完成
      for (auto& task : initTasks) {
          task.waitForFinished();
      }
      
      // 显示主窗口
      m_mainWindow->show();
  }
  ```
- **收益**：启动时间从 1 秒降至 300ms

**M. 消息流式渲染优化**
- **现状**：每个 token 触发一次完整 UI 重绘
- **建议优化**：
  ```cpp
  // src/ui/chat_view.cpp
  void ChatView::onStreamToken(const QString& token) {
      m_accumulatedTokens.append(token);
      
      // 每 50ms 或累积 10 个 token 才重绘一次
      if (!m_renderTimer->isActive()) {
          m_renderTimer->start(50);
      }
  }
  
  void ChatView::flushAccumulatedTokens() {
      if (m_accumulatedTokens.isEmpty()) return;
      
      m_currentMessageWidget->appendText(m_accumulatedTokens);
      m_accumulatedTokens.clear();
  }
  ```
- **收益**：CPU 占用降低 60%，滚动更流畅

**N. 会话持久化优化（JSONL 追加式写入）**
- **Claude Code 做法**：每条消息追加写入 `<session-id>.jsonl`，而非覆盖整个 JSON 文件
- **建议实现**：
  ```cpp
  void SessionManager::appendMessage(const Message& msg) {
      QString filePath = QString("%1/%2.jsonl")
          .arg(m_sessionDir, m_currentSessionId);
      
      QFile file(filePath);
      if (file.open(QIODevice::Append | QIODevice::Text)) {
          QJsonDocument doc(msg.toJson());
          file.write(doc.toJson(QJsonDocument::Compact));
          file.write("\n");
      }
  }
  
  QVector<Message> SessionManager::loadSession(const QString& id) {
      QVector<Message> messages;
      QFile file(QString("%1/%2.jsonl").arg(m_sessionDir, id));
      
      if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
          while (!file.atEnd()) {
              QByteArray line = file.readLine();
              messages.append(Message::fromJson(QJsonDocument::fromJson(line).object()));
          }
      }
      return messages;
  }
  ```
- **收益**：
  - 崩溃恢复（追加式写入不会丢失历史）
  - 性能提升（无需每次序列化整个会话）

---

### 11.6 **可选高级特性（优先级：低，按需实现）**

#### 🔧 可选点

**O. MCP 协议支持（Model Context Protocol）**
- **用途**：连接第三方工具服务器（如 Brave Search、Filesystem、Database）
- **复杂度**：中等（需实现 JSON-RPC 客户端 + 5 种传输：stdio / HTTP / SSE / WebSocket / in-process）
- **建议**：暂不实现，优先做好内置工具

**P. 子 Agent 系统（Hierarchical Agents）**
- **用途**：主 Agent 分派子任务给子 Agent（如"压缩对话"、"搜索代码"）
- **复杂度**：高（需实现任务队列、Agent 间消息传递、Worktree 隔离）
- **建议**：当前 `SubagentTool` 已有基础，暂不扩展为完整编排系统

**Q. 技能系统（Skills）**
- **用途**：用户自定义工作流（如"git commit 前自动 lint"）
- **复杂度**：中（需定义技能格式、加载器、触发器）
- **建议**：可用 Qt 插件系统（QPluginLoader）轻量实现

**R. 遥测与分析（Telemetry）**
- **用途**：收集使用数据、错误日志、性能指标
- **复杂度**：低（发送 HTTP POST 到自己的服务器）
- **建议**：做本地日志即可，不做云端上传

---

### 11.7 GUI 同步改进建议

> 以下改进需同步修改 Widgets 和 QML 界面

#### 🎨 UI 优化点

**S. 工具调用可视化增强**
- **当前状态**：工具事件显示为系统提示文本
- **建议增强**：
  ```qml
  // src/qml/components/ToolCallCard.qml
  Rectangle {
      width: parent.width
      height: column.height + 20
      color: "#f0f4f8"
      radius: 8
      
      Column {
          id: column
          spacing: 8
          
          Row {
              spacing: 10
              Image {
                  source: toolIcon(toolName)  // 工具图标
                  width: 24; height: 24
              }
              Text {
                  text: toolName
                  font.bold: true
              }
              Text {
                  text: status  // "Running", "Success", "Failed"
                  color: statusColor(status)
              }
          }
          
          // 可折叠的输入参数
          Collapsible {
              title: "Arguments"
              content: JSON.stringify(arguments, null, 2)
          }
          
          // 可折叠的输出结果
          Collapsible {
              title: "Result"
              content: resultText
              visible: status === "Success"
          }
      }
  }
  ```

**T. Agent 思考过程可视化**
- **建议**：在 UI 中分段显示
  - **Thinking**：模型的思考过程（灰色气泡）
  - **Action**：工具调用（蓝色卡片）
  - **Observation**：工具结果（绿色/红色边框）
  - **Answer**：最终回答（白色气泡）
  
  ```cpp
  // src/ui/chat_view.cpp
  void ChatView::appendThinkingSegment(const QString& thought) {
      auto widget = new MessageBubble(MessageRole::System, this);
      widget->setIcon(":/icons/brain.svg");
      widget->setTitle("Thinking");
      widget->setContent(thought);
      widget->setStyleSheet("background-color: #f5f5f5; font-style: italic;");
      m_messageLayout->addWidget(widget);
  }
  ```

**U. 上下文状态指示器**
- **建议**：在状态栏显示
  ```cpp
  // src/ui/main_window.cpp
  m_contextIndicator = new QLabel(this);
  m_contextIndicator->setToolTip("Current context usage");
  statusBar()->addPermanentWidget(m_contextIndicator);
  
  void MainWindow::updateContextIndicator(int tokenCount, int limit) {
      double percentage = (double)tokenCount / limit * 100;
      QString color = percentage > 90 ? "red" : (percentage > 70 ? "orange" : "green");
      m_contextIndicator->setText(QString("🧠 %1/%2 (%3%)")
          .arg(tokenCount).arg(limit).arg(percentage, 0, 'f', 1));
      m_contextIndicator->setStyleSheet(QString("color: %1").arg(color));
  }
  ```

**V. 快捷操作面板**
- **建议**：添加常用操作快捷按钮
  ```qml
  Row {
      spacing: 10
      
      ToolButton {
          icon.name: "document-open"
          text: "Load Files"
          onClicked: fileDialog.open()
      }
      
      ToolButton {
          icon.name: "edit-clear"
          text: "Clear Context"
          onClicked: backend.clearContext()
      }
      
      ToolButton {
          icon.name: "view-history"
          text: "Show Memory"
          onClicked: memoryDialog.open()
      }
      
      ToolButton {
          icon.name: "document-save"
          text: "Export Session"
          onClicked: backend.exportSession()
      }
  }
  ```

---

### 11.8 实施优先级矩阵

| 功能 | 复杂度 | 收益 | 优先级 | 建议阶段 |
|------|--------|------|--------|----------|
| **A. 原生函数调用** | 中 | 高 | ⭐⭐⭐⭐⭐ | 第一批 |
| **C. 7 种继续策略** | 中 | 高 | ⭐⭐⭐⭐⭐ | 第一批 |
| **D. 四级渐进压缩** | 高 | 高 | ⭐⭐⭐⭐ | 第二批 |
| **G. 自动并行工具** | 低 | 中 | ⭐⭐⭐⭐ | 第二批 |
| **B. 流式预执行** | 中 | 中 | ⭐⭐⭐ | 第二批 |
| **F. Prompt 缓存** | 低 | 中 | ⭐⭐⭐ | 第二批 |
| **H. 工具输出限制** | 低 | 中 | ⭐⭐⭐ | 第二批 |
| **J. Bash AST 分析（当前为规则版）** | 中 | 中 | ⭐⭐ | 第三批 |
| **L. 并行启动优化（轻量版）** | 低 | 低 | ⭐⭐ | 第三批（已完成） |
| **S-V. GUI 增强** | 中 | 中 | ⭐⭐ | 第三批（核心项已完成） |
| **E. 压缩后恢复** | 低 | 低 | ⭐ | 可选 |
| **I. 工具去重** | 低 | 低 | ⭐ | 可选 |
| **K. 防抖确认** | 低 | 低 | ⭐ | 可选 |
| **M. 流式渲染优化** | 中 | 低 | ⭐ | 可选 |
| **N. JSONL 持久化** | 低 | 低 | ⭐ | 可选 |
| **O-R. 高级特性** | 极高 | 低 | ❌ | 不建议 |

---

### 11.9 推荐实施路径（分 3 批）

#### **第一批：核心循环质量跃升**（2-3 天工作量）
1. **原生函数调用**（A）：改造 Provider 接口，支持 `tools` 参数
2. **7 种继续策略**（C）：定义 `ContinueReason` 枚举并实现恢复逻辑
3. **测试验证**：确保工具调用成功率从 85% 提升至 95%+

#### **第二批：性能与智能优化**（3-4 天工作量）
4. **四级渐进压缩**（D）：实现 Trim → Dedupe → Fold → Summarize 流水线
5. **自动并行工具**（G）：修改 `ToolExecutor::executeBatch()` 支持并发
6. **Prompt 缓存**（F）：为 Anthropic Provider 添加缓存标记
7. **工具输出限制**（H）：大文件自动摘要

#### **第三批：用户体验打磨**（2-3 天工作量）
8. **GUI 增强**（S-V）：工具卡片、思考过程、上下文指示器
9. **Bash 分析**（J）：实现轻量版 23 项检查
10. **启动优化**（L）：并行初始化

当前进度（2026-04）：
- 已完成：S（工具卡片化）、T（思考过程可视化）、V（快捷操作面板）、L（轻量并行启动优化）
- 已完成（轻量版）：J（规则匹配安全分析 + 风险拦截 + GUI 提示）
- 待继续：U（上下文状态指示器在 QML 侧进一步增强展示）

---

### 11.10 不建议实现的特性（保持中等复杂度）

❌ **完整 MCP 协议栈**：5 种传输 + OAuth 认证过于复杂  
❌ **Swarm 多 Agent 编排**：任务队列、消息传递、Worktree 隔离成本高  
❌ **技能市场与发现**：需要中心化服务器  
❌ **遥测云端上传**：隐私问题 + 维护成本  
❌ **完整压缩 Agent**：专门的子 Agent 做摘要，过度工程化  
❌ **Bridge 远程执行**：需要 JWT 认证 + 进程管理  

---

### 11.11 成功标准

实施完成后，Claw++ 应达到：

✅ **功能层面**
- 工具调用成功率 ≥ 95%（原生函数调用）
- 上下文溢出自动恢复率 100%（渐进压缩）
- 危险命令拦截率 ≥ 90%（Bash 分析）

✅ **性能层面**
- 启动时间 ≤ 500ms（并行初始化）
- 工具执行延迟 -30%（并行 + 预执行）
- Token 费用 -40%（Prompt 缓存）

✅ **体验层面**
- 用户可见工具执行过程（卡片可视化）
- 错误自动恢复无需用户干预（7 种继续策略）
- 上下文状态清晰可见（指示器）

✅ **代码质量**
- 模块边界清晰（Agent / Tool / Memory 独立）
- 单元测试覆盖率 ≥ 60%
- 无循环依赖

---

## 12. 总结

本次补充分析基于三个 Claude Code 仓库的深度研究，提炼出 **17 项可行优化**（A-V），按优先级分为 3 批实施：

**第一批（必做）**：原生函数调用、继续策略 → **质量跃升**  
**第二批（推荐）**：渐进压缩、并行工具、缓存优化 → **性能翻倍**  
**第三批（加分）**：GUI 增强、安全分析 → **体验打磨**（目前已完成核心闭环）

保持 **中等复杂度** 原则：
- ✅ 吸收核心工程模式（流式、压缩、并发、安全）
- ✅ 保持 Qt6 原生优势（信号槽、并发库、跨平台）
- ❌ 不做过度抽象（MCP、Swarm、遥测云端）

实施完成后，Claw++ 将从"能用的 AI 工具"升级为 **"工业级 Qt6 AI Agent"**，架构清晰、性能优异、体验流畅，但复杂度仍保持在个人/小团队可维护范围内。

