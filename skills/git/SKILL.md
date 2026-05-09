---
nanobot:
  description: "使用 Git 进行版本控制操作"
  requires:
    bins:
      - git
  always: false
---

# Git 技能

## 何时使用

当用户要求进行以下操作时使用此技能：
- 提交代码更改
- 查看提交历史
- 创建或切换分支
- 合并代码
- 查看文件差异

## 使用步骤

### 1. 查看状态

在执行任何 Git 操作前，先查看当前状态：

```bash
git status
```

### 2. 查看差异

查看未暂存的更改：

```bash
git diff
```

查看已暂存的更改：

```bash
git diff --staged
```

### 3. 提交更改

添加文件到暂存区：

```bash
git add <file>
```

提交更改：

```bash
git commit -m "提交信息"
```

### 4. 分支操作

查看所有分支：

```bash
git branch -a
```

创建新分支：

```bash
git checkout -b <branch-name>
```

切换分支：

```bash
git checkout <branch-name>
```

### 5. 查看历史

查看提交历史：

```bash
git log --oneline -10
```

## 注意事项

- 在提交前务必检查 `git status` 和 `git diff`
- 提交信息应简洁明了，描述本次更改的内容
- 不要提交敏感信息（密码、密钥等）
