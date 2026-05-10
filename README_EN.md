<p align="center" id="readme-top">
  <a href="README.md"><img src="https://img.shields.io/badge/🇨_中文-红色?style=for-the-badge&color=red" alt="中文"/></a>
  <a href="README_EN.md"><img src="https://img.shields.io/badge/🇬_English-蓝色?style=for-the-badge&color=blue" alt="English"/></a>
</p>

<p align="center">
  <img src="https://raw.githubusercontent.com/DavidLi-TJ/ClawPP-Agent/master/icon.png" alt="Claw++ Icon" width="160" height="160"/>
</p>

<h1 align="center">Claw++ — AI Agent Desktop Application</h1>

<p align="center">
  <img src="https://img.shields.io/github/v/tag/DavidLi-TJ/ClawPP-Agent?style=for-the-badge&logo=semantic-release&color=green&label=version&labelColor=1A1B26" alt="version"/>
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
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Qt-6.5+-green?style=for-the-badge&logo=qt&logoColor=white" alt="Qt 6.5+"/>
  <img src="https://img.shields.io/badge/C%2B%2B-17-blue?style=for-the-badge&logo=cplusplus&logoColor=white" alt="C++ 17"/>
  <img src="https://img.shields.io/badge/CMake-3.20+-orange?style=for-the-badge&logo=cmake&logoColor=white" alt="CMake 3.20+"/>
  <img src="https://img.shields.io/badge/SQLite-3.x-purple?style=for-the-badge&logo=sqlite&logoColor=white" alt="SQLite"/>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Platform-Windows%2011-blue?style=for-the-badge&logo=windows&logoColor=white" alt="Windows 11"/>
</p>

<p align="center">
  <img src="https://raw.githubusercontent.com/DavidLi-TJ/ClawPP-Agent/master/pic.png" alt="Claw++ Main UI Preview" width="100%"/>
</p>

---

## 📖 Table of Contents

<br/>

- [✨ Introduction](#-introduction)
- [🌐 Supported Providers](#-supported-providers)
- [🚀 Features](#-features)
- [🏗️ Architecture](#️-architecture)
- [🛠️ Tech Stack](#️-tech-stack)
- [📸 Screenshots](#-screenshots)
- [📦 Quick Start](#-quick-start)
- [📁 Project Structure](#-project-structure)
- [🔧 Development Guide](#-development-guide)
- [🎓 About](#-about)
- [📊 Stats](#-stats)
- [📜 License](#-license)
- [💡 Acknowledgments](#-acknowledgments)

<br/>

---

<br/>

## ✨ Introduction

<br/>

> **Claw++** is a modern AI Agent desktop application built with Qt/C++ for the Windows platform. It implements a complete **ReAct (Reasoning + Acting)** loop, supports **23 LLM service providers** (covering OpenAI, Anthropic, Gemini, DeepSeek, Zhipu and other mainstream platforms), and provides rich tool calling capabilities with intelligent memory management. Built with Qt Quick 6, it features a stunning **Windows 11 glass morphism UI** with real-time streaming chat, session management, context compression, and a skill plugin system.

<br/>

- 🤖 **ReAct Engine**: Think-Act-Observe loop with multi-turn autonomous reasoning
- 🔧 **9 Built-in Tools**: file read/write, shell commands, network requests, subagents, search, etc.
- 💬 **Streaming Chat**: SSE-based real-time responses with Markdown rendering and code highlighting
- 🔐 **3-Level Permission System**: Safe / Moderate / Dangerous with shell risk scoring (1-4)
- 🧠 **4-Stage Memory Compression**: Trim → Dedupe → Fold → Summarize progressive pipeline
- 🎯 **Skill Plugin System**: Markdown skill definitions with YAML metadata, hot-reload at runtime
- 🪟 **Win11 Glass Morphism UI**: Frosted panels, liquid buttons, ripple effects, smooth animations
- 🌐 **23 LLM Providers**: Comprehensive coverage of domestic and international platforms
- 💾 **SQLite Persistence**: Full session, message, and memory storage with import/export

<br/>

### 🌐 Supported Model Providers

<br/>

<div align="center">

| Category | Provider | Category | Provider |
|:---:|:---|:---:|:---|
| **Global** | OpenAI | **China** | DeepSeek |
| | Anthropic | | Zhipu AI |
| | Google Gemini | | Z.ai |
| | Mistral AI | | Moonshot AI |
| | Groq | | DashScope |
| | OpenRouter | | SiliconFlow |
| | GitHub Copilot | | StepFun |
| | Azure OpenAI | | MiniMax |
| | OpenAI Codex | | Volcengine |
| **Local** | Ollama | | BytePlus |
| | OpenVINO | | AI Hub Mix |
| | vLLM | | |

</div>

<br/>

---

<br/>

## 🚀 Features

<br/>

<div align="center">
  <table>
    <tr>
      <td align="center"><b>🤖 ReAct Engine</b><br/><br/>Think-Act-Observe loop with multi-turn reasoning</td>
      <td align="center"><b>🔧 Tool Calling</b><br/><br/>File system, Shell, network requests and more</td>
    </tr>
    <tr>
      <td align="center"><b>💬 Streaming Chat</b><br/><br/>Real-time responses, smooth chat experience</td>
      <td align="center"><b>🔐 Permission Mgmt</b><br/><br/>3-level permissions (Safe/Moderate/Dangerous)</td>
    </tr>
    <tr>
      <td align="center"><b>🧠 Smart Memory</b><br/><br/>Auto-compression of conversation history</td>
      <td align="center"><b>🎯 Skill System</b><br/><br/>Extensible skill plugin mechanism</td>
    </tr>
  </table>
</div>

<br/>

---

<br/>

## 🏗️ Architecture

<br/>

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
│  ┌──────────┐ ┌────────── ┌──────────┐ ──────────┐  │
│  │ Provider │ │  Memory  │ │   Tool   │ │Permission│  │
│  └────────── └──────────┘ ──────────┘ └──────────┘  │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐  │
│  │Database  │ │  Config  │ │  Logger  │ │  Event   │  │
│  └──────────┘ └────────── └──────────┘ ──────────┘  │
└─────────────────────────────────────────────────────────┘
```

<br/>

---

<br/>

## 🛠️ Tech Stack

<br/>

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

<br/>

---

<br/>

## 📸 Screenshots

<br/>

> Images can be previewed directly from the repository. Open in a new tab for full resolution.

<br/>

<p align="center">
  <b>Main Interface</b>
</p>

<p align="center">
  <img src="https://raw.githubusercontent.com/DavidLi-TJ/ClawPP-Agent/master/pic.png" alt="Main UI" width="90%"/>
</p>

<br/>
<br/>

<p align="center">
  <b>Token Configuration</b>
</p>

<p align="center">
  <img src="https://raw.githubusercontent.com/DavidLi-TJ/ClawPP-Agent/master/pic2.png" alt="Token Configuration" width="90%"/>
</p>

<br/>

---

<br/>

## 📦 Quick Start

<br/>

### Requirements

<br/>

| Dependency | Version |
|------------|---------|
| Qt | 6.5+ |
| CMake | 3.20+ |
| C++ Compiler | MSVC 2019+ / GCC 8+ / Clang 9+ |

<br/>

### Build

<br/>

```bash
git clone https://github.com/DavidLi-TJ/ClawPP-Agent.git
cd ClawPP-Agent
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

<br/>

### Run

<br/>

```bash
# Windows
.\bin\ClawPP.exe

# Linux/Mac
./bin/ClawPP
```

<br/>

---

<br/>

## 📁 Project Structure

<br/>

```
cpqclaw/
├── src/                    # C++ source code
│   ├── agent/              # Agent core (ReAct, context builder)
│   ├── application/        # Application layer (service, session)
│   ├── common/             # Types, constants
│   ├── infrastructure/     # Config, database, network, logging
│   ├── memory/             # Memory system
│   ├── permission/         # Permission management
│   ├── provider/           # LLM providers
│   ├── skill/              # Skill system
│   ├── tool/               # Tool system
│   └── ui/                 # Qt Widgets-based UI
├── qml/                    # QML UI files
├── resources/              # Resource files
├── CMakeLists.txt          # Build configuration
└── README.md               # Documentation (Chinese)
```

<br/>

---

<br/>

## 🔧 Development Guide

<br/>

### Adding a New Tool

<br/>

```cpp
class MyTool : public ITool {
    QString name() const override { return "my_tool"; }
    QString description() const override { return "My Tool"; }
    ToolResult execute(const QJsonObject& args) override {
        // Implementation
        return ToolResult::success("Result");
    }
};

// Register tool
ToolRegistry::instance().registerTool(new MyTool());
```

<br/>

### Adding a New Provider

<br/>

```cpp
class MyProvider : public ILLMProvider {
    QString name() const override { return "my-provider"; }
    void chatStream(...) override { /* Streaming call implementation */ }
};

// Register Provider
ProviderManager::instance().registerProvider("my", new MyProvider());
```

<br/>

### Debugging

<br/>

Use the logging system for debug output:

```cpp
LOG_INFO("Message content");
LOG_DEBUG("Debug info");
LOG_ERROR("Error info");
```

Log file location: `~/.clawpp/logs/app.log`

<br/>

---

<br/>

## 🎓 About

<br/>

This project is a **Nankai University (NKU)** course assignment, developed by [DavidLi-TJ](https://github.com/DavidLi-TJ).

Built with **Qt/C++** framework to explore practical applications of AI Agents in desktop software.

<br/>

---

<br/>

## 📊 Stats

<br/>

<div align="center">
  <a href="https://github.com/DavidLi-TJ/ClawPP-Agent/stargazers">
    <img src="https://img.shields.io/github/stars/DavidLi-TJ/ClawPP-Agent?style=for-the-badge&logo=starship&color=orange&logoColor=white&labelColor=1A1B26" alt="stars"/>
  </a>
  <br/><br/>
  <a href="https://star-history.com/#DavidLi-TJ/ClawPP-Agent&Date">
    <img src="https://api.star-history.com/svg?repos=DavidLi-TJ/ClawPP-Agent&type=Date" alt="Star History Chart"/>
  </a>
</div>

<br/>

---

<br/>

## 📜 License

<br/>

MIT License - see [LICENSE](LICENSE) file for details.

<br/>

---

<br/>

## 💡 Acknowledgments

<br/>

- Nankai University course project support
- Pioneering work by ReAct paper authors
- Excellent development foundation provided by Qt framework
- All open source project contributors

<br/>

---

<br/>

<p align="center">
  <b>⬆️ Find this helpful? Give it a Star! ⬆️</b>
</p>

<p align="center">
  <a href="#readme-top">
    <img src="https://img.shields.io/badge/Back_to_Top-000000?style=for-the-badge&logo=github&logoColor=white" alt="Back to Top"/>
  </a>
</p>

<p align="center">
  Made with ❤️ by <a href="https://github.com/DavidLi-TJ">DavidLi-TJ</a> | NKU Course Project
</p>
