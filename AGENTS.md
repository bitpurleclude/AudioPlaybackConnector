# AudioPlaybackConnector Agent Rules

在修改本仓库前，先阅读：

1. `docs/README.md`
2. `docs/overview.md`
3. `docs/architecture.md`
4. `docs/code-map.md`

## 硬规则

- 遵守现有模块职责，不要无说明地继续把不相关逻辑堆进 `AudioPlaybackConnector.cpp`
- 不要轻易扩大全局状态；若必须新增全局状态，要明确初始化、使用和清理路径
- 新增用户可见文本时，必须使用 `_()` 或 `C_()` 并同步更新 `translate/` 流程
- 如果新文案出现在新的源文件或头文件中，必须同步更新 `translate/gen_pot.sh`
- 新增设置项时，必须同时更新 `SettingsUtil.hpp` 中的默认值、读取、写入逻辑，并更新文档
- 自动连接默认语义是“最近设备优先，固定设备补集追加”；无明确需求不要改
- 自启动当前语义是“当前用户登录后延迟 15 秒启动”；无明确需求不要改实现方式
- 变更资源、清单、构建或发布流程时，必须同步更新相关文档，至少检查 `docs/build-and-release.md`
- 涉及翻译或资源生成时，不要手工维护 `.ymo` 生成产物
- 如果改动影响用户行为或代码结构，必须同步更新 `docs/` 中对应文档
- `docs/agent-development-spec.md` 是完整规范说明；本文件是精简执行版，二者不得冲突

## 常见改动指引

- 菜单 / 托盘 / 退出交互：看 `docs/architecture.md` 与 `docs/development-guide.md`
- 蓝牙连接 / 重连逻辑：看 `docs/architecture.md` 与 `docs/code-map.md`
- 设置持久化：看 `docs/code-map.md` 与 `docs/development-guide.md`
- 设置窗口 / 自启动 / 自动连接：看 `docs/architecture.md` 与 `docs/code-map.md`
- 本地化：看 `docs/localization.md`
- 构建 / 发布：看 `docs/build-and-release.md`

## 验证要求

- 仅文档改动：检查链接、文件引用和规范一致性
- 运行时代码改动：至少完成对应平台构建和相关手工回归，尤其是设置窗口、自动连接、自启动
- 构建或发布改动：校对本地命令、工作流和产物命名一致性
