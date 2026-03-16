# 代码地图

本文件按“文件 / 模块职责”组织，帮助在修改前快速判断逻辑应放在哪里。

## 顶层入口与全局状态

### `AudioPlaybackConnector.cpp`

这是程序主入口，负责把各模块串起来。

主要职责：

- `wWinMain`：初始化 COM、公用翻译表、主窗口与主 XAML Island
- `PreTranslateXamlMessage(...)`：把消息分发给主 XAML Island 与设置窗口 XAML Island
- `WndProc(...)`：处理托盘消息、主题变化、自动连接驱动、退出清理
- `SetupFlyout()`：创建退出确认 Flyout
- `SetupMenu()`：创建右键菜单
- `SetupDevicePicker()`：初始化主 `DevicePicker`
- `SetupSvgIcon()` / `UpdateNotifyIcon()`：托盘图标生成与刷新

应继续保留在这里的内容：

- Win32 入口
- 主消息循环
- 托盘与主窗口宿主相关逻辑

不应继续堆在这里的内容：

- 设置存储细节
- 任务计划程序细节
- 设置窗口内部 UI 逻辑
- 自动连接队列规划

### `AudioPlaybackConnector.h`

这是全局状态聚合点。

当前包含：

- WinRT 命名空间别名
- 自定义消息常量
- 主窗口 / 主 XAML / 托盘 / `DevicePicker` / 连接表 / 图标 / `g_settings`
- 运行时模块头文件聚合

风险点：

- 仍然直接定义了大量全局状态
- 如果未来引入新的 `.cpp` 并直接包含它，可能产生重复定义问题

## 设置与自动连接模块

### `SettingsModels.hpp`

只定义数据结构：

- `SavedDevice`
- `AppSettings`
- `CONFIG_NAME`
- `MAX_RECENT_DEVICES`

它不处理任何 IO，不应混入逻辑。

### `SettingsUtil.hpp`

这里的 `SettingsStore` 是配置中心。

主要职责：

- `LoadSettings()`：读取 JSON 并迁移旧字段
- `SaveSettings()`：写回当前结构
- `RememberRecentDevice(...)`：更新最近设备
- `AddPreferredDevice(...)` / `RemovePreferredDevice(...)`
- `RemoveRecentDevice(...)` / `ClearRecentDevices()`
- 内部归一化：去重、排序、限制最近设备上限

改动规则：

- 新增设置项必须同时补默认值、读取、写入、文档、兼容行为
- JSON 结构变化必须保持向后兼容

### `AutoConnectPlanner.hpp`

职责单一：

- 根据 `g_settings` 生成启动自动连接队列
- 按“最近设备优先，固定设备补集追加”的规则合并
- 以设备 ID 去重

不要把真正的连接动作放进这里。

### `AutostartManager.hpp`

负责 Windows Task Scheduler COM 交互。

主要职责：

- `IsEnabled()`：读取任务状态
- `Enable()`：创建或更新当前用户登录自启动任务
- `Disable()`：删除任务
- `GetStatus()` / `RepairIfNeeded()`：检测任务目标路径是否失效或错位，并在需要时重建到当前 exe

这里应只处理任务计划程序本身，不负责 UI 文案或设置窗口刷新。

### `ConnectionController.hpp`

这是连接路径统一入口。

主要职责：

- `ConnectDevice(DevicePicker, DeviceInformation)`：手工或自动连接统一逻辑
- `ConnectDevice(DevicePicker, std::wstring_view)`：按设备 ID 查找再连接
- `ConnectConfiguredDevices()`：消费自动连接队列

关键副作用：

- 成功后调用 `SettingsStore::RememberRecentDevice(...)`
- 失败后清理 `g_audioPlaybackConnections`
- 成功后刷新设置窗口

## 设置窗口模块

### `SettingsWindow.hpp`

负责独立设置窗口的全部 UI 行为。

主要职责：

- 单实例窗口创建与聚焦
- 设置窗口自己的 XAML Island 生命周期
- 开关与按钮事件绑定
- 固定设备 / 最近设备列表刷新
- “添加设备”专用 `DevicePicker`

当前约束：

- 只负责设置界面，不直接承载主托盘菜单或退出确认
- 设备添加只写入固定列表，不直接发起连接

## 通用辅助模块

### `I18n.hpp`

负责自定义 `YMO` 资源的加载与字符串查找。

关键事实：

- `LoadTranslateData()` 现在由 `wWinMain` 显式调用
- `_()` / `C_()` 是所有用户可见文本的唯一入口

### `Util.hpp`

提供：

- `Utf8ToUtf16()`
- `Utf16ToUtf8()`
- `GetModuleFsPath()`

保持无状态，不要混入业务逻辑。

### `Direct2DSvg.hpp`

只负责把 SVG 文本转换为托盘 `HICON`，不要在这里加入通知区或主题判断逻辑。

### `FnvHash.hpp`

只为本地化查表服务，不建议继续叠加其他用途。

## 工程与资源

### `pch.h` / `pch.cpp`

定义整个工程的公共编译环境。当前新增了：

- Task Scheduler 相关头
- `algorithm` / `chrono` / `unordered_set` / `vector` 等标准库头

### `AudioPlaybackConnector.vcxproj`

当前与本次功能直接相关的事实：

- 链接依赖新增 `taskschd.lib`
- 新增头文件被加入工程
- 仍使用 `translate/generated/translate.rc`

### `AudioPlaybackConnector.rc` / `resource.h` / `.manifest`

仍然负责图标、版本资源、SVG 资源和运行清单。

## 翻译与自动化

### `translate/gen_pot.sh`

当前会提取以下文件中的 `_()` / `C_()`：

- `AudioPlaybackConnector.cpp`
- `ConnectionController.hpp`
- `SettingsWindow.hpp`

如果未来把用户可见文本移动到新的源文件或头文件，必须同步更新这个脚本。

### `translate/source/*.po`

维护简体中文与繁体中文翻译。

### `translate/po2ymo.py` / `translate/gen_rc.sh`

把 `.po` 生成为 `.ymo` 并写出 `translate/generated/translate.rc`。

### `.github/workflows/build.yaml`

CI 会先生成翻译资源，再恢复 NuGet，再并行构建 4 个平台。

## 修改定位速查

### 需要改托盘菜单或退出行为

优先看：

- `AudioPlaybackConnector.cpp`
- `docs/architecture.md`

### 需要改设置模型或配置文件

优先看：

- `SettingsModels.hpp`
- `SettingsUtil.hpp`
- `docs/architecture.md`

### 需要改自动连接行为

优先看：

- `AutoConnectPlanner.hpp`
- `ConnectionController.hpp`
- `SettingsWindow.hpp`

### 需要改自启动

优先看：

- `AutostartManager.hpp`
- `SettingsWindow.hpp`
- `docs/development-guide.md`

### 需要改本地化

优先看：

- `I18n.hpp`
- `translate/gen_pot.sh`
- `translate/source/*.po`
- `docs/localization.md`
