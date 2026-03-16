# 本地化说明

## 总体机制

项目没有使用标准 gettext 运行时或 Windows 资源字符串表，而是采用自定义翻译流水线：

1. 源码里用 `_()` / `C_()` 标记用户可见文本
2. `translate/gen_pot.sh` 提取文案到 `translate/source/messages.pot`
3. 维护者编辑 `translate/source/*.po`
4. `translate/po2ymo.py` 把 `.po` 转成自定义二进制格式 `YMO`
5. `translate/gen_rc.sh` 生成 `translate/generated/translate.rc`
6. 程序运行时从 `YMO` 资源建立哈希查找表

## 代码中的入口

### 宏

- `_()`：普通翻译
- `C_()`：带上下文翻译

这些宏定义在 `I18n.hpp` 中。

### 运行时加载

`LoadTranslateData()` 现在会在 `wWinMain` 早期显式调用。它会：

- 根据当前线程 UI 语言查找 `YMO` 资源
- 解析资源头和偏移表
- 建立 `hash -> wchar_t*` 查找表

`Translate()` 会对原始字符串做 FNV-1a 哈希再查表；同时维护 `原始指针 -> 翻译指针` 的缓存。

## 当前提取范围

`translate/gen_pot.sh` 当前会扫描：

- `AudioPlaybackConnector.cpp`
- `ConnectionController.hpp`
- `SettingsWindow.hpp`

这点很重要：设置窗口和连接控制现在都在头文件里实现。如果未来把用户可见文本移到新的头文件或源文件，必须同步更新 `translate/gen_pot.sh`。

## 相关文件

### 模板与翻译源

- `translate/source/messages.pot`
- `translate/source/zh_CN.po`
- `translate/source/zh_TW.po`

### 生成脚本

- `translate/gen_pot.sh`
- `translate/po2ymo.py`
- `translate/gen_rc.sh`

### 生成结果

- `translate/generated/translate.rc`
- `translate/generated/zh_CN.ymo`
- `translate/generated/zh_TW.ymo`

其中：

- `translate/generated/translate.rc` 是脚本生成的资源脚本
- `.ymo` 文件是生成产物，不应手工维护

## 新增或修改文案的标准流程

### 1. 修改源码

在所有用户可见文本外围使用 `_()` 或 `C_()`。

例如：

```cpp
state.autostartCheckBox.Content(winrt::box_value(_(L"Start on Windows sign-in")));
```

### 2. 检查提取脚本覆盖范围

如果新文案在新的文件里，先更新 `translate/gen_pot.sh`。

### 3. 更新 POT 模板

```powershell
cd translate
./gen_pot.sh
```

### 4. 更新 PO 文件

同步更新：

- `translate/source/zh_CN.po`
- `translate/source/zh_TW.po`

### 5. 生成资源

```powershell
cd translate
pip install -r requirements.txt
./gen_rc.sh
```

### 6. 重新构建并验证

重点验证：

- 菜单、设置窗口、错误对话框的文案都已本地化
- 多行文本换行正常
- 新文案不会回退为英文

## 编码与实现细节

- `po2ymo.py` 默认使用 `utf-16le`
- `SettingsStore` 的 JSON 文件使用 UTF-8 读写
- 原始英文字符串变化会改变哈希，等同于新增一条文案

## 当前维护要求

- 所有用户可见文本都必须走 `_()` / `C_()`
- 不要手工维护 `.ymo` 生成产物
- 翻译提取范围变化时，先改 `translate/gen_pot.sh`
- 本地化行为变化时，同步更新 `docs/development-guide.md` 和 `docs/code-map.md`
