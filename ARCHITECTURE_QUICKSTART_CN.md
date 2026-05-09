# Claw++ 二次开发速查（中文）

> 目标：30 分钟内建立可修改心智模型，避免先读全量代码。

## 📘 详细阅读路线

- PROJECT_READING_PATH_CN.md（按阶段/按模块/按实战任务的完整阅读路径）
- FUNCTION_LEVEL_READING_TASKS_CN.md（函数级阅读任务单）
- POST_CLASS_EXECUTION_PLAN_CN.md（复课后执行计划）
- TOOLS_LAYER_GUIDE_CN.md（工具层专项开发指南）
- TOOLS_BOUNDARY_TEST_CHECKLIST_CN.md（工具层边界测试清单）
- PROVIDER_MEMORY_GUIDE_CN.md（Provider 与 Memory 改造指南）
- UI_PROVIDER_MEMORY_REFACTOR_NOTES_CN.md（UI/Provider/Memory 深化改造说明）
- TESTING_QUICKSTART_CN.md（测试快速上手）

## 1. 先看什么（15 分钟）

1. src/common/types.h
2. src/application/agent_service.h
3. src/agent/react_agent_core.cpp
4. src/tool/tool_executor.cpp
5. src/memory/memory_manager.cpp
6. src/provider/openai_provider.cpp

这 6 个文件能串起 80% 的核心执行链路。

## 2. 核心调用链（从 UI 到模型）

1. UI 输入触发：ChatView::onSend -> messageSubmitted
2. 业务入口：AgentService::sendMessage
3. 上下文构建：ContextBuilder::buildSystemPromptForInput + buildRuntimeContext
4. Agent 循环：ReactAgentCore::run -> processIteration
5. 模型请求：ProviderManager/OpenAIProvider -> HttpClient
6. 工具执行：ReactAgentCore::executeToolCalls -> ToolExecutor::execute
7. 权限检查：PermissionManager::checkPermission
8. 内存落盘：MemoryManager::addMessage / compress

## 3. 模块职责速记

- application：业务编排层（会话、消息、调用顺序）
- agent：ReAct 推理循环与上下文拼接
- provider：大模型接入与流式解析
- tool：工具注册、权限判断、执行
- memory：会话上下文与长期记忆文件
- permission：策略规则 + 白名单 + 缓存
- infrastructure/network：HTTP/SSE、超时与重试

## 4. 改需求时怎么定位

- 改“提示词结构/系统设定”：
  - src/agent/context_builder.cpp
  - src/application/template_loader.cpp

- 改“回复行为/迭代策略/工具时机”：
  - src/agent/react_agent_core.cpp

- 改“会话管理/导入导出/重命名规则”：
  - src/application/session_manager.cpp

- 改“工具权限策略”：
  - src/permission/permission_manager.cpp

- 改“网络容错/重试/超时”：
  - src/infrastructure/network/http_client.cpp

- 改“记忆压缩策略”：
  - src/memory/memory_manager.cpp
  - src/memory/conversation_history.cpp

## 5. 当前建议先别动的大风险区域

- react_agent_core.cpp 中的主迭代控制（影响全局行为）
- external_platform_manager.*（平台侧适配仍可能调整）
- provider_manager 的多 Provider 架构（后续扩展点）

## 6. 这次已做的关键质量改进（便于你续改）

- ToolResult 增加默认初始化，减少未赋值结果对象风险
- EventBus 增加 bad_any_cast 防护，避免单个订阅器导致崩溃
- MemoryManager 去掉冗余手动 delete（Qt 父子对象自动托管）
- SessionManager 复制会话后缀常量化
- ToolExecutor 增加未知异常捕获，并统一权限超时时间常量
- PermissionManager 修复通配符到正则转换逻辑
- ConversationHistory 摘要拼接增加预分配，减少重复扩容
- ReactAgentCore 补充关键迭代日志，便于排查“为什么停/为什么调工具”

## 7. 推荐二次开发流程

1. 先改一处最小闭环（如权限规则或一个工具）
2. 构建验证：cmake --build cpqclaw/build
3. 跑一轮核心路径：发送消息 -> 工具调用 -> 会话切换
4. 再改高影响区域（React 循环、上下文压缩）

---

如果你希望，我下一步可以继续把 src/tool/tools 下每个具体工具文件也补一版中文注释（read/edit/write/search/shell/system/subagent）。
