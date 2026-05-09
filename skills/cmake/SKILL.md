---
nanobot:
  description: "CMake 构建系统配置技能"
  requires:
    bins:
      - cmake
  always: false
---

# CMake 构建技能

## 何时使用

当用户要求进行以下操作时使用此技能：
- 创建或修改 CMakeLists.txt
- 配置项目构建
- 添加依赖库
- 设置编译选项

## 使用步骤

### 1. 项目配置

基本 CMakeLists.txt 结构：

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyProject VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
```

### 2. 查找依赖

查找 Qt6 组件：

```cmake
find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Widgets 
    Network 
    Sql
)
```

### 3. 添加源文件

添加可执行文件：

```cmake
add_executable(MyApp
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
)
```

### 4. 链接库

链接 Qt 库：

```cmake
target_link_libraries(MyApp PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)
```

### 5. 包含目录

设置包含目录：

```cmake
target_include_directories(MyApp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
```

## 常用命令

### 构建项目

```bash
cmake -B build -G Ninja
cmake --build build
```

### 安装

```bash
cmake --install build --prefix /path/to/install
```

### 清理

```bash
cmake --build build --target clean
```

## Qt 特定配置

### 启用 MOC

```cmake
set(CMAKE_AUTOMOC ON)
```

### 启用资源编译

```cmake
set(CMAKE_AUTORCC ON)
```

### 启用 UI 编译

```cmake
set(CMAKE_AUTOUIC ON)
```

## 注意事项

- 使用 `PRIVATE`、`PUBLIC`、`INTERFACE` 正确设置可见性
- 避免使用全局的 `include_directories()` 和 `link_libraries()`
- 使用现代 CMake 语法（target-based）
- 注意 Windows 平台的导出宏
