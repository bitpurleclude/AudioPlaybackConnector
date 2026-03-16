# AudioPlaybackConnector 开发文档

本目录是 AudioPlaybackConnector 的中文开发手册，目标是帮助开发者和 agent 在不先读完整个仓库的情况下，快速理解：

- 这个程序解决什么问题
- 程序运行时的界面与行为是什么样
- 仓库里每个核心文件负责什么
- 构建、翻译、发布流程如何串起来
- 后续改动应遵循什么约束

## 建议阅读顺序

1. `overview.md`：先理解项目定位、运行效果和仓库结构
2. `architecture.md`：再看运行架构、生命周期与核心数据流
3. `code-map.md`：定位代码入口、辅助头文件和资源职责
4. `build-and-release.md`：需要编译或出包时再读
5. `localization.md`：涉及文案、本地化或资源生成时必读
6. `development-guide.md`：准备修改代码前必读
7. `known-issues.md`：了解当前技术债、风险与待验证点
8. `agent-development-spec.md`：完整的人类版开发规范

## 文档索引

### 项目与架构

- `overview.md`：项目概览、系统要求、用户使用路径、目录总览
- `architecture.md`：启动流程、托盘菜单、设置窗口、自动连接、自启动、设置持久化、图标主题切换
- `code-map.md`：按文件和模块解释职责、关键入口、改动风险点

### 工具链与维护

- `build-and-release.md`：本地构建前提、翻译资源生成、GitHub Actions 发布流程
- `localization.md`：`po -> ymo -> rc` 翻译流水线与维护方式
- `development-guide.md`：常见改动剧本、验证策略、文档同步要求

### 风险与规范

- `known-issues.md`：当前实现中的已观察问题与待验证疑点
- `agent-development-spec.md`：完整开发规范
- 根目录 [`AGENTS.md`](../AGENTS.md)：给 agent 的精简硬规则

## 文档原则

- 以当前仓库事实为准，不臆造不存在的模块、测试体系或构建工具
- 明确区分“已确认事实”和“待验证疑点”
- 文档不替代代码；当代码变化时，应优先更新对应文档而不是让文档过期

## 维护约定

- 改动用户可见行为时，至少同步更新 `overview.md`、`architecture.md` 或 `development-guide.md`
- 改动构建、翻译、资源、发布流程时，必须同步更新对应专题文档
- 改动仓库约束时，`agent-development-spec.md` 与根目录 `AGENTS.md` 必须同时更新
