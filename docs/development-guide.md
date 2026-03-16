# 开发指南

## 开发前必读

建议按以下顺序建立上下文：

1. `docs/overview.md`
2. `docs/architecture.md`
3. `docs/code-map.md`
4. 当前准备修改的源文件

如果改动涉及翻译、构建、发布或资源，再补读对应专题文档。

## 通用改动原则

- 保持小步修改，不顺手扩大重构范围
- 优先遵守现有模块边界，不要把无关逻辑重新塞回 `AudioPlaybackConnector.cpp`
- 新增用户可见文本必须本地化
- 新增设置必须同步默认值、读写、迁移和文档
- 改变用户行为时，同时更新文档与翻译
- 没有自动化测试时，更要控制改动面并执行手工验证

## 常见改动剧本

### 新增或调整右键菜单项

通常需要修改：

- `AudioPlaybackConnector.cpp` 中的 `SetupMenu()`
- 如有新文案，更新 `translate/`
- 如改变用户行为，更新 `docs/overview.md` / `docs/architecture.md`

最小验证：

- 右键托盘图标可以看到新项
- 鼠标和键盘呼出菜单时焦点行为正常
- 点击行为执行正确

### 调整设置窗口

通常需要修改：

- `SettingsWindow.hpp`
- 相关设置模型：`SettingsModels.hpp` / `SettingsUtil.hpp`
- 相关行为模块：`AutostartManager.hpp`、`AutoConnectPlanner.hpp` 或 `ConnectionController.hpp`

注意事项：

- 设置窗口必须保持单实例
- 所有操作应即时生效，不要引入“应用 / 取消”混合模式
- 固定设备添加流程只写配置，不直接连接

最小验证：

- 右键菜单可以打开设置窗口
- 重复打开时会聚焦现有窗口
- 开关切换、列表增删、清空最近设备后配置立即落盘

### 调整自动连接流程

通常需要修改：

- `AutoConnectPlanner.hpp`
- `ConnectionController.hpp`
- 必要时同步 `SettingsUtil.hpp`

注意事项：

- 保持“最近设备优先、固定设备补集追加”的语义，除非需求明确变更
- 去重单位必须是设备 ID
- 失败路径必须清理 `g_audioPlaybackConnections`

最小验证：

- 仅最近设备开启时，队列正确
- 仅固定设备开启时，队列正确
- 两者同时开启时，重复设备只连接一次

### 新增设置项

通常需要修改：

- `SettingsModels.hpp`
- `SettingsUtil.hpp`
- 对应 UI 或行为模块
- `docs/architecture.md`
- `docs/development-guide.md`

必须同时完成：

1. 增加默认值
2. 增加读取逻辑
3. 增加写入逻辑
4. 处理旧配置缺字段或旧字段迁移
5. 更新文档

最小验证：

- 无配置文件时可启动
- 旧配置文件存在时可启动
- 修改设置后重启仍生效

### 调整开机自启动

通常需要修改：

- `AutostartManager.hpp`
- `SettingsWindow.hpp`
- 如任务模型有变化，更新 `docs/architecture.md`

注意事项：

- 当前默认语义是“当前用户登录后 15 秒启动”
- 不要在没有明确需求时改成注册表 `Run` 或管理员级任务
- 失败时 UI 必须回滚到真实状态
- 当前实现会在启动和设置窗口刷新时自愈“已启用但目标 exe 已失效/错位”的任务；修改这一行为时要同时验证 portable 场景

最小验证：

- 开启后成功创建任务
- 关闭后成功删除任务
- 任务操作失败时不会留下“配置已开但系统任务未创建”的假状态
- 将任务指向旧路径后重新运行当前副本，任务会被修复到当前 exe

### 新增本地化文本

通常需要修改：

- 源码中的 `_()` / `C_()`
- `translate/source/messages.pot`
- `translate/source/zh_CN.po`
- `translate/source/zh_TW.po`

如果新文案位于新的源文件或头文件中，还必须检查：

- `translate/gen_pot.sh`

最小验证：

- 目标语言下文案可显示
- `messages.pot` 与两份 `.po` 已收录新文案

### 修改托盘图标或其他资源

通常需要修改：

- `AudioPlaybackConnector.svg` / `.ico`
- `AudioPlaybackConnector.rc`
- 必要时 `Direct2DSvg.hpp`

最小验证：

- 托盘图标正常显示
- 浅色 / 深色主题切换后可见性正常

## 建议手工回归

运行时代码改动后，至少回归：

- 程序可以启动并显示托盘图标
- 左键可以打开主设备选择器
- 右键可以打开菜单
- 设置窗口可以打开、关闭、再次聚焦
- 手工连接、断开、失败提示正常
- 自动连接在最近设备 / 固定设备两种模式下表现正确
- 开机自启动开关与任务计划程序实际状态一致
- 退出确认 Flyout 正常
- 系统主题切换后托盘图标刷新正常

## 文档同步规则

### 改动用户行为

至少同步：

- `docs/overview.md`
- `docs/architecture.md`
- `README.md` / `README.zh_CN.md`（如果用户可见能力发生明显变化）

### 改动代码结构

至少同步：

- `docs/code-map.md`
- `docs/agent-development-spec.md`
- 根目录 `AGENTS.md`（如果规则也受影响）

### 改动翻译、资源、构建或发布

至少同步：

- `docs/localization.md`
- `docs/build-and-release.md`
- 必要时 `docs/known-issues.md`

## 不建议的做法

- 把设置窗口、自启动、自动连接细节重新塞回 `AudioPlaybackConnector.cpp`
- 新增全局状态却不说明初始化与清理路径
- 只改 `SettingsModels.hpp`，却不改 `SettingsUtil.hpp`
- 在新头文件里加了 `_()` 文案，却不更新 `translate/gen_pot.sh`
- 修改构建或发布流程却不更新文档
