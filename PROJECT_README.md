# Claw++ 项目阅读指南

> 本指南帮助你快速理解 Claw++ 项目的架构和代码结构，方便你进行二次开发。

## 🚀 快速入口（建议先看）

- [ARCHITECTURE_QUICKSTART_CN.md](ARCHITECTURE_QUICKSTART_CN.md) - 30 分钟二次开发速查（调用链、改动入口、风险区）
- [PROJECT_READING_PATH_CN.md](PROJECT_READING_PATH_CN.md) - 新手详细阅读路径（按阶段执行）
- [FUNCTION_LEVEL_READING_TASKS_CN.md](FUNCTION_LEVEL_READING_TASKS_CN.md) - 函数级阅读任务单（逐函数理解）
- [POST_CLASS_EXECUTION_PLAN_CN.md](POST_CLASS_EXECUTION_PLAN_CN.md) - 复课后可直接执行的改造计划
- [TOOLS_LAYER_GUIDE_CN.md](TOOLS_LAYER_GUIDE_CN.md) - 工具层专项开发指南
- [TOOLS_BOUNDARY_TEST_CHECKLIST_CN.md](TOOLS_BOUNDARY_TEST_CHECKLIST_CN.md) - 工具层边界测试清单
- [PROVIDER_MEMORY_GUIDE_CN.md](PROVIDER_MEMORY_GUIDE_CN.md) - Provider 与 Memory 改造指南
- [UI_PROVIDER_MEMORY_REFACTOR_NOTES_CN.md](UI_PROVIDER_MEMORY_REFACTOR_NOTES_CN.md) - UI/Provider/Memory 深化改造说明
- [TESTING_QUICKSTART_CN.md](TESTING_QUICKSTART_CN.md) - 测试快速上手

## 📚 目录

- [项目概述](#项目概述)
- [架构图](#架构图)
- [目录结构](#目录结构)
- [核心模块详解](#核心模块详解)
- [代码阅读顺序](#代码阅读顺序)
- [修改建议](#修改建议)
- [常见问题](#常见问题)

---

## 项目概述

**Claw++** 是一个基于 Qt/C++ 的 AI Agent 桌面应用程序，支持：
- 🤖 ReAct 模式 Agent（思考 - 行动 - 观察循环）
- 🔧 工具调用系统（文件系统、Shell、网络等）
- 💬 流式对话界面
- 🔐 权限管理系统
- 🧠 对话内存管理（自动压缩）
- 🎯 技能系统（可扩展）

**技术栈**：
- Qt 6.x (C++17)
- SQLite 数据库
- HTTP 客户端 (QNetworkAccessManager)
- SSE 流式解析

---

## 架构图

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

## 目录结构

```
cpqclaw/
├── src/                          # 源代码目录
│   ├── agent/                    # Agent 核心层
│   │   ├── i_agent_core.h        # Agent 接口定义
│   │   ├── react_agent_core.h    # ReAct 模式实现
│   │   └── context_builder.h     # 上下文构建器
│   │
│   ├── application/              # 应用层
│   │   ├── agent_service.h       # Agent 服务（核心业务逻辑）
│   │   ├── session_manager.h     # 会话管理
│   │   ├── session_thread.h      # 会话线程
│   │   └── template_loader.h     # 提示词模板加载
│   │
│   ├── common/                   # 公共类型和工具
│   │   ├── types.h               # 核心数据结构（重要！）
│   │   ├── constants.h           # 常量定义
│   │   └── result.h              # Result 模板类
│   │
│   ├── infrastructure/           # 基础设施层
│   │   ├── config/               # 配置管理
│   │   ├── database/             # 数据库（SQLite）
│   │   ├── event/                # 事件总线
│   │   ├── logging/              # 日志系统
│   │   └── network/              # 网络（HTTP、SSE）
│   │
│   ├── memory/                   # 内存系统
│   │   ├── i_memory_system.h     # 内存接口
│   │   ├── conversation_history.h# 对话历史
│   │   └── memory_manager.h      # 内存管理器
│   │
│   ├── permission/               # 权限管理
│   │   └── permission_manager.h  # 权限检查和规则
│   │
│   ├── provider/                 # LLM Provider
│   │   ├── i_llm_provider.h      # Provider 接口
│   │   ├── openai_provider.h     # OpenAI 实现
│   │   └── provider_manager.h    # Provider 管理器
│   │
│   ├── skill/                    # 技能系统
│   │   ├── skill.h               # 技能定义
│   │   └── skill_manager.h       # 技能管理
│   │
│   ├── tool/                     # 工具系统
│   │   ├── i_tool.h              # 工具接口
│   │   ├── tool_registry.h       # 工具注册表
│   │   ├── tool_executor.h       # 工具执行器
│   │   └── tools/                # 具体工具实现
│   │       ├── filesystem_tool.h # 文件系统工具
│   │       ├── shell_tool.h      # Shell 工具
│   │       ├── network_tool.h    # 网络工具
│   │       └── system_tool.h     # 系统工具
│   │
│   └── ui/                       # 用户界面
│       ├── main_window.h         # 主窗口
│       ├── chat_view.h           # 聊天视图（核心 UI）
│       ├── session_panel.h       # 会话面板
│       └── widgets/              # UI 组件
│           └── message_bubble.h  # 消息气泡
│
├── config/                       # 配置文件
│   └── default_config.json       # 默认配置
│
├── resources/                    # 资源文件
│   └── styles/                   # QSS 样式
│
└── CMakeLists.txt                # CMake 构建配置
```

---

## 核心模块详解

### 1. Agent 核心层 🤖

**关键文件**：
- [`src/agent/i_agent_core.h`](src/agent/i_agent_core.h) - Agent 接口定义
- [`src/agent/react_agent_core.h`](src/agent/react_agent_core.h) - ReAct 模式实现

**核心流程**：
```cpp
// ReAct 循环
1. 接收用户输入 → run(input)
2. 思考下一步 → processThought()
3. 是否有行动？→ hasAction()
4. 执行工具 → executeTool()
5. 观察结果 → 添加到消息
6. 重复直到完成
```

**修改建议**：
- 如果想修改 Agent 行为，重点看 `react_agent_core.cpp`
- 可以创建新的 Agent 类继承 `IAgentCore`

---

### 2. 应用层 📱

**关键文件**：
- [`src/application/agent_service.h`](src/application/agent_service.h) - **核心业务逻辑**
- [`src/application/session_manager.h`](src/application/session_manager.h) - 会话管理

**AgentService 职责**：
- 管理会话生命周期
- 处理用户消息
- 调用 LLM Provider
- 协调工具执行
- 发送信号到 UI

**修改建议**：
- 添加新功能（如文件上传）在 `agent_service.h` 添加方法
- 修改会话逻辑看 `session_manager.cpp`

---

### 3. UI 层 🎨

**关键文件**：
- [`src/ui/main_window.h`](src/ui/main_window.h) - 主窗口
- [`src/ui/chat_view.h`](src/ui/chat_view.h) - **聊天界面（最重要）**

**ChatView 功能**：
- 显示消息气泡
- 处理用户输入
- 调用 AgentService
- 处理流式响应
- 执行工具调用

**修改建议**：
- 修改 UI 样式：看 `chat_view.cpp` 的 `setupUi()`
- 添加新 UI 功能：在 `chat_view.h` 添加槽函数

---

### 4. Provider 层 🔌

**关键文件**：
- [`src/provider/i_llm_provider.h`](src/provider/i_llm_provider.h) - Provider 接口
- [`src/provider/openai_provider.h`](src/provider/openai_provider.h) - OpenAI 实现

**添加新 Provider**：
```cpp
// 1. 继承 ILLMProvider
class MyProvider : public ILLMProvider {
    QString name() const override { return "my-provider"; }
    void chatStream(...) override { /* 实现 */ }
};

// 2. 注册到 ProviderManager
ProviderManager::instance().registerProvider("my", new MyProvider());
```

---

### 5. 工具系统 🛠️

**关键文件**：
- [`src/tool/i_tool.h`](src/tool/i_tool.h) - 工具接口
- [`src/tool/tool_registry.h`](src/tool/tool_registry.h) - 工具注册表
- [`src/tool/tools/`](src/tool/tools/) - 具体工具实现

**添加工具示例**：
```cpp
// 1. 创建新工具
class MyTool : public ITool {
    QString name() const override { return "my_tool"; }
    ToolResult execute(const QJsonObject& args) override {
        // 实现逻辑
        return result;
    }
};

// 2. 注册工具
ToolRegistry::instance().registerTool(new MyTool());
```

**现有工具**：
- `filesystem_tool` - 文件读写、目录操作
- `shell_tool` - Shell 命令执行
- `network_tool` - HTTP 请求
- `system_tool` - 系统信息

---

### 6. 内存系统 🧠

**关键文件**：
- [`src/memory/memory_manager.h`](src/memory/memory_manager.h) - 内存管理器
- [`src/memory/conversation_history.h`](src/memory/conversation_history.h) - 对话历史

**功能**：
- 管理对话消息
- Token 计数
- 自动压缩（保留开头和结尾，压缩中间）

**修改建议**：
- 调整压缩参数：修改 `MemoryConfig` 结构体
- 修改 token 估算算法：`conversation_history.cpp::estimateTokens()`

---

### 7. 权限系统 🔐

**关键文件**：
- [`src/permission/permission_manager.h`](src/permission/permission_manager.h)

**权限级别**：
- `Safe` - 安全操作，直接执行
- `Moderate` - 中等风险，需要确认
- `Dangerous` - 危险操作，严格限制

**修改建议**：
- 添加工具权限：在工具的 `permissionLevel()` 返回对应级别
- 添加权限规则：修改 `permission_manager.cpp`

---

## 代码阅读顺序

### 🎯 快速上手路径

**第一天**：理解整体架构
1. `src/common/types.h` - 了解所有数据结构
2. `src/ui/main_window.cpp` - 看 UI 如何搭建
3. `src/ui/chat_view.cpp` - 看消息如何发送

**第二天**：理解 Agent 核心
4. `src/agent/i_agent_core.h` - Agent 接口
5. `src/agent/react_agent_core.cpp` - ReAct 实现
6. `src/application/agent_service.cpp` - 业务逻辑

**第三天**：理解工具系统
7. `src/tool/i_tool.h` - 工具接口
8. `src/tool/tool_registry.cpp` - 工具注册
9. `src/tool/tools/filesystem_tool.cpp` - 具体工具示例

**第四天**：理解基础设施
10. `src/provider/openai_provider.cpp` - LLM 调用
11. `src/memory/memory_manager.cpp` - 内存管理
12. `src/permission/permission_manager.cpp` - 权限检查

---

## 修改建议

### ✅ 推荐的修改方式

#### 1. 添加新工具
```cpp
// src/tool/tools/my_tool.h
class MyTool : public ITool {
    // 实现接口方法
};

// 在 main.cpp 或初始化代码中注册
ToolRegistry::instance().registerTool(new MyTool());
```

#### 2. 修改 UI 样式
```cpp
// src/ui/chat_view.cpp::setupUi()
m_sendBtn->setStyleSheet("background-color: #4caf50;");  // 改颜色
```

#### 3. 添加新 Provider
```cpp
// src/provider/my_provider.h
class MyProvider : public ILLMProvider {
    // 实现接口方法
};

// 注册
ProviderManager::instance().registerProvider("my", new MyProvider());
```

#### 4. 修改 Agent 行为
```cpp
// src/agent/react_agent_core.cpp
void ReactAgentCore::processThought() {
    // 修改思考逻辑
    if (hasAction(m_currentThought)) {
        // 自定义行动处理
    }
}
```

#### 5. 调整配置参数
```cpp
// src/common/types.h::MemoryConfig
struct MemoryConfig {
    int tokenThreshold = 8000;  // 调整阈值
    int keepFirst = 5;          // 保留更多开头
    int keepLast = 10;          // 保留更多结尾
};
```

### ⚠️ 注意事项

1. **不要直接删除文件** - 先注释掉，确保编译通过
2. **修改头文件后重新编译** - Qt 的 MOC 需要重新生成
3. **测试后再提交** - 确保所有功能正常
4. **备份原文件** - 使用 git 或手动备份

---

## 常见问题

### Q1: 如何修改默认模型？
**A**: 修改 `src/common/constants.h` 中的 `DEFAULT_MODEL`，或在设置界面修改。

### Q2: 如何添加工具？
**A**: 
1. 继承 `ITool` 接口
2. 实现 `name()`, `description()`, `execute()` 等方法
3. 注册到 `ToolRegistry`

### Q3: 如何修改 UI 颜色？
**A**: 在对应 UI 文件的 `setupUi()` 方法中修改 `setStyleSheet()`。

### Q4: 如何添加新的 LLM Provider？
**A**:
1. 继承 `ILLMProvider` 接口
2. 实现 `chat()` 和 `chatStream()` 方法
3. 注册到 `ProviderManager`

### Q5: 对话内存如何压缩？
**A**: 参考 `src/memory/conversation_history.cpp::compressMiddle()`，保留开头和结尾，中间生成摘要。

### Q6: 如何调试代码？
**A**: 使用 `LOG_INFO("message")` 输出日志，日志文件在 `~/.clawpp/logs/app.log`。

### Q7: 权限检查在哪里？
**A**: `src/permission/permission_manager.cpp::checkPermission()`，工具执行前会调用。

---

## 快速修改清单

### 想修改 UI？
- 📍 `src/ui/chat_view.cpp` - 聊天界面
- 📍 `src/ui/main_window.cpp` - 主窗口
- 📍 `src/ui/widgets/message_bubble.cpp` - 消息气泡

### 想修改 Agent 逻辑？
- 📍 `src/agent/react_agent_core.cpp` - ReAct 循环
- 📍 `src/application/agent_service.cpp` - 业务逻辑

### 想添加工具？
- 📍 `src/tool/tools/` - 工具实现目录
- 📍 `src/tool/i_tool.h` - 工具接口定义

### 想修改配置？
- 📍 `src/common/constants.h` - 常量定义
- 📍 `config/default_config.json` - 默认配置
- 📍 `src/common/types.h` - 数据结构

### 想修改 LLM 调用？
- 📍 `src/provider/openai_provider.cpp` - OpenAI 实现
- 📍 `src/infrastructure/network/http_client.cpp` - HTTP 请求

---

## 构建和运行

### 构建步骤
```bash
cd cpqclaw
mkdir build && cd build
cmake ..
cmake --build .
```

### 运行
```bash
./bin/ClawPP.exe  # Windows
./bin/ClawPP      # Linux/Mac
```

### 清理重建
```bash
rm -rf build
mkdir build && cd build
cmake ..
cmake --build .
```

---

## 下一步

1. **阅读 `src/common/types.h`** - 理解所有数据结构
2. **运行项目** - 先让项目跑起来
3. **修改一个小功能** - 比如按钮颜色
4. **添加简单工具** - 比如 "hello world" 工具
5. **深入理解 Agent 循环** - 阅读 `react_agent_core.cpp`

---

## 资源链接

- **Qt 文档**: https://doc.qt.io/
- **CMake 文档**: https://cmake.org/documentation/
- **OpenAI API**: https://platform.openai.com/docs/api-reference

---

**祝你修改愉快！** 🎉

遇到问题可以随时查看对应文件的注释，所有关键文件都已添加中文注释。
