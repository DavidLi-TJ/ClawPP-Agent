# Plan 实现差距报告（第三节）

更新时间：2026-03-29
范围：`c:\david\Qt\plan.txt` 第三节（3.1 ~ 3.5）

## 状态定义
- 已实现：核心能力已在代码中落地且可用
- 半实现：有基础实现，但与 plan 目标存在明显差距
- 未实现：当前代码中未找到对应实现

## 3.1 Agent Core
- 已实现
- `IAgentCore` 接口与 `ReactAgentCore` 主循环已存在
- 最大迭代限制已实现（`MAX_ITERATIONS = 10`）
- 流式输出、工具调用增量解析、完成/错误信号已接通
- 半实现
- `injectSkill(skill)` 接口未在 `IAgentCore` 中实现
- 终止条件仅覆盖“无工具调用/达到迭代上限/停止”，未实现 plan 中 `FINAL_ANSWER` 标记策略
- ReAct 过程结构存在，但未拆成 plan 中的独立 `parseResponse/executeTool/checkTermination` 级别抽象
- 已补强（本轮）
- 新增“按输入自动激活 skill 指令注入”，由 `ContextBuilder::buildSystemPromptForInput()` 提供

## 3.2 Tool System
- 已实现
- `ITool` / `ToolRegistry` / `ToolExecutor` 主体完整
- 内置工具链已注册（读写文件、搜索、shell、network、subagent 等）
- `start_subagent` 已可执行
- 半实现
- `Subagent` 结果 metadata 未完整覆盖 plan 示例中的 `iterations/toolsUsed`
- 未实现
- `FileSystemSandbox` 独立沙箱类未落地（目前以工具与权限策略为主）
- `ToolRegistry::loadFromConfig/loadFromDirectory` 的插件式工具加载未落地

## 3.3 Memory System
- 已实现
- 会话消息持久化（SQLite）
- token 估算、阈值检测、中段压缩（`keep_first/keep_last`）
- `MEMORY.md/HISTORY.md` 文件化长期记录
- 半实现
- 压缩策略为本地摘要拼接，不是 plan 中 `ContextCompressor + LLM` 专用模块
- 长期记忆是 markdown 文本累积，不是 plan 中结构化条目（importance/access_count/decay）
- 未实现
- `LongTermMemoryStore` 独立类（JSON schema + 检索）
- `ContextCompressor` 独立类与可替换策略接口

## 3.4 Provider Manager
- 已实现
- `ILLMProvider` 抽象
- `ProviderManager` + `OpenAIProvider`（含 SSE 流式解析）
- 半实现
- 管理器接口是多 provider 形态，但当前仅注册 OpenAI
- 未实现
- `ClaudeProvider` / `DeepSeekProvider`
- plan 中“统一多提供商格式转换层”的完整覆盖（目前主要是 OpenAI 兼容形态）

## 3.5 Skill Manager
- 已实现
- Skill 解析（`SKILL.md` frontmatter + content）
- 目录递归加载
- 文件监控热重载（`QFileSystemWatcher`）
- skills summary 注入 system prompt
- 按输入触发词自动激活 skill（动态追加 skill 指令）
- 按 skill.tools 对本轮可用工具进行基础白名单裁剪
- 半实现
- 触发机制仍是规则匹配（trigger contains），未实现 plan 里的意图匹配层
- 工具裁剪仅在“skill 明确声明 tools”时生效；无声明时仍回退全部工具
- 未实现
- plan 中意图匹配（LLM intent）
- Skill 市场/远程仓库集成

## 与你当前优先级对应的关键缺口
- P1: skills 自动触发注入（已完成）
- P2: skills 动态工具白名单（已完成基础版）
- P3: memory 独立压缩模块化（`ContextCompressor`）
- P4: 多 provider 扩展（Claude/DeepSeek）

## 额外说明：exe 打不开
- 本地自动化检查中，`build/bin/ClawPP.exe` 可启动并保持运行。
- 建议优先确认你启动的是 `c:\david\Qt\cpqclaw\build\bin\ClawPP.exe`，而不是顶层 demo 工程的输出。