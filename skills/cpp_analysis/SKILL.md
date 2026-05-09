---
nanobot:
  description: "C++ 代码分析和重构技能"
  requires:
    bins: []
  always: false
---

# C++ 代码分析技能

## 何时使用

当用户要求进行以下操作时使用此技能：
- 分析 C++ 代码结构
- 重构代码
- 查找代码问题
- 优化代码性能

## 使用步骤

### 1. 代码结构分析

首先阅读代码文件，了解整体结构：

1. 使用 `read_file` 读取源文件
2. 识别类、函数和变量的定义
3. 分析依赖关系和包含关系

### 2. 问题检测

检查以下常见问题：

- 内存泄漏风险（裸指针、未释放资源）
- 空指针解引用风险
- 未初始化变量
- 资源管理问题
- 线程安全问题

### 3. 重构建议

根据分析结果提供重构建议：

- 使用智能指针替代裸指针
- 使用 RAII 管理资源
- 提取重复代码为函数
- 简化复杂条件表达式
- 改善命名和代码组织

### 4. Qt 特定优化

针对 Qt 代码的优化建议：

- 使用 Qt 父子对象机制管理内存
- 避免在头文件中使用 `using namespace`
- 使用信号槽进行对象间通信
- 使用 `QString` 替代 `std::string` 进行 UI 操作

## 代码示例

### 智能指针使用

```cpp
// 不推荐
QWidget* widget = new QWidget(this);

// 推荐（Qt 父子对象机制）
QWidget* widget = new QWidget(this);  // Qt 会自动管理

// 或者使用智能指针
auto widget = std::make_unique<QWidget>();
```

### RAII 资源管理

```cpp
// 不推荐
QFile* file = new QFile(path);
file->open(QIODevice::ReadOnly);
// ... 忘记关闭和删除

// 推荐
QFile file(path);
if (file.open(QIODevice::ReadOnly)) {
    // 使用文件
    // 自动关闭和释放
}
```

## 注意事项

- 重构前确保有测试覆盖
- 一次只做一个改动
- 保持代码风格一致
- 注意 Qt 的内存管理规则
