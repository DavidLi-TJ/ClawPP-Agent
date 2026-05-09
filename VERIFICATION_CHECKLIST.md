# Claw++ 修复验证清单

## ✅ 问题 1: Session 切换数据消失

### 修复内容
- [x] `AgentService::setHistory()` 方法已添加
- [x] `SessionManager::switchSession()` 调用 `setHistory()`
- [x] `MainWindow::setupServices()` 创建 AgentService
- [x] `MainWindow::setupServices()` 连接 SessionManager 和 AgentService
- [x] 消息历史在切换时正确保存和恢复

### 验证步骤
1. 启动应用
2. 创建新 Session 并发送几条消息
3. 切换到另一个 Session
4. 切回第一个 Session
5. **预期**: 消息应该完整显示

---

## ✅ 问题 2: AI 不能操控本地文件/初始化

### 修复内容
- [x] `main.cpp` 中初始化 `ContextBuilder`
- [x] `ContextBuilder` 自动加载工作空间配置文件
- [x] 创建 `WORKSPACE_SETUP.md` 文档

### 验证步骤
1. 在工作目录创建 `templates/` 文件夹
2. 创建 `templates/AGENTS.md`, `templates/SOUL.md` 等配置文件
3. 重启应用
4. 发送消息询问 AI:"你能读取哪些配置文件？"
5. **预期**: AI 应该能读取并引用配置文件内容

---

## ✅ 问题 3: UI 现代化改造

### 修复内容
- [x] 删除传统菜单栏
- [x] 创建扁平化顶部工具栏
- [x] 移除所有二级菜单
- [x] 使用 Material Design 风格
- [x] 删除重复的控件创建代码
- [x] 清理不需要的成员变量

### 验证步骤
1. 启动应用
2. 检查顶部是否有简洁的工具栏
3. 检查是否没有传统菜单栏
4. 检查 "+ New" 和 "Settings" 按钮是否正常工作
5. 检查 Model 下拉框是否显示
6. **预期**: UI 简洁现代，类似 Claude Desktop/Cursor

---

## 🔧 技术修复清单

### 核心服务创建和连接
- [x] `AgentService` 在 `MainWindow` 中创建
- [x] `ProviderManager` 在 `MainWindow` 中创建
- [x] `MemoryManager` 在 `MainWindow` 中创建
- [x] `AgentService` 设置 Provider
- [x] `AgentService` 设置 Memory
- [x] `AgentService` 设置 ToolRegistry
- [x] `AgentService` 设置 PermissionManager
- [x] `SessionManager` 设置 AgentService

### 信号槽连接
- [x] `AgentService::responseChunk` → `ChatView::appendResponseChunk()`
- [x] `AgentService::responseComplete` → `ChatView::finishStreaming()`
- [x] `AgentService::usageReport` → `MainWindow::updateTokenCount()`
- [x] `AgentService::errorOccurred` → `ChatView::showError()`
- [x] `ChatView::messagesChanged` → `SessionManager::updateMessages()`

### ChatView 方法补充
- [x] `appendResponseChunk()` - 代理到 `onStreamChunk()`
- [x] `finishStreaming()` - 代理到 `onStreamComplete()`
- [x] `showError()` - 代理到 `onStreamError()`

### 代码清理
- [x] 删除 `m_settingsBtn` 成员变量 (不再需要)
- [x] 删除 `m_fileTree` 成员变量 (未使用)
- [x] 删除 `m_rightTabs` 成员变量 (未使用)
- [x] 删除 `m_undoAction` 和 `m_redoAction` (未实现)
- [x] 删除 `createRightPanel()` 中重复的控件创建
- [x] 删除不需要的菜单相关方法

---

## 📝 修改的文件列表

### 新增文件
- `WORKSPACE_SETUP.md` - 工作空间配置指南
- `FIXES_SUMMARY.md` - 修复总结文档

### 修改的核心文件
1. `src/application/agent_service.h` - 添加 `setHistory()` 声明
2. `src/application/agent_service.cpp` - 实现 `setHistory()` 方法
3. `src/application/session_manager.cpp` - 修改 `switchSession()` 调用 `setHistory()`
4. `src/main.cpp` - 添加 `ContextBuilder` 初始化
5. `src/ui/main_window.h` - 添加服务成员和方法声明
6. `src/ui/main_window.cpp` - 实现 `setupServices()` 和信号槽连接
7. `src/ui/chat_view.h` - 添加公共方法声明
8. `src/ui/chat_view.cpp` - 实现代理方法

### UI 优化文件
1. `src/ui/main_window.h` - 删除不需要的成员
2. `src/ui/main_window.cpp` - 删除旧菜单代码，实现 `setupModernUI()`

---

## 🎯 完整功能测试

### Session 管理
- [ ] 创建新 Session
- [ ] 切换 Session
- [ ] 重命名 Session
- [ ] 删除 Session
- [ ] Session 数据持久化

### 聊天功能
- [ ] 发送消息
- [ ] 接收流式响应
- [ ] 工具调用
- [ ] 错误处理
- [ ] Token 计数显示

### 文件操作
- [ ] AI 读取配置文件
- [ ] AI 使用工具读写文件
- [ ] ContextBuilder 加载配置

### UI 交互
- [ ] 顶部工具栏显示正常
- [ ] "+ New" 按钮创建 Session
- [ ] "Settings" 按钮打开设置
- [ ] Model 下拉框切换模型
- [ ] 侧边栏切换 (Sessions/Logs)

---

## ⚠️ 注意事项

1. **编译前清理构建目录**: `rm -rf build/*`
2. **确保 Qt 6.x 环境**: 需要 Qt 6.x 支持
3. **配置文件路径**: 工作目录的 `templates/` 文件夹
4. **API 配置**: 需要在设置中配置 API Key 和 Base URL

---

## 📚 参考文档

- [NanoBot ContextBuilder 实现](https://blog.csdn.net/A534173227/article/details/159416519)
- [Material Design 规范](https://material.io/design)
- [PROJECT_README.md](./PROJECT_README.md) - 项目阅读指南
