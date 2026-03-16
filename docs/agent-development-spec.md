# Agent 开发规范

本文件是面向开发者与 agent 的完整规范说明。根目录 `AGENTS.md` 是它的精简执行版。

## 1. 启动工作前的阅读顺序

任何 agent 在修改仓库前，应至少阅读：

1. `docs/README.md`
2. `docs/overview.md`
3. `docs/architecture.md`
4. `docs/code-map.md`
5. 与本次改动直接相关的专题文档

## 2. 当前仓库事实

- 这是一个单项目 Win32 + C++/WinRT + XAML Island 桌面程序
- 主入口仍在 `AudioPlaybackConnector.cpp`
- 设置模型、自启动、自动连接规划和设置窗口已经拆成独立头文件模块
- 长期配置通过 `AudioPlaybackConnector.json` 保存
- 自启动通过 Task Scheduler COM API 管理
- 翻译通过 `translate/` 生成自定义 `YMO` 资源

## 3. 模块职责约束

### `AudioPlaybackConnector.cpp`

允许：

- 调整入口、消息处理、托盘菜单、退出流程、主 `DevicePicker`

不鼓励：

- 把设置存储、自启动细节、设置窗口内部 UI、自动连接规划重新塞回主文件

### `AudioPlaybackConnector.h`

- 不要轻易继续扩大这里的全局状态定义
- 若必须新增全局状态，需说明初始化点、使用方和清理路径

### `SettingsModels.hpp` / `SettingsUtil.hpp`

- 新增设置项时，必须同时更新默认值、读取、写入、迁移、文档
- 不要破坏旧配置兼容性

### `AutoConnectPlanner.hpp`

- 只负责队列规划，不负责实际连接
- 默认语义是“最近设备优先，固定设备补集追加”

### `AutostartManager.hpp`

- 只负责任务计划程序状态查询与任务创建/删除
- 除非需求明确变化，不要私自改成注册表 `Run` 或管理员级任务
- 当前语义是“当前用户登录后延迟 15 秒启动”

### `SettingsWindow.hpp`

- 只负责独立设置窗口
- 窗口必须保持单实例
- 列表与开关操作必须即时生效
- “Add device” 只加入固定设备列表，不直接连接

### `ConnectionController.hpp`

- 统一连接入口
- 连接成功后必须回写最近设备
- 失败路径必须清理连接表中的中间状态

### `I18n.hpp` 与 `translate/`

- 所有用户可见文本都必须使用 `_()` 或 `C_()`
- 如果新文案出现在新的文件中，必须同步更新 `translate/gen_pot.sh`
- 不要手工维护 `.ymo` 生成产物

## 4. 文档同步要求

### 改动用户可见行为

至少同步：

- `docs/overview.md`
- `docs/architecture.md`

### 改动代码结构或职责边界

至少同步：

- `docs/code-map.md`
- `docs/development-guide.md`
- 如规则变化，再同步 `docs/agent-development-spec.md` 与 `AGENTS.md`

### 改动翻译、构建、发布、资源

至少同步：

- `docs/localization.md`
- `docs/build-and-release.md`
- 必要时 `docs/known-issues.md`

## 5. 验证策略

### 仅文档改动

至少完成：

- 文档互链检查
- 规则文件一致性检查

### 运行时代码改动

至少完成：

- 对应平台构建
- 与改动相关的手工回归

建议回归：

- 程序能启动
- 托盘图标正常显示
- 左键主 `DevicePicker`、右键菜单正常
- 设置窗口打开、关闭、聚焦正常
- 手工连接、自动连接、断开清理正常
- 自启动开关与任务计划程序状态一致

## 6. 禁止事项

- 不要无说明地继续扩大头文件中的全局状态
- 不要新增用户可见文本而不本地化
- 不要在新头文件中加入 `_()` 文案却不更新 `translate/gen_pot.sh`
- 不要修改设置模型却不处理旧配置兼容
- 不要修改构建或发布流程却不更新文档
- 不要把未验证推测写成已确认事实

## 7. 完成定义

一次改动只有在以下条件都满足时，才算完成：

- 代码或文档已更新
- 所有受影响的专题文档已同步
- `AGENTS.md` 与本文件没有冲突
- 对应验证已执行
- 没有把未经验证的推测包装成结论
