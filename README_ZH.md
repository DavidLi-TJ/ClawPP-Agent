<p align="center">
  <a href="README.md"><img src="https://img.shields.io/badge/🇬🇧_English-blue?style=for-the-badge" alt="English"/></a>
  <a href="README_ZH.md"><img src="https://img.shields.io/badge/🇨🇳_中文-red?style=for-the-badge" alt="中文"/></a>
</p>

***

<p align="center">
  <img src="images/icon.png" alt="Claw++ Icon" width="128" height="128"/>
</p>

<h1 align="center">Claw++ — AI Agent 桌面应用</h1>

<p align="center">
  <a href="https://github.com/DavidLi-TJ/ClawPP-Agent/stargazers">
    <img src="https://img.shields.io/github/stars/DavidLi-TJ/ClawPP-Agent?style=for-the-badge&logo=starship&color=orange&logoColor=white&labelColor=1A1B26" alt="stars"/>
  </a>
  <a href="https://github.com/DavidLi-TJ/ClawPP-Agent/network/members">
    <img src="https://img.shields.io/github/forks/DavidLi-TJ/ClawPP-Agent?style=for-the-badge&logo=git&color=blue&logoColor=white&labelColor=1A1B26" alt="forks"/>
  </a>
  <a href="https://github.com/DavidLi-TJ/ClawPP-Agent/issues">
    <img src="https://img.shields.io/github/issues/DavidLi-TJ/ClawPP-Agent?style=for-the-badge&logo=github&color=red&logoColor=white&labelColor=1A1B26" alt="issues"/>
  </a>
  <a href="https://github.com/DavidLi-TJ/ClawPP-Agent/blob/main/LICENSE">
    <img src="https://img.shields.io/github/license/DavidLi-TJ/ClawPP-Agent?style=for-the-badge&color=green&logoColor=white&labelColor=1A1B26" alt="license"/>
  </a>
  <a href="https://github.com/DavidLi-TJ/ClawPP-Agent">
    <img src="https://img.shields.io/github/last-commit/DavidLi-TJ/ClawPP-Agent?style=for-the-badge&color=purple&logo=git&logoColor=white&labelColor=1A1B26" alt="last commit"/>
  </a>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Qt-6.5+-green?style=for-the-badge&logo=qt&logoColor=white" alt="Qt 6.5+"/>
  <img src="https://img.shields.io/badge/C%2B%2B-17-blue?style=for-the-badge&logo=cplusplus&logoColor=white" alt="C++ 17"/>
  <img src="https://img.shields.io/badge/CMake-3.20+-orange?style=for-the-badge&logo=cmake&logoColor=white" alt="CMake 3.20+"/>
  <img src="https://img.shields.io/badge/SQLite-3.x-purple?style=for-the-badge&logo=sqlite&logoColor=white" alt="SQLite"/>
</p>

<p align="center">
  <img src="https://img.shields.io/github/repo-size/DavidLi-TJ/ClawPP-Agent?style=for-the-badge&color=yellow&logo=hackthebox&logoColor=white&labelColor=1A1B26" alt="repo size"/>
  <img src="https://img.shields.io/github/contributors/DavidLi-TJ/ClawPP-Agent?style=for-the-badge&color=blue&logo=github&logoColor=white&labelColor=1A1B26" alt="contributors"/>
</p>

***

## 📖 目录

- [📖 目录](#-目录)
- [项目简介](#项目简介)
  - [🌐 支持的模型服务商](#-支持的模型服务商)
- [🚀 功能特性](#-功能特性)
- [🏗️ 架构设计](#️-架构设计)
- [🛠️ 技术栈](#️-技术栈)
- [📸 项目截图](#-项目截图)
- [📦 快速开始](#-快速开始)
  - [📋 环境要求](#-环境要求)
  - [🔨 构建步骤](#-构建步骤)
  - [▶️ 运行](#️-运行)
- [🌐 外部平台接入](#-外部平台接入)
- [📁 项目结构](#-项目结构)
- [🔧 开发指南](#-开发指南)
  - [添加新工具](#添加新工具)
  - [添加新 Provider](#添加新-provider)
  - [调试技巧](#调试技巧)
- [🎓 关于项目](#-关于项目)
- [📊 项目统计](#-项目统计)
- [📜 许可证](#-许可证)
- [💡 致谢](#-致谢)

***

## 项目简介

> **Claw++** 是一个基于 Qt/C++ 的现代化 AI Agent 桌面应用程序，专为 Windows 平台打造。它实现了完整的 ReAct（Reasoning + Acting）推理循环，支持 **23 种模型服务商**（涵盖 OpenAI、Anthropic、Gemini、DeepSeek、智谱等国内外主流平台），并提供丰富的工具调用能力和智能记忆管理。项目采用 Qt Quick 6 构建高颜值 Windows 11 风格玻璃质感界面，支持实时流式对话、会话管理、上下文压缩、技能系统等企业级功能。

- 🤖 **ReAct 推理引擎**：思考-行动-观察循环，支持多轮自主推理与工具调用
- 🔧 **9 大内置工具**：文件读写、Shell 命令、网络请求、子代理、搜索等
- 💬 **实时流式对话**：基于 SSE 的流式响应，Markdown 渲染，代码高亮
- 🔐 **三级权限系统**：Safe / Moderate / Dangerous，Shell 风险评分 1-4 级
- 🧠 **四级记忆压缩**：Trim → Dedupe → Fold → Summarize 渐进式压缩管道
- 🎯 **技能插件系统**：Markdown 格式技能定义，YAML 元数据，运行时热加载
- **Win11 玻璃质感 UI**：毛玻璃面板、液态按钮、涟漪效果、流畅动画
- 🎨 **极致个性化 UI**：自定义背景图片、毛玻璃模糊程度、行距、圆角、阴影深度，你的界面你做主
- 🪶 **轻量小巧**：C++ 原生编译，安装包体积 < 100MB，内存占用极低，告别 Electron 的沉重
- 🌐 **23 种模型服务商**：全面覆盖国内外主流大模型平台
- 💾 **SQLite 持久化**：会话、消息、记忆全量存储，支持导入导出

### 🌐 外部平台当前状态

- 已真正实现双向接入：`Telegram`、`Feishu`

也就是说，当前版本里：

- `Telegram`：支持轮询收消息、自动建会话、把模型回复发回 Telegram
- `Feishu`：支持本地 HTTP 回调收消息、签名校验、自动建会话、把模型回复发回飞书

### ✨ 为什么选择 Claw++？

> 厌倦了那些动辄几百 MB、占满内存的 AI 聊天工具？Claw++ 用 C++ 原生编译，安装包不到 100MB，运行流畅不卡顿。
>
> 不想用千篇一律的界面？Claw++ 把控制权交还给你——换一张你喜欢的背景图，调整毛玻璃的模糊程度，调大或调小行距，连圆角和阴影都能精确控制。每一个细节都可以按你的喜好来定。
>
> 想要颜值，也要效率。Windows 11 风格玻璃质感界面，搭配液态按钮和涟漪效果，好看的同时绝不拖慢你的电脑。

### 🌐 支持的模型服务商

| 类别       | 服务商            |
| -------- | -------------- |
| **国际主流** | OpenAI         |
| <br />   | Anthropic      |
| <br />   | Google Gemini  |
| <br />   | Mistral AI     |
| <br />   | Groq           |
| <br />   | OpenRouter     |
| <br />   | GitHub Copilot |
| <br />   | Azure OpenAI   |
| <br />   | OpenAI Codex   |
| **国内主流** | DeepSeek       |
| <br />   | 智谱AI           |
| <br />   | Z.ai           |
| <br />   | 月之暗面           |
| <br />   | 阿里通义           |
| <br />   | 硅基流动           |
| <br />   | 阶跃星辰           |
| <br />   | MiniMax        |
| <br />   | 火山引擎           |
| <br />   | BytePlus       |
| <br />   | AI Hub Mix     |
| **本地部署** | Ollama         |
| <br />   | OpenVINO       |
| <br />   | vLLM           |

***

## 🚀 功能特性

<div align="center">
  <table>
    <tr>
      <td align="center"><b>🤖 ReAct 推理引擎</b><br/>支持多轮思考-行动-观察循环与自动续跑策略</td>
      <td align="center"><b>🔧 10 类工具能力</b><br/>读写文件、编辑、目录、搜索、Shell、网络、系统、子代理、compact</td>
    </tr>
    <tr>
      <td align="center"><b>💬 流式对话界面</b><br/>流式输出、乐观 UI、最终消息与真实会话状态对齐</td>
      <td align="center"><b>🔐 权限与风控</b><br/>Safe / Moderate / Dangerous + Shell 风险评分与拦截</td>
    </tr>
    <tr>
      <td align="center"><b>🧠 双层上下文压缩</b><br/>MicroCompact + Full Compact + CompactState 持续追踪</td>
      <td align="center"><b>🎯 技能系统</b><br/>Markdown 技能定义、运行时热加载、按输入匹配</td>
    </tr>
  </table>
</div>

***

## 🏗️ 架构设计

```text
┌───────────────────────────────────────────────────────────────────────────┐
│ UI / QML                                                                 │
│ Main.qml -> AppShell -> WorkspacePanel / ChatBubble / ChatComposer       │
└───────────────────────────────────────────────────────────────────────────┘
                                   │
                                   ▼
┌───────────────────────────────────────────────────────────────────────────┐
│ QML Bridge                                                                │
│ QmlBackend + SessionListModel + MessageListModel                         │
│ - 负责 UI 状态桥接、乐观气泡、流式更新、外部平台接入                       │
└───────────────────────────────────────────────────────────────────────────┘
                                   │
                                   ▼
┌───────────────────────────────────────────────────────────────────────────┐
│ Application Layer                                                         │
│ AgentService + SessionManager + AgentOrchestrator + TemplateLoader       │
│ - 负责会话、模式切换、技能匹配、上下文同步、线程调度                      │
└───────────────────────────────────────────────────────────────────────────┘
                                   │
                                   ▼
┌───────────────────────────────────────────────────────────────────────────┐
│ Agent Core                                                                │
│ ReactAgentCore + ContextBuilder                                           │
│ - ReAct 迭代、工具调用、流式处理、最终回答、上下文压缩                    │
└───────────────────────────────────────────────────────────────────────────┘
            │                         │                         │
            ▼                         ▼                         ▼
┌───────────────────┐      ┌───────────────────┐      ┌───────────────────┐
│ Provider Layer    │      │ Tool Layer        │      │ Memory Layer      │
│ ProviderManager   │      │ ToolRegistry      │      │ MemoryManager     │
│ OpenAI/Anthropic/ │      │ ToolExecutor      │      │ ConversationHistory│
│ Gemini ...        │      │ tools/*           │      │                   │
└───────────────────┘      └───────────────────┘      └───────────────────┘
            │                         │                         │
            └──────────────┬──────────┴──────────┬──────────────┘
                           ▼                     ▼
┌───────────────────────────────────────────────────────────────────────────┐
│ Infrastructure                                                            │
│ PermissionManager / BashAnalyzer / HttpClient / SSEParser / SQLite       │
│ ConfigManager / Logger / ExternalPlatformManager                          │
└───────────────────────────────────────────────────────────────────────────┘
```

### 当前消息流

1. 用户输入先进入 `QmlBackend::sendMessage()`，界面先显示乐观 user 气泡和 assistant 占位。
2. `AgentService::sendMessage()` 根据输入决定 lightweight mode、multi-step mode、工具范围和系统提示词。
3. `ReactAgentCore::run()` 把真实 user message 写入当前会话上下文，并开始 ReAct 迭代。
4. Provider 以流式方式返回内容和 tool_call delta，QML 侧持续刷新 assistant 气泡。
5. 如需工具，`ToolExecutor` 统一执行，经过 `PermissionManager` 和 `BashAnalyzer`。
6. `ReactAgentCore::finalizeResponse()` 统一落地最终 assistant 消息。
7. `conversationUpdated -> SessionManager::updateMessages()` 将真实会话写入 SQLite，并同步回 UI。

### 当前压缩流

1. 每轮模型调用前先做 `MicroCompact`，把旧工具结果替换成占位摘要。
2. 最新一批工具结果永不折叠，避免模型看不到刚执行完的观察结果。
3. 若上下文仍超过阈值，则执行 `Full Compact`：
   - 存档完整消息到本地
   - 调模型生成结构化摘要
   - 用摘要替换旧历史，同时保留最后一批工具结果
4. `CompactState` 记录压缩次数、摘要、存档路径和 RecentFiles。

### 外部平台消息流

当前外部平台接入只保留 `Telegram` 和 `Feishu` 两条完整消息链路：

1. 外部平台收到用户消息后，由 `ExternalPlatformManager` 转成 `externalMessageReceived(platform, userId, content, messageId)`。
2. `QmlBackend` / `MainWindow` 为这个外部用户创建或切换到独立会话。
3. 消息进入正常的 `AgentService -> ReactAgentCore` 推理链路。
4. 最终回答完成后，再由 `ExternalPlatformManager` 回发到对应平台。

其中：

- `Telegram` 使用 `getUpdates` 轮询
- `Feishu` 使用本地 `QTcpServer` 监听回调，并支持签名校验与 reply 接口

***

## 🛠️ 技术栈

<p align="center">
  <img src="https://skillicons.dev/icons?i=qt,cmake,sqlite,github,git,vscode,linux,windows&perline=8" alt="tech stack"/>
</p>

<div align="center">
  <img src="https://img.shields.io/badge/Qt-41CD52?style=flat-square&logo=qt&logoColor=white" alt="Qt"/>
  <img src="https://img.shields.io/badge/C%2B%2B-00599C?style=flat-square&logo=cplusplus&logoColor=white" alt="C++"/>
  <img src="https://img.shields.io/badge/CMake-064F8C?style=flat-square&logo=cmake&logoColor=white" alt="CMake"/>
  <img src="https://img.shields.io/badge/SQLite-003B57?style=flat-square&logo=sqlite&logoColor=white" alt="SQLite"/>
  <img src="https://img.shields.io/badge/Git-F05032?style=flat-square&logo=git&logoColor=white" alt="Git"/>
  <img src="https://img.shields.io/badge/Visual_Studio_Code-0078D4?style=flat-square&logo=visual-studio-code&logoColor=white" alt="VSCode"/>
</div>

***

## 📸 项目截图


![主界面](https://raw.githubusercontent.com/DavidLi-TJ/ClawPP-Agent/master/images/pic.png)

<p align="center"><i>Claw++ 主界面</i></p>

![Token 配置](https://raw.githubusercontent.com/DavidLi-TJ/ClawPP-Agent/master/images/pic2.png)

<p align="center"><i>Token 配置界面</i></p>

***

## 📦 安装

### 📥 下载安装包（推荐）

前往 [GitHub Releases](https://github.com/DavidLi-TJ/ClawPP-Agent/releases) 下载最新版本的 Windows 安装包（.exe），双击即可安装，无需任何配置。

> 💡 安装包体积 < 100MB，一键安装，开箱即用
>
> 📦 安装包文件：`ClawPP-Installer-v1.0.0.exe`（57.7 MB）

### 🔄 使用 IFW 自行打包

> 需要预先安装 [Qt Installer Framework](https://doc.qt.io/qtinstallerframework/)

```bash
# 在项目根目录运行
build_installer.bat
```

脚本会自动完成：Release 构建 → 文件收集 → IFW 打包，最终生成 `ClawPP-Installer-v1.0.0.exe`。

### 🔨 从源码构建

#### 📋 环境要求

| 依赖      | 版本要求                           |
| ------- | ------------------------------ |
| Qt      | 6.5+                           |
| CMake   | 3.20+                          |
| C++ 编译器 | MSVC 2019+ / GCC 8+ / Clang 9+ |

#### 构建步骤

```bash
# 克隆仓库
git clone https://github.com/DavidLi-TJ/ClawPP-Agent.git
cd ClawPP-Agent

# 创建构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 构建项目
cmake --build . --config Release
```

#### 🏗️ 构建安装包

> 需要预先安装 [Qt Installer Framework](https://doc.qt.io/qtinstallerframework/)

```bash
# 在项目根目录运行
build_installer.bat
```

脚本会自动完成：Release 构建 → 文件收集 → IFW 打包，最终生成 `ClawPP-Installer-v1.0.0.exe`。

### ▶️ 运行

```bash
# Windows
.\bin\ClawPP.exe

# Linux/Mac
./bin/ClawPP
```

***

## 📁 项目结构

```
cpqclaw/
├── src/
│   ├── agent/                    # ReactAgentCore、ContextBuilder、Agent Profile
│   ├── application/              # AgentService、SessionManager、Orchestrator、线程封装
│   ├── common/                   # Message / Session / ToolCall / CompactState 等核心类型
│   ├── infrastructure/
│   │   ├── config/               # 配置管理
│   │   ├── database/             # SQLite 会话与消息存储
│   │   ├── event/                # 事件总线
│   │   ├── logging/              # 日志系统
│   │   ├── network/              # HTTP、SSE、Telegram、Feishu
│   │   └── runner/               # Shell 运行器
│   ├── memory/                   # 记忆系统与对话历史
│   ├── permission/               # 权限判断与 shell 风险分析
│   ├── provider/                 # OpenAI / Anthropic / Gemini / ProviderManager
│   ├── qml/                      # QmlBackend 与 QML ListModel 桥接
│   ├── skill/                    # 技能定义与热加载
│   ├── tool/
│   │   ├── tool_registry.*       # 工具注册
│   │   ├── tool_executor.*       # 工具执行与权限接入
│   │   └── tools/                # read/write/edit/search/shell/network/compact 等工具
│   └── ui/                       # Widgets 版本界面（保留）
├── qml/                          # 主界面 QML 组件
├── agents/                       # agent 配置和记忆模板
├── templates/                    # SOUL / TOOLS / AGENTS 等提示词模板
├── memory/                       # 运行时 MEMORY.md / HISTORY.md
├── workspace/                    # 默认工作区、compact 存档等运行数据
├── config/                       # 配置文件
├── resources/                    # 资源
└── build/                        # 构建输出
```

***

## 🔧 开发指南

### 添加新工具

```cpp
class MyTool : public ITool {
    QString name() const override { return "my_tool"; }
    QString description() const override { return "我的工具"; }
    ToolResult execute(const QJsonObject& args) override {
        // 实现逻辑
        return ToolResult::success("结果");
    }
};

// 注册工具（通常在 QmlBackend::registerTools 中）
ToolRegistry::instance().registerTool(new MyTool());
```

### 添加新 Provider

```cpp
class MyProvider : public ILLMProvider {
    QString name() const override { return "my-provider"; }
    void chatStream(...) override { /* 实现流式调用 */ }
};

// 在 ProviderManager 中接入并在配置层暴露
```

### 调试技巧

建议优先观察以下几条链路：

- `QmlBackend::sendMessage()`：看 UI 到业务入口
- `AgentService::sendMessage()`：看模式切换、技能匹配、工具推断
- `ReactAgentCore::processIteration()`：看 ReAct 主循环
- `ToolExecutor::execute()`：看工具执行和权限拦截
- `ReactAgentCore::performFullCompact()`：看全量压缩流程

使用日志系统输出调试信息：

```cpp
LOG_INFO("消息内容");
LOG_DEBUG("调试信息");
LOG_ERROR("错误信息");
```

当前项目还附带了更详细的本地文档：

- `ARCHITECTURE_QUICKSTART_CN.md`
- `ARCHITECTURE_OVERVIEW_2026_CN.md`
- `VIDEO_SCRIPT_15MIN_CN.md`

***

## 🎓 关于项目

本项目是 **南开大学（NKU）** 的课程大作业，由 [DavidLi-TJ](https://github.com/DavidLi-TJ) 开发。

项目使用 **Qt/C++** 框架实现，旨在探索 AI Agent 在桌面应用中的实际应用场景。

***

## 📊 项目统计

<div align="center">
  <a href="https://github.com/DavidLi-TJ/ClawPP-Agent/stargazers">
    <img src="https://img.shields.io/github/stars/DavidLi-TJ/ClawPP-Agent?style=for-the-badge&logo=starship&color=orange&logoColor=white&labelColor=1A1B26" alt="stars"/>
  </a>
  <br/><br/>
  <a href="https://star-history.com/#DavidLi-TJ/ClawPP-Agent&Date">
    <img src="https://api.star-history.com/svg?repos=DavidLi-TJ/ClawPP-Agent&type=Date" alt="Star History Chart"/>
  </a>
</div>

***

## 📜 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件

***

## 💡 致谢

- 南开大学课程项目支持
- ReAct 论文作者的开创性工作
- Qt 框架提供的优秀开发基础
- 所有开源项目的贡献者

***

<p align="center">
  <b>⬆️ 觉得有帮助？给个 Star 支持一下吧！⬆️</b>
</p>

<p align="center">
  <a href="#readme-top">
    <img src="https://img.shields.io/badge/Back_to_Top-000000?style=for-the-badge&logo=github&logoColor=white" alt="Back to Top"/>
  </a>
</p>

<p align="center">
  Made with ❤️ by <a href="https://github.com/DavidLi-TJ">DavidLi-TJ</a> | NKU Course Project
</p>
