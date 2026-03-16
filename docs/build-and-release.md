# 构建与发布

## 本地构建前提

当前仓库不是 CMake 项目，而是标准 Visual Studio / MSBuild 工程。

建议准备：

- Visual Studio 2019 或兼容 `v142` 工具集的构建环境
- Windows 10 SDK
- NuGet CLI 或 Visual Studio 自带的 NuGet Restore
- Python 3.7（用于翻译资源生成）
- Bash 环境（CI 使用 `shell: bash` 执行翻译脚本）

补充说明：

- `translate/gen_pot.sh` 与 `translate/gen_rc.sh` 是 shell 脚本；如果本地只有纯 PowerShell / CMD 环境，通常需要借助 Git Bash、MSYS2 或 WSL 才能直接执行这些脚本

## C++ 工程依赖

`packages.config` 声明了两个 NuGet 包：

- `Microsoft.Windows.CppWinRT`
- `Microsoft.Windows.ImplementationLibrary`

`AudioPlaybackConnector.vcxproj` 在构建前会检查这些包是否存在；如果缺失，会直接报错并阻止构建。

## 翻译资源生成

工程文件会编译 `translate/generated/translate.rc`，并引用脚本生成的 `.ymo` 文件，因此本地构建前需要先执行翻译资源生成。

### 生成链路

1. 源码中的 `_()` / `C_()` 文案通过 `translate/gen_pot.sh` 提取到 `translate/source/messages.pot`
2. 维护者在 `translate/source/zh_CN.po`、`translate/source/zh_TW.po` 中编写翻译
3. `translate/po2ymo.py` 把 `.po` 转成自定义 `.ymo`
4. `translate/gen_rc.sh` 生成 `translate/generated/translate.rc`

补充：

- 当前提取脚本会扫描 `AudioPlaybackConnector.cpp`、`ConnectionController.hpp`、`SettingsWindow.hpp`
- 如果新的本地化文案出现在其他文件中，必须先更新 `translate/gen_pot.sh`

### 本地生成步骤

在仓库根目录：

```powershell
cd translate
pip install -r requirements.txt
./gen_rc.sh
```

如果只是更新文案模板而不是生成资源，还可以执行：

```powershell
cd translate
./gen_pot.sh
```

## 本地构建步骤

### 方案一：使用 Visual Studio

1. 打开 `AudioPlaybackConnector.sln`
2. 先确保已恢复 NuGet 包
3. 先执行翻译资源生成
4. 选择需要的平台与配置，例如 `Release|x64`
5. 构建解决方案

### 方案二：使用命令行

```powershell
nuget restore AudioPlaybackConnector.sln
msbuild AudioPlaybackConnector.sln "-p:Configuration=Release;Platform=x64"
```

可选平台与 CI 保持一致：

- `x64`
- `Win32`
- `ARM64`
- `ARM`

注意：项目里 `Win32` 输出文件名会带 `32` 后缀，x64 则带 `64` 后缀。

## 输出产物

按当前 CI 命名约定，Release 主要输出为：

- `x64/Release/AudioPlaybackConnector64.exe`
- `Release/AudioPlaybackConnector32.exe`
- `ARM64/Release/AudioPlaybackConnectorARM64.exe`
- `ARM/Release/AudioPlaybackConnectorARM.exe`

对应的 `.pdb` 也会一起生成。

## GitHub Actions 发布流程

`.github/workflows/build.yaml` 只在 tag push 时触发：

```yaml
on:
  push:
    tags: [ '**' ]
```

### 工作流步骤

1. `actions/checkout@v2` 检出仓库
2. `microsoft/setup-msbuild@v1.0.0` 注入 msbuild
3. `nuget/setup-nuget@v1` 注入 nuget
4. `actions/setup-python@v1` 安装 Python 3.7
5. 进入 `translate/` 安装依赖并执行 `gen_rc.sh`
6. `nuget restore AudioPlaybackConnector.sln`
7. 用 PowerShell 后台作业并行构建 4 个平台
8. 上传各平台 exe 与 pdb 为 artifact
9. 创建 draft GitHub Release
10. 上传 4 个可执行文件为 release asset

## 发布维护注意事项

- 改了输出文件名、平台矩阵或工程配置，必须同步修改工作流
- 改了翻译脚本、依赖或生成目录，必须同步修改工作流中的翻译步骤
- 改了版本号时，通常还要同步 `AudioPlaybackConnector.rc` 中的版本资源

## 文档与流程一致性要求

当以下文件变更时，必须同步检查本文件：

- `AudioPlaybackConnector.vcxproj`
- `packages.config`
- `translate/*`
- `.github/workflows/build.yaml`
- `AudioPlaybackConnector.rc`

如果只是做文档改动，一般不需要完整编译；如果改动运行时代码、资源、翻译或构建配置，则应至少完成一次对应平台的本地构建验证。
