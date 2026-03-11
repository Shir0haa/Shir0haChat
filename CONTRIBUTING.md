# CONTRIBUTING.md

# 项目协作指南
本指南用于规范团队协作、代码管理与文档管理流程。
---

# 一、文档规范

本项目的文档统一格式要求如下：

### 1. 唯一 `.odt` 文件

项目主文档采用 **唯一一个 `.odt` 文件**：

```
docs/ShirohaChat项目文档.odt
```

### 2. 除上述 `.odt` 外，所有文档均为 `.md`

即所有 API 文档、架构说明等全部使用 Markdown 格式：

```
docs/api-design.md
docs/setup-guide.md
docs/architecture.md
```

### 3. 文档提交原则

* 任何文档更新都必须走 PR 流程
* 文档必须可读、结构清晰
* 开发者修改代码时应同步更新相关文档

### 4. 目录及文件命名规则

* 目录名禁止中文，统一使用小写字母和短横线（`-`）分隔
* 文件名同上，禁止使用空格和特殊字符 （.odt文件除外）

---

# 二、分支策略

本项目采用 ：

```
main       → 可发布、可演示、稳定版本
dev        → 主开发分支（所有功能合流点）
feature/*  → 功能开发分支（每个任务一个）
```

### 分支说明

| 分支               | 说明             | 权限                |
| ---------------- | -------------- | ----------------- |
| `main`           | 仅存放稳定可发布/答辩版本  | 仅负责人/Tech Lead 合并 |
| `dev`            | 所有功能最终整合的主开发分支 | 所有开发，但必须通过 PR     |
| `feature/<task>` | 每个功能的开发分支      | 对应任务的负责开发者        |

### 注意

* 禁止任何人直接 push 到 `dev` 或 `main`

---

# 三、任务与 Issue 流程

### 1. PM 创建 Issue

每个开发任务需要对应一个 Issue，内容包含：

* 功能描述
* 验收标准
* 截止日期
* 负责人
* 标签（`frontend`, `backend`, `bug`, `docs` 等）

### 2. 开发者领取任务后创建分支

```bash
git switch dev
git pull origin dev
git switch -c feature/<task-name>
```

示例：

```
feature/login
feature/chat-list
feature/file-upload
feature/friend-search
```

---

# 四、代码开发规范

### 1. 提交信息规范

采用简化版 Conventional Commits：

```
feat: 实现登录接口
fix: 修复 token 校验异常
refactor: 优化用户缓存结构
docs: 更新 API 文档
test: 增加单元测试
```

### 2. 小步提交

每次提交应解决单一问题，不得将无关修改混在一起。

### 3. 保持 feature 分支同步

```bash
git pull --rebase origin dev
```

---

# 五、Pull Request（PR）流程

### 1. 创建 PR（feature → dev）

开发完成后：

```bash
git push origin feature/<task>
```

在 GitHub 发起 PR，要求：

* 填写 PR 描述
* 关联 Issue：`Closes #xx`
* 附上截图/测试方法（如果适用）
* 自动触发 CI（构建、lint、测试）

---

# 六、合并策略

| 场景      | 分支            | 合并方式                       |
| ------- | ------------- | -------------------------- |
| 功能开发完成  | feature → dev | **Squash and Merge**（整洁历史） |
| 阶段性版本发布 | dev → main    | **No-FF Merge**（保持历史节点）    |

合并 dev → main 的时机：

* 阶段性验收
* 演示版准备
* 项目提交

示例：

```bash
git switch main
git merge --no-ff dev
git push origin main
```

---

# 七、CI / 自动化

项目可在 `.github/workflows/ci.yml` 中配置以下检查：

* 编译检查
* 单元测试（如有）
* PR 必须通过所有检查才能合并

---

# 八、非开发成员职责

| 角色        | 职责                           |
| --------- | ---------------------------- |
| PM        | Issue 管理、需求拆解、迭代规划           |
| QA        | 测试 dev 或 main，提交 bug Issue   |
| 文档人员      | 修改 `.md` 文档（唯一 `.odt` 由主笔维护） |
| Tech Lead | Code Review、合并权、质量控制         |
| 开发人员      | feature 开发、PR、Review、文档同步    |

---

# 九、目录结构

```
project/
  backend/
  frontend/
  src/
  tests/
  docs/
    ShirohaChat项目文档.odt     ← 唯一 odt 文档
    api-design.md
    architecture.md
    setup-guide.md
  README.md
  CONTRIBUTING.md
```

---

# 十、冲突处理流程

1. 拉取最新 dev
2. 本地解决冲突
3. 自测代码能否运行
4. push 后 PR 自动更新
5. reviewer 再次审核

不要在 GitHub 网页冲突编辑器中处理复杂冲突。

---

# 十一、行为规范

* 不得跳过 PR 流程
* 不得随意修改他人代码
* 不得提交无意义注释或 console.log
* 私下讨论结论必须同步回 Issue/PR
* 新功能必须同步更新文档

---
