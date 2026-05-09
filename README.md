<p align="center">
  <img src="https://capsule-render.vercel.app/api?type=waving&color=auto&height=300&section=header&text=Claw%2B%2B&fontSize=90&animation=fadeIn&fontAlignY=38&desc=AI%20Agent%20Desktop%20App&descAlignY=51&descAlign=62" alt="header"/>
</p>

<p align="center">
  <a href="https://github.com/DavidLi-TJ/clawpp/stargazers">
    <img src="https://img.shields.io/github/stars/DavidLi-TJ/clawpp?style=for-the-badge&logo=starship&color=orange&logoColor=white&labelColor=1A1B26" alt="stars"/>
  </a>
  <a href="https://github.com/DavidLi-TJ/clawpp/network/members">
    <img src="https://img.shields.io/github/forks/DavidLi-TJ/clawpp?style=for-the-badge&logo=git&color=blue&logoColor=white&labelColor=1A1B26" alt="forks"/>
  </a>
  <a href="https://github.com/DavidLi-TJ/clawpp/issues">
    <img src="https://img.shields.io/github/issues/DavidLi-TJ/clawpp?style=for-the-badge&logo=github&color=red&logoColor=white&labelColor=1A1B26" alt="issues"/>
  </a>
  <a href="https://github.com/DavidLi-TJ/clawpp/blob/main/LICENSE">
    <img src="https://img.shields.io/github/license/DavidLi-TJ/clawpp?style=for-the-badge&color=green&logoColor=white&labelColor=1A1B26" alt="license"/>
  </a>
  <a href="https://img.shields.io/github/last-commit/DavidLi-TJ/clawpp">
    <img src="https://img.shields.io/github/last-commit/DavidLi-TJ/clawpp?style=for-the-badge&color=purple&logo=git&logoColor=white&labelColor=1A1B26" alt="last commit"/>
  </a>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Qt-6.5+-green?style=for-the-badge&logo=qt&logoColor=white" alt="Qt 6.5+"/>
  <img src="https://img.shields.io/badge/C%2B%2B-17-blue?style=for-the-badge&logo=cplusplus&logoColor=white" alt="C++ 17"/>
  <img src="https://img.shields.io/badge/CMake-3.20+-orange?style=for-the-badge&logo=cmake&logoColor=white" alt="CMake 3.20+"/>
  <img src="https://img.shields.io/badge/SQLite-3.x-purple?style=for-the-badge&logo=sqlite&logoColor=white" alt="SQLite"/>
</p>

<p align="center">
  <img src="https://img.shields.io/github/repo-size/DavidLi-TJ/clawpp?style=for-the-badge&color=yellow&logo=hackthebox&logoColor=white&labelColor=1A1B26" alt="repo size"/>
  <img src="https://img.shields.io/github/contributors/DavidLi-TJ/clawpp?style=for-the-badge&color=blue&logo=github&logoColor=white&labelColor=1A1B26" alt="contributors"/>
</p>

---

## 📖 目录

- [✨ 项目简介](#-项目简介)
- [🚀 功能特性](#-功能特性)
- [🏗️ 架构设计](#️-架构设计)
- [🛠️ 技术栈](#️-技术栈)
- [📸 项目截图](#-项目截图)
- [📦 快速开始](#-快速开始)
- [📁 项目结构](#-项目结构)
- [🔧 开发指南](#-开发指南)
- [🎓 关于项目](#-关于项目)
- [📊 项目统计](#-项目统计)

---

## ✨ 项目简介

> **Claw++** 是一个基于 Qt/C++ 的 AI Agent 桌面应用程序，支持 ReAct 模式推理、工具调用、会话管理等核心功能。

- 🤖 **智能推理**：采用 ReAct（Reasoning + Acting）架构，实现思考-行动-观察循环
- 🔧 **工具扩展**：支持文件系统、Shell 命令、网络请求等多种工具
- 💬 **实时交互**：基于 SSE 的流式对话，响应迅速
- 🔐 **安全可控**：三级权限管理系统，保障操作安全
- 🧠 **记忆优化**：自动压缩对话历史，支持长期记忆
- 📱 **现代界面**：Qt Quick 6 打造的高颜值 UI

---

## 🚀 功能特性

<div align="center">
  <table>
    <tr>
      <td align="center"><b>🤖 ReAct 推理引擎</b><br/>支持思考-行动-观察循环的自主推理</td>
      <td align="center"><b>🔧 工具调用系统</b><br/>文件系统、Shell、网络请求等多类工具</td>
    </tr>
    <tr>
      <td align="center"><b>💬 流式对话界面</b><br/>实时响应，流畅的聊天体验</td>
      <td align="center"><b>🔐 权限管理</b><br/>三级权限（安全/中等/危险），安全执行</td>
    </tr>
    <tr>
      <td align="center"><b>🧠 智能记忆</b><br/>对话历史自动压缩，支持长期记忆</td>
      <td align="center"><b>🎯 技能系统</b><br/>可扩展的技能插件机制</td>
    </tr>
  </table>
</div>

---

## 🏗️ 架构设计

```
┌─────────────────────────────────────────────────────────┐
│                      UI Layer                           │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐    │
│  │ MainWindow  │  │  ChatView   │  │SessionPanel │    │
│  └─────────────┘  └─────────────┘  └─────────────┘    │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│                  Application Layer                      │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐    │
│  │AgentService │  │SessionManager│ │TemplateLoader│    │
│  └─────────────┘  └─────────────┘  └─────────────┘    │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│                    Agent Core Layer                     │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐    │
│  │IAgentCore   │  │ReactAgentCore│ │ContextBuilder│   │
│  └─────────────┘  └─────────────┘  └─────────────┘    │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│                   Infrastructure Layer                  │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐  │
│  │ Provider │ │  Memory  │ │   Tool   │ │Permission│  │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘  │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐  │
│  │Database  │ │  Config  │ │  Logger  │ │  Event   │  │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘  │
└─────────────────────────────────────────────────────────┘
```

---

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

---

## 📸 项目截图

> 💡 在此处添加项目截图，展示 Claw++ 的实际运行效果。

<div align="center">
  <img src="screenshots/main-ui.png" alt="主界面" width="800"/>
  <p><i>Claw++ 主界面</i></p>
  
  <img src="screenshots/chat-view.png" alt="聊天界面" width="800"/>
  <p><i>实时流式对话界面</i></p>
</div>

---

## 📦 快速开始

### 📋 环境要求

| 依赖 | 版本要求 |
|------|----------|
| Qt | 6.5+ |
| CMake | 3.20+ |
| C++ 编译器 | MSVC 2019+ / GCC 8+ / Clang 9+ |

### 🔨 构建步骤

```bash
# 克隆仓库
git clone https://github.com/DavidLi-TJ/clawpp.git
cd clawpp

# 创建构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 构建项目
cmake --build . --config Release
```

### ▶️ 运行

```bash
# Windows
.\bin\ClawPP.exe

# Linux/Mac
./bin/ClawPP
```

---

## 📁 项目结构

```
cpqclaw/
├── src/                          # 源代码目录
│   ├── agent/                    # Agent 核心层
│   │   ├── i_agent_core.h        # Agent 接口定义
│   │   ├── react_agent_core.h    # ReAct 模式实现
│   │   └── context_builder.h     # 上下文构建器
│   ├── application/              # 应用层
│   │   ├── agent_service.h       # Agent 服务
│   │   ├── session_manager.h     # 会话管理
│   │   └── session_thread.h      # 会话线程
│   ├── common/                   # 公共类型和工具
│   │   ├── types.h               # 核心数据结构
│   │   ├── constants.h           # 常量定义
│   │   └── result.h              # Result 模板类
│   ├── infrastructure/           # 基础设施层
│   │   ├── config/               # 配置管理
│   │   ├── database/             # 数据库（SQLite）
│   │   ├── event/                # 事件总线
│   │   ├── logging/              # 日志系统
│   │   └── network/              # 网络（HTTP、SSE）
│   ├── memory/                   # 内存系统
│   ├── permission/               # 权限管理
│   ├── provider/                 # LLM Provider
│   ├── skill/                    # 技能系统
│   ├── tool/                     # 工具系统
│   └── ui/                       # 用户界面
├── config/                       # 配置文件
├── qml/                          # QML UI 文件
├── resources/                    # 资源文件
├── CMakeLists.txt                # CMake 构建配置
└── README.md                     # 项目说明文档
```

---

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

// 注册工具
ToolRegistry::instance().registerTool(new MyTool());
```

### 添加新 Provider

```cpp
class MyProvider : public ILLMProvider {
    QString name() const override { return "my-provider"; }
    void chatStream(...) override { /* 实现流式调用 */ }
};

// 注册 Provider
ProviderManager::instance().registerProvider("my", new MyProvider());
```

### 调试技巧

使用日志系统输出调试信息：
```cpp
LOG_INFO("消息内容");
LOG_DEBUG("调试信息");
LOG_ERROR("错误信息");
```

日志文件位置: `~/.clawpp/logs/app.log`

---

## 🎓 关于项目

本项目是 **南开大学（NKU）** 的课程大作业，由 [DavidLi-TJ](https://github.com/DavidLi-TJ) 开发。

项目使用 **Qt/C++** 框架实现，旨在探索 AI Agent 在桌面应用中的实际应用场景。

---

## 📊 项目统计

<div align="center">
  <h3>📈 Stars 趋势</h3>
  <a href="https://starchart.cc/DavidLi-TJ/clawpp">
    <img src="https://starchart.cc/DavidLi-TJ/clawpp.svg?variant=adaptive" alt="starchart"/>
  </a>
  
  <h3>🏆 GitHub Trophy</h3>
  <a href="https://github.com/ryo-ma/github-profile-trophy">
    <img src="https://github-profile-trophy.vercel.app/?username=DavidLi-TJ&theme=darkhub&row=1&column=7&margin-w=15" alt="trophies"/>
  </a>
  
  <h3>👥 Contributors</h3>
  <a href="https://github.com/DavidLi-TJ/clawpp/graphs/contributors">
    <img src="https://contrib.rocks/image?repo=DavidLi-TJ/clawpp&max=100" alt="contributors"/>
  </a>
</div>

---

## 📜 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件

---

## 💡 致谢

- 南开大学课程项目支持
- ReAct 论文作者的开创性工作
- Qt 框架提供的优秀开发基础
- 所有开源项目的贡献者

---

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
