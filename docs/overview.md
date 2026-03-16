# 项目概览

## 项目定位

AudioPlaybackConnector 是一个面向 Windows 10 2004+ 的蓝牙音频接收（A2DP Sink）连接工具。它常驻通知区，提供系统原生 API 的轻量桌面入口，而不是自建蓝牙协议栈或音频播放能力。

当前实现重点是：

- 常驻通知区，保持无主窗口工作流
- 左键托盘图标弹出 `DevicePicker` 连接蓝牙设备
- 右键菜单提供设置入口、系统蓝牙设置入口和退出入口
- 独立设置窗口集中管理长期配置
- 持久化开机自启动、最近连接设备和固定设备列表
- 启动后按配置自动连接最近设备和固定设备
- 如果已存在的自启动任务指向失效或旧的 exe，程序会在当前副本启动时自动修复为当前路径

## 当前用户可见行为

程序运行后的主要行为如下：

1. 启动后仅显示通知区图标，不显示传统主窗口
2. 左键托盘图标时弹出系统设备选择器，用户可手动连接蓝牙音频设备
3. 右键托盘图标时弹出菜单，可打开设置窗口、系统蓝牙设置或退出
4. 设置窗口允许配置：
   - 开机自启动
   - 自动连接最近连接设备
   - 自动连接固定设备列表
   - 固定设备列表增删
   - 最近连接设备查看、移除、清空
5. 如果存在活动连接，退出时会显示确认 Flyout；该 Flyout 只负责确认退出，不再承载长期设置

仓库根目录中的 `AudioPlaybackConnector.gif` 是整体交互的历史预览；当前实现比预览额外增加了独立设置窗口和更完整的自动连接能力。

## 系统与依赖前提

- 操作系统：Windows 10 2004（内部版本 19041）及以上
- UI 能力：需要 `DesktopWindowXamlSource`
- 蓝牙音频能力：需要 `AudioPlaybackConnection`
- 构建工具链：Visual Studio / MSBuild，项目文件使用 `v142`
- 运行时依赖：Win32 API、C++/WinRT、WIL、Shell API、Direct2D
- 自启动实现：Windows Task Scheduler COM API

程序启动时会通过 `ApiInformation::IsTypePresent(...)` 检查 `DesktopWindowXamlSource` 与 `AudioPlaybackConnection` 是否可用；不满足条件时直接报错并退出。

## 仓库结构总览

### 核心运行时代码

- `AudioPlaybackConnector.cpp`：程序入口、消息循环、托盘菜单、退出确认、主 `DevicePicker`
- `AudioPlaybackConnector.h`：全局状态、消息常量、运行时模块聚合入口
- `SettingsModels.hpp`：设置模型与持久化结构定义
- `SettingsUtil.hpp`：`SettingsStore`，负责默认值、读取、写入、旧配置迁移
- `AutoConnectPlanner.hpp`：根据设置生成自动连接队列
- `AutostartManager.hpp`：通过 Task Scheduler COM 管理开机自启动
- `SettingsWindow.hpp`：独立设置窗口及其 XAML Island UI
- `ConnectionController.hpp`：统一连接入口、最近设备回写、自动连接调度

### 通用辅助模块

- `I18n.hpp`：翻译资源加载与运行时字符串查找
- `Util.hpp`：UTF-8/UTF-16 转换、模块路径工具
- `Direct2DSvg.hpp`：SVG 到托盘 `HICON` 的转换
- `FnvHash.hpp`：翻译查表使用的 FNV-1a 哈希

### 工程与资源

- `AudioPlaybackConnector.vcxproj` / `.sln`：Visual Studio 工程与解决方案
- `AudioPlaybackConnector.rc` / `resource.h`：图标、版本信息、SVG 资源
- `AudioPlaybackConnector.manifest`：兼容性、DPI 感知、长路径声明
- `AudioPlaybackConnector.svg` / `.ico`：托盘图标资源

### 翻译与发布

- `translate/`：提取 `_()` / `C_()` 文案、维护 `.po`、生成 `.ymo` 和 `translate.rc`
- `.github/workflows/build.yaml`：tag push 触发的多平台构建与发布

## 当前实现特点

- 仍然是单项目、无测试的 Win32 + C++/WinRT 小型程序
- 主入口仍集中在 `AudioPlaybackConnector.cpp`，但设置、自启动、连接规划已拆为独立头文件模块
- 运行态仍依赖全局状态，但配置模型与连接规划已从“退出时重连”升级为“长期设置中心”
- 本地化采用自定义 `YMO` 资源查表，而不是标准 gettext 运行时

## 建议阅读顺序

- 先读 `docs/architecture.md` 理解启动、设置、自启动和自动连接流程
- 再读 `docs/code-map.md` 定位各模块职责
- 准备改代码前读 `docs/development-guide.md`
