# Claw++ 项目重构与功能增强计划

## 概述

本计划基于现有代码库进行全面重构和功能增强，严格遵循C++最佳实践（声明在.h，实现在.cpp），并实现用户要求的五项核心功能。

---

## 第一部分：代码重构（按文件夹排查）

### 1.1 common/ 文件夹

| 文件 | 状态 | 说明 |
|------|------|------|
| `types.h` | ⚠️ 需重构 | Message、ResponseFormat、ProviderConfig、ChatOptions、MemoryConfig、AppConfig、Skill等struct包含toJson/fromJson等方法实现 |
| `result.h` | ✅ 保留 | 模板类必须在头文件实现 |
| `constants.h` | ✅ 无需重构 | 仅包含常量定义 |

**重构任务：**
- 创建 `types.cpp`，将所有struct方法实现移至.cpp

### 1.2 tool/ 文件夹

| 文件 | 状态 | 说明 |
|------|------|------|
| `i_tool.h` | ⚠️ 需重构 | toJson()方法有实现 |
| `tool_registry.h/cpp` | ✅ 已重构 | - |
| `tool_executor.h/cpp` | ✅ 已重构 | - |
| `tools/*.h/cpp` | ✅ 已重构 | - |

**重构任务：**
- 创建 `i_tool.cpp`，将toJson()实现移至.cpp

### 1.3 skill/ 文件夹

| 文件 | 状态 | 说明 |
|------|------|------|
| `skill.h` | ⚠️ 需重构 | Skill struct包含多个方法实现 |
| `skill_manager.h/cpp` | ✅ 已重构 | - |

**重构任务：**
- 创建 `skill.cpp`，将Skill方法实现移至.cpp

### 1.4 agent/ 文件夹

| 文件 | 状态 | 说明 |
|------|------|------|
| `i_agent_core.h` | ✅ 无需重构 | 纯虚接口 |
| `react_agent_core.h/cpp` | ⚠️ 需重构 | 实现全在.h中，.cpp为空 |

**重构任务：**
- 将 `react_agent_core.h` 中所有方法实现移至 `react_agent_core.cpp`

### 1.5 memory/ 文件夹

| 文件 | 状态 | 说明 |
|------|------|------|
| `i_memory_system.h` | ✅ 无需重构 | 纯虚接口 |
| `memory_manager.h/cpp` | ✅ 已重构 | - |
| `conversation_history.h/cpp` | ⚠️ 需重构 | 实现全在.h中，.cpp为空 |

**重构任务：**
- 将 `conversation_history.h` 中所有方法实现移至 `conversation_history.cpp`

### 1.6 provider/ 文件夹

| 文件 | 状态 | 说明 |
|------|------|------|
| `i_llm_provider.h` | ✅ 无需重构 | 纯虚接口 |
| `openai_provider.h/cpp` | ⚠️ 需重构 | 实现全在.h中，.cpp为空 |
| `provider_manager.h/cpp` | ⚠️ 需重构 | 实现全在.h中，.cpp为空 |

**重构任务：**
- 将 `openai_provider.h` 中所有方法实现移至 `openai_provider.cpp`
- 将 `provider_manager.h` 中所有方法实现移至 `provider_manager.cpp`

### 1.7 infrastructure/ 文件夹

#### 1.7.1 network/

| 文件 | 状态 | 说明 |
|------|------|------|
| `http_client.h/cpp` | ⚠️ 需重构 | 实现全在.h中，.cpp为空 |
| `sse_parser.h/cpp` | ✅ 已重构 | - |

**重构任务：**
- 将 `http_client.h` 中所有方法实现移至 `http_client.cpp`

#### 1.7.2 logging/

| 文件 | 状态 | 说明 |
|------|------|------|
| `logger.h/cpp` | ⚠️ 需重构 | 实现全在.h中，.cpp为空 |

**重构任务：**
- 将 `logger.h` 中所有方法实现移至 `logger.cpp`

#### 1.7.3 event/

| 文件 | 状态 | 说明 |
|------|------|------|
| `events.h` | ✅ 无需重构 | 仅struct定义 |
| `event_bus.h/cpp` | ✅ 可保留 | 模板方法必须在头文件 |

#### 1.7.4 database/

| 文件 | 状态 | 说明 |
|------|------|------|
| `database_manager.h/cpp` | ⚠️ 需重构 | 实现全在.h中，.cpp为空 |
| `session_store.h/cpp` | ✅ 已重构 | - |
| `conversation_store.h/cpp` | ✅ 已重构 | - |

**重构任务：**
- 将 `database_manager.h` 中所有方法实现移至 `database_manager.cpp`

#### 1.7.5 config/

| 文件 | 状态 | 说明 |
|------|------|------|
| `config_manager.h/cpp` | ✅ 已重构 | - |

### 1.8 ui/ 文件夹

| 文件 | 状态 | 说明 |
|------|------|------|
| `main_window.h/cpp` | ⚠️ 需重构 | SettingsDialog和MainWindow实现全在.h中 |
| `chat_view.h/cpp` | ⚠️ 需重构 | InputTextEdit和ChatView实现全在.h中 |
| `session_panel.h/cpp` | ✅ 已重构 | - |
| `widgets/message_bubble.h/cpp` | ⚠️ 需重构 | 实现全在.h中，.cpp为空 |
| `dialogs/permission_dialog.h/cpp` | ✅ 已重构 | - |

**重构任务：**
- 将 `main_window.h` 中SettingsDialog和MainWindow实现移至 `main_window.cpp`
- 将 `chat_view.h` 中InputTextEdit和ChatView实现移至 `chat_view.cpp`
- 将 `message_bubble.h` 中MessageBubble实现移至 `message_bubble.cpp`

---

## 第二部分：功能实现

### 2.1 Session搜索和新增问题修复

**问题分析：**
- main.cpp中MainWindow创建后没有连接sessionCreated信号
- SessionPanel发出sessionCreated信号但无接收者
- 需要SessionManager来管理session生命周期

**解决方案：**
1. 在main.cpp中初始化SessionManager
2. 连接SessionPanel::sessionCreated → SessionManager::createSession
3. 连接SessionManager::sessionCreated → SessionPanel::setSessions
4. 在MainWindow中集成SessionManager

**修改文件：**
- `src/main.cpp` - 初始化SessionManager
- `src/ui/main_window.h/cpp` - 集成SessionManager，连接信号槽

### 2.2 AI加载动画

**设计方案：**
1. 创建LoadingIndicator组件（自定义QWidget）
2. 使用QPropertyAnimation实现旋转/脉冲动画
3. 在ChatView中集成：
   - 流式响应开始时显示动画
   - 流式响应结束时隐藏动画
   - 动画显示在消息区域底部

**新增文件：**
- `src/ui/widgets/loading_indicator.h`
- `src/ui/widgets/loading_indicator.cpp`

**修改文件：**
- `src/ui/chat_view.h/cpp` - 集成LoadingIndicator
- `CMakeLists.txt` - 添加新文件

### 2.3 聊天气泡动态效果

**设计方案：**
1. 为MessageBubble添加淡入(FadeIn)动画
2. 使用QGraphicsOpacityEffect + QPropertyAnimation
3. 用户消息从右侧滑入，AI消息从左侧滑入
4. 动画时长：200-300ms

**修改文件：**
- `src/ui/widgets/message_bubble.h` - 添加动画相关成员
- `src/ui/widgets/message_bubble.cpp` - 实现动画效果

### 2.4 日志查看界面

**设计方案：**
1. 创建LogView组件：
   - 日志级别过滤按钮（DEBUG/INFO/WARN/ERROR）
   - 日志列表显示（时间戳、级别、消息）
   - 搜索/过滤功能
   - 实时日志更新

2. 创建CentralWidget容器：
   - 使用QTabWidget实现标签页切换
   - Tab1: ChatView（会话界面）
   - Tab2: LogView（日志界面）

3. 扩展Logger：
   - 添加信号机制支持UI实时更新
   - 添加日志缓存队列

**新增文件：**
- `src/ui/views/log_view.h`
- `src/ui/views/log_view.cpp`
- `src/ui/views/central_widget.h`
- `src/ui/views/central_widget.cpp`

**修改文件：**
- `src/infrastructure/logging/logger.h/cpp` - 添加信号和日志缓存
- `src/ui/main_window.h/cpp` - 使用CentralWidget替代直接使用ChatView
- `CMakeLists.txt` - 添加新文件

### 2.5 ChatView多Session切换

**设计方案：**
1. 重构ChatView支持session切换：
   - 每个session维护独立的MessageList
   - 切换session时保存当前session状态
   - 加载新session的消息历史

2. 集成SessionManager：
   - ChatView持有SessionManager引用
   - 监听sessionSwitched信号
   - 自动加载对应session的聊天记录

**修改文件：**
- `src/ui/chat_view.h/cpp` - 添加session管理逻辑
- `src/ui/main_window.h/cpp` - 连接SessionManager和ChatView
- `src/infrastructure/database/conversation_store.h/cpp` - 确保消息存储正确

---

## 第三部分：实施顺序

### 阶段一：代码重构（优先级：高）

1. **common/types.h** → types.cpp
2. **tool/i_tool.h** → i_tool.cpp
3. **skill/skill.h** → skill.cpp
4. **agent/react_agent_core.h** → react_agent_core.cpp
5. **memory/conversation_history.h** → conversation_history.cpp
6. **provider/openai_provider.h** → openai_provider.cpp
7. **provider/provider_manager.h** → provider_manager.cpp
8. **infrastructure/network/http_client.h** → http_client.cpp
9. **infrastructure/logging/logger.h** → logger.cpp
10. **infrastructure/database/database_manager.h** → database_manager.cpp
11. **ui/main_window.h** → main_window.cpp
12. **ui/chat_view.h** → chat_view.cpp
13. **ui/widgets/message_bubble.h** → message_bubble.cpp

### 阶段二：功能实现（优先级：中）

1. **Session搜索/新增修复** - 集成SessionManager
2. **AI加载动画** - 创建LoadingIndicator
3. **聊天气泡动画** - MessageBubble动画效果
4. **日志查看界面** - 创建LogView和CentralWidget
5. **ChatView多Session** - Session切换支持

### 阶段三：测试验证

1. 编译测试
2. 功能测试
3. UI交互测试

---

## 第四部分：文件变更清单

### 新增文件

```
src/types.cpp
src/tool/i_tool.cpp
src/skill/skill.cpp
src/ui/widgets/loading_indicator.h
src/ui/widgets/loading_indicator.cpp
src/ui/views/log_view.h
src/ui/views/log_view.cpp
src/ui/views/central_widget.h
src/ui/views/central_widget.cpp
```

### 修改文件

```
src/common/types.h
src/tool/i_tool.h
src/skill/skill.h
src/agent/react_agent_core.h
src/agent/react_agent_core.cpp
src/memory/conversation_history.h
src/memory/conversation_history.cpp
src/provider/openai_provider.h
src/provider/openai_provider.cpp
src/provider/provider_manager.h
src/provider/provider_manager.cpp
src/infrastructure/network/http_client.h
src/infrastructure/network/http_client.cpp
src/infrastructure/logging/logger.h
src/infrastructure/logging/logger.cpp
src/infrastructure/database/database_manager.h
src/infrastructure/database/database_manager.cpp
src/ui/main_window.h
src/ui/main_window.cpp
src/ui/chat_view.h
src/ui/chat_view.cpp
src/ui/widgets/message_bubble.h
src/ui/widgets/message_bubble.cpp
src/main.cpp
CMakeLists.txt
```

---

## 第五部分：技术要点

### C++最佳实践遵循

1. **声明与实现分离**：所有非模板类的实现在.cpp文件中
2. **头文件保护**：使用 `#pragma once` 或 `#ifndef/#define/#endif`
3. **命名空间**：所有代码在 `clawpp` 命名空间内
4. **成员变量命名**：使用 `m_` 前缀
5. **Qt信号槽**：使用新式Qt5连接语法
6. **内存管理**：使用Qt父子关系或智能指针

### Qt动画技术

1. **QPropertyAnimation**：属性动画
2. **QGraphicsOpacityEffect**：透明度效果
3. **QParallelAnimationGroup**：并行动画组
4. **QEasingCurve**：缓动曲线

### 日志系统扩展

1. **线程安全**：使用QMutex保护日志队列
2. **信号机制**：继承QObject，发出logAdded信号
3. **级别过滤**：使用QFlags或位掩码

---

## 附录：估算工作量

| 任务 | 预估复杂度 |
|------|-----------|
| 代码重构（13个文件） | 中等 |
| Session修复 | 简单 |
| 加载动画 | 简单 |
| 气泡动画 | 简单 |
| 日志界面 | 中等 |
| Session切换 | 中等 |
| 测试验证 | 简单 |

**总计**：中等复杂度项目
