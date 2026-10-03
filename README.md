# 植物大战僵尸 Decomp —— 豆包二次修改版（原生支持中文年度版 PAK）

> 本仓库是 [InLiothixie/stabledecompile](https://github.com/InLiothixie/stabledecompile) 的一个 Fork，由**豆包（Doubao）二次修改**。它在原项目"让 GOTY / OG 都能做 Mod"的基础上，额外实现了**对《植物大战僵尸》中文年度版 `main.pak` 的原生直接支持**。

---

## 本 Fork 相比上游的核心改动

社区早已发现：中文年度版并没有修改 EXE，所有差异都体现在它的 `main.pak` 数据上。为了让这份 Decomp 编译出的 EXE **不需要任何额外工具、开箱即可加载中文年度版 PAK**，豆包做了如下源码级改造：

### 1. 文本编码支持（UTF-16LE / UTF-8 带 BOM）
- 中文版 `properties\LawnStrings.txt`、`ZombatarTOS.txt` 等是 **UTF-16LE 带 BOM**（英文版是窄 ASCII）。
- `Common.cpp/h` 新增 UTF-8 ↔ UTF-16 互转、`Utf8Decode` / `Utf8ToCodePoints`；`TodStringFile.cpp` 在读取时检测并解码 BOM，换行按 Unicode 码点迭代。

### 2. 中文字形加载（ImageFont / SysFont 全链路码点化）
- 中文版字体描述 `data\BrianneTod*.txt` 是 **UTF-8 带 BOM**，内含约 2800+ 中文字符，图集为 `data\_BrianneTod*.png`（下划线前缀）。
- `ImageFont.h/cpp` 中原来写死的 `CharData[256] / mCharMap[256] / mScaledCharImageRects[256]` 全部改为 `std::map<uint32_t, …>`，按 Unicode 码点索引。
- 新增 `TodUtf8SingleChar` 辅助：先反转义（`\' \" \\`），再把字体描述里"用成对引号表示字面引号"（`'' → '`、`"" → "`）的约定折叠为单字符，最后校验为单码点。
- 宽度、绘制、换行、矩阵路径（`Font`、`SDL3Font`、`Graphics.cpp`、`TodCommon.cpp`）全部改为按码点迭代；`SysFont.cpp` 走 UTF-16 GDI 宽字符输出。

### 3. 解析器修复（DescParser / LayerSetExInfo）
- `DescParser.cpp` 剥离行首 UTF-8 BOM（逐字节读取时 BOM 会粘到第一个命令 token 上，导致乱码"Unknown Command"）。
- `ImageFont.cpp` 补上原代码未实现的 **`LayerSetExInfo`** 命令（安全忽略分支），使中文字体描述能完整加载。

### 4. 分辨率强制 4:3（适配中文版 PAK 布局）
- 中文年度版 PAK 仅按 **4:3（800x600）** 设计。原 Fork 默认 `_WIDE_SCREEN`（1066x600 / 1280x720），会让贴图位置和比例错乱。
- `LawnApp.cpp` 关闭宽屏宏，强制 `mWidth/Height = BOARD_WIDTH/HEIGHT = 800x600`，并把 `mWideScreenOffsetX/Y` 置 0，使游戏恢复经典 4:3 布局。
- 开场动画 `assets\videos\intro.mp4` 已用 ffmpeg 中心裁剪为 **960x720（4:3）**，与 4:3 窗口匹配。

### 5. 修改 dependency.pak（让中文文本真正生效）
- 该 Fork 会同时加载 `dependency.pak`（差分补丁），而它内部含一份**英文原始版 `properties\LawnStrings.txt`**，会覆盖中文版文本。
- 已用 [Pistonight/pvz-bintools](https://github.com/Pistonight/pvz-bintools) 解包 `assets\dependency.pak`，**移除其中的英文 `dependency\properties\LawnStrings.txt`** 后重新打包。游戏因此回落到 `main.pak` 里的中文文本。`dependency.pak` 其余 291 个文件保持不变（不可删除，仍承担资源补丁职责）。

> ⚠️ 原始 `main.pak` 从未被修改，测试均使用拷贝到 `build\...\bin` 下的副本。

### 6. 修复开场视频音频流泄漏（解决 ESC 跳过与生存模式崩溃）
- 根因：`LawnApp.cpp` 的 `PlayVideo()` 在播放开场 `intro.mp4` 时用 `SDL_OpenAudioDeviceStream` 创建音频流，但视频结束（无论播完还是按 ESC 跳过）后**从未销毁该流**，导致它一直占用默认音频设备，后续声音资源（如 `sounds/buttonclick`）加载失败。
- 后果：按 ESC 跳过开场动画时报 `Failed to load sound: sounds/buttonclick` 的 FATAL ERROR 后崩溃；同一会话再进生存模式也会因音频子系统异常而访问违规（`0xC0000005`）。
- 修复：在 `PlayVideo()` 的 `SDL_DestroyTexture` 之后补上 `SDL_DestroyAudioStream(audio_playback_stream)`（已在源码中带中文注释标注）。修复后按 ESC 可正常进主菜单，生存模式可正常游玩。

---

## 编译环境与工具链（务必核对版本）

本项目是 **C/C++（MSVC）** 工程，Windows 平台。以下为经过验证可用的环境（豆包本机实测）：

### 操作系统
- Windows 10 / 11，64 位（同时支持 Win32/x86 目标，见下方说明）。

### 编译器 / 工具链
- **Visual Studio 2022 Build Tools**（Community / Build Tools 均可，需包含"使用 C++ 的桌面开发"工作负载）。
  - 实测版本：**MSBuild 17.14.60**，MSVC 工具集 **v143（cl.exe 14.44.35207）**。
  - 下载：Visual Studio Installer → 修改 → 勾选"使用 C++ 的桌面开发"；或直接下载 Build Tools 独立安装器。
- **Windows SDK**（10.x，随 VS 组件自动安装）。
- **注意**：源码含中文字符串与中文注释，且为 **UTF-8 无 BOM**。MSVC 默认按本机代码页（简体中文系统为 GBK/936）读取，会导致**数百个"中文注释/字符串被误解析"的编译错误**。因此工程的所有 `.cpp` 编译选项里**必须带 `/utf-8`**（本项目已在 `SexyAppBase.vcxproj` 中为全部 ClCompile 加好）。

### 第三方运行库（位于根目录 `bin\`）
- `bin\x64\` 与 `bin\x86\` 下的 DLL 会在 PostBuildEvent 时自动复制到输出目录，包括：
  - **SDL3 / SDL3_ttf**（图形与字体）、**FFmpeg**（avcodec/avformat/avutil/swresample/swscale，视频播放）、**Bass**（音频）、**PortAudio**（麦克风）、**tinyfiledialogs64**（原生对话框）。
- 若这些 DLL 损坏或缺失，从本仓库重新下载 `bin\` 覆盖即可。

> ⚠️ **Git LFS（必须）**：本仓库的 `bin\x64\`、`bin\x86\` 下的 DLL 使用 **Git LFS（大文件存储）** 管理（见 `.gitattributes`）。首次克隆前请安装 [Git LFS](https://git-lfs.com/) 并执行：
> ```bat
> git lfs install
> git lfs pull
> ```
> 否则这些 DLL 会以 1KB 的 LFS 指针文件形式出现，编译可能成功但运行时会报"找不到 DLL"。

### PAK 工具（可选，用于研究 / 复现修改）
- [Pistonight/pvz-bintools](https://github.com/Pistonight/pvz-bintools)：
  - 解包：`pakc -u <xxx.pak> <输出目录>`
  - 打包：`pakc -p <xxx.pak> <输入目录>`

---

## 编译步骤

### 1. 准备工作区
1. 拥有一份**正版**《植物大战僵尸》（GOTY 或 2009 原版）。
2. 把游戏目录（含 `PlantsVsZombies.exe`、`properties\` 文件夹、`main.pak` 的那一层）复制到本项目根目录下，文件夹名取 `Plants Vs Zombies`（或 `Plants Vs. Zombies`，构建脚本只认这两个名字）。
   - Steam 版的 PvZ 有启动器，重要文件在子文件夹里，注意找对层级。
3. 确认根目录存在：`assets/`、`bin/`、`Editor/`、`include/`、`lib/`、`PakLib/`、`Plants Vs Zombies/`、`Sexy.TodLib/`、`SexyAppFramework/`、`tools/`。
4. 用 Visual Studio 打开 `PlantsVsZombies.sln`。

### 2. 选择配置与平台
- **开发调试**：用 `Debug`（内含调试工具）。
- **发布**：用 `Release`（优化、无调试工具）。
- 配置与游戏版本的实际对应关系（已在 `SexyAppBase.vcxproj` 中核实，**注意 x64 与 Win32 不同**）：

  | 平台 | Debug / Release（非 GOTY 名） | DebugGOTY / ReleaseGOTY |
  |---|---|---|
  | **Win32**（x86） | 编译为 **OG（2009 原版）** | 编译为 **GOTY（年度版）** |
  | **x64** | 编译为 **OG（2009 原版）** | 编译为 **GOTY（年度版）** |

  > ✅ 本 Fork 已修正上游"x64 配置误带 `_GOTY` 宏"的问题：现在 **Debug/Release（含 x64）= OG**、**DebugGOTY/ReleaseGOTY（含 x64）= GOTY**，配置名与实际版本完全一致（8 个配置已在 `SexyAppBase.vcxproj` 中核实）。
- **玩中文年度版推荐：`ReleaseGOTY` + `x64`**（GOTY 内容依赖 `_GOTY` 宏，且为优化发布版）。中文年度版的成就、生存模式、Last Stand 等内容依赖 GOTY 宏，因此必须用 GOTY 配置（`DebugGOTY` / `ReleaseGOTY`）构建，用非 GOTY 配置会缺失这些内容。
- 平台：`x64`（更优性能）或 `Win32`（兼容旧设备）。

### 3.（可选）修改功能开关
`Lawn/Common/Common Include/GameConstants.h` 中有大量 `#define` 可开关功能（成就、僵尸大头贴、迷你游戏、QoL 等）。去掉/加上 `//` 即可启用/停用对应功能。

### 4. 构建 EXE
- **Visual Studio 方式**：打开工程后按 **F6**（仅构建）或 **Local Windows Debugger**（构建并自动运行）。
- **命令行方式**（豆包本机实测）：
  ```bat
  "D:\VS\BuildTools\MSBuild\Current\Bin\MSBuild.exe" ^
    PlantsVsZombies.sln /p:Configuration=Release /p:Platform=x64 /m /nologo
  ```
- 产物生成在 `build\<配置>_<平台>\bin\`，例如 `build\Release_x64\bin\PlantsVsZombies.exe`。
- PostBuildEvent 会自动：从 `assets\` 复制 `dependency.pak`、`videos\` 等 → 输出目录；从 `%PVZ_DIR%`（你复制的游戏目录）复制 `main.pak` 和 `properties\` → 输出目录；从 `bin\x64` 复制 DLL → 输出目录。
- **改 `assets\` 里的文件会自动同步到输出目录**（但删除 `assets\` 里的文件不会同步删除，需手动清理 `build\...\bin\`）。

### 5. 运行中文年度版
> ⚠️ **重要说明**：本 Fork 的 EXE **原生支持**中文年度版 PAK，但**仓库里不包含任何游戏数据**。`main.pak` 是 PopCap 的受版权保护数据，**必须由用户自己提供**——构建产物默认从你的游戏目录复制的是英文版 `main.pak`，并不会自动变成中文版。

1. 准备一份《植物大战僵尸》中文年度版（2012 年度中文版）的完整游戏数据，取出其中的 `main.pak`。
2. 把中文年度版的 `main.pak` 放到输出目录（覆盖 PostBuildEvent 复制进来的英文版），保持 `dependency.pak`、`videos\intro.mp4` 为本仓库修改后的版本。
3. 直接运行 `PlantsVsZombies.exe` 即可看到完整中文界面。

---

## GitHub Actions 云编译（CI）

本仓库已配置 `.github/workflows/build.yml`，每次 push 到 `master` 或手动触发时，会在 GitHub 的 Windows 云主机上自动执行 **4 个 Release 配置的干净构建**：

- `Release | Win32`（OG / x86）
- `Release | x64`（OG / x64）
- `ReleaseGOTY | Win32`（GOTY / x86）
- `ReleaseGOTY | x64`（GOTY / x64）

CI 流程：`git lfs pull` 拉取 DLL → 安装 VS2022 Build Tools（MSVC v143 + Windows SDK）→ MSBuild 全量构建 → 校验产物（EXE、必要 DLL、`dependency.pak`、`properties`）。

> CI 环境**不包含游戏本体**（`main.pak` 等版权数据不会进入仓库或 CI），因此 PostBuild 已做保护：游戏目录不存在时跳过 `.pak` 复制，编译仍可通过；输出目录里不会出现 `main.pak`，运行仍需按上文第 5 节自行放入中文年度版 `main.pak`。

---

## 编译报错处理（豆包实测踩坑记录）

> 若按上面步骤仍编译报错，优先按下面的"常见错误对照表"排查。

### 常见错误对照表
| 报错形态 | 根因 | 解决办法 |
|---|---|---|
| 大量 `C2001/C2143/C2065` 等，集中在"字符串/注释/中文字符"附近，一次可能上百个 | 源码为 UTF-8 无 BOM，MSVC 按 GBK(936) 读取中文，导致把中文字符串/注释当语法错误 | **确保编译带 `/utf-8`**。在 `.vcxproj` 每个 `<ClCompile>` 的 `AdditionalOptions` 中加入 `/utf-8`（本仓库已加好）。若你新加了 `.cpp`，也要给它加 `/utf-8` |
| `D8016`：`/diagnostics:classic` 与 `/diagnostics:column` 冲突 | 两者不能同时启用 | 二选一，删掉其中一个（本仓库用 `/diagnostics:column`） |
| `C2065`/`未声明的标识符`，集中在新增的 `uint32_t aCp;` / `TodUtf8SingleChar` 处 | 新加的循环变量/辅助函数作用域或头文件缺失 | 确认 `#include <cstdint>`、把 `aCp` 声明在循环内、头文件声明与定义签名一致 |
| 运行时报 `Unknown Command on Line 1`（乱码） | 字体描述 UTF-8 BOM 未剥离 | 已在 `DescParser.cpp` 修复；若改动过该文件请保留 BOM 剥离逻辑 |
| 运行时报 `Missing font 'FONT_BRIANNETOD32'` | 中文字体加载失败（BOM / LayerSetExInfo 未处理） | 保留 `ImageFont.cpp` 的 `LayerSetExInfo` 分支与码点化 map |
| 运行时报 `LayerSetCharWidths Invalid Paramater Type` | 字体字符表里的转义 `\'`、成对引号 `''` 未被折叠为单字符 | 保留 `TodUtf8SingleChar` 的"反转义 + 成对引号折叠 + 单码点校验"逻辑 |
| 运行时 `找不到某 DLL` | `bin\` 的 DLL 缺失/损坏 | 用本仓库 `bin\x64`（或 `bin\x86`）的 DLL 覆盖输出目录 |
| 贴图/按钮位置错乱 | 宽屏（16:9）下渲染中文版 4:3 PAK | 确认 `LawnApp.cpp` 强制了 `800x600` 且 `_WIDE_SCREEN` 已关闭 |

### 通用排查顺序
1. 先 `Rebuild`（不要只 Build，避免用脏缓存）。
2. 看输出日志第一个报错文件，多数问题都在该文件的中文相关行。
3. 若涉及中文乱码/字符串错误 → 检查 `/utf-8`。
4. 若涉及新增的 UTF-8/码点逻辑 → 检查 `Common.cpp`、`ImageFont.cpp`、`TodStringFile.cpp` 是否保持豆包改造后的代码。
5. 运行期崩溃/字体问题 → 先看是否有 `FATAL ERROR` 弹窗标题，对照上表。

---

## 项目功能（译自上游 README）

- [x] 支持 x64 / x86 平台编译
- [x] 可基于 OG（2009）或 GOTY 版本构建
- [x] 用 **SDL3** 替换传统 DirectX 7 图形 API，兼容现代平台
- [x] 通过 **FFmpeg** 播放视频
- [x] 移植成就系统
- [x] 移植僵尸大头贴（Zombatar）
- [x] 通过 **PortAudio** 实现麦克风输入
- [x] 支持多 PAK 文件加载
- [x] 还原并补全被砍掉的内容
- [x] 新增屏幕保护
- [x] 修复原版发行时的若干 Bug 与疏漏
- [x] 新增移动版迷你游戏与"最后一关"内容（`GameConstant.h`）
- [x] 可构建 Bloom / Doom 内容（`GameConstant.h`）
- [x] 新增若干生活质量开关（`GameConstant.h`）

### 规划中功能
- [x] 粒子编辑器 *（进行中）*
- [ ] 主机版迷你游戏与 PvP 内容
- [ ] 字体构建器
- [ ] 分辨率/宽高比切换器（宽屏）
- [ ] 更多设置界面
- [ ] 多语言 Unicode 支持（豆包已为中文打通文本链路的底层）

---

# 免责声明（译自上游 README）

本项目**不纵容盗版**。项目不包含 PopCap 除其开源引擎之外的任何 IP，仅输出一份对 PvZ 进行"逆向重实现"的、同人性质的 EXE。

- 若以 GOTY 配置编译，你需要通过 [Steam](https://store.steampowered.com/app/3590/Plants_vs_Zombies_GOTY_Edition/) 购买正版《植物大战僵尸：年度版》以获取原始游戏文件。
- 若以 2009 配置编译，你需要拥有正版《植物大战僵尸》。

---

# Mod 制作指南（译自上游 README）

## 新增资源
在 `assets/` 内创建 `extension/` 文件夹，包含：
```
  compiled/particles/
  compiled/reanim/
  images/
  sounds/
  particles/
  properties/
  reanim/
```
若新增图片、粒子、文本、声音，需在 `properties/` 内创建 `resources.xml`，格式如下：
```xml
  <?xml version="1.0"?>
  <ResourceManifest>
      <Resources id="LoadingImages">
      <SetDefaults path="extension/images" idprefix="IMAGE_" />
      ... 其余代码
      </Resources>
  </ResourceManifest>
```
`SetDefaults` 的 `path` 必须以 `extension\` 开头才能生效。若新增 Reanim 动画，`.reanim` 放入 `reanim/`、编译产物放入 `compiled/reanim/`；粒子与轨迹同理（共用 `particles/`）。**此规则 x64 与 Win32 均适用，漏了会在运行时报错。**

## 新增资源包（Resource Packs）
若启用资源包（`#define _ALLOW_RESOURCE_PACKS`），可用任意 PAK Mod（如 E-Pea 的 PvZ2Pak）。在输出目录创建 `resourcepack/`，把 PAK Mod 的
  - `compiled/particles/`、`compiled/reanim/`、`images/`、`sounds/`、`particles/`、`properties/`、`reanim/`
粘贴进去。再修改 `resourcepack/properties/resources.xml`，把所有
```xml
<SetDefaults path="xxx" idprefix="..." />
```
改成
```xml
<SetDefaults path="resourcepack/xxx" idprefix="..." />
```

### 资源查找优先级（高 → 低）
```
resourcepack\ （若启用）
extension\
dependency\
--- （以下表示在 main.pak 内或外）
```
找到即用，不再往下找。想替换某个资源（如 `logo.png`）放进 `extension/` 即可。

---

# 开发团队（译自上游 README）

### 主程序
- InLiothixie

### 美术
- Andreko、ReatExists、Nostalgic2137、Fruko、Unnamed

### 特别感谢
- YourLocalMoon、Electr0Gunner、Exter、Adnini、bayant81_0613、CharneleX、Drenco、BoneL

# 致谢（译自上游 README）
- [@patoke](https://github.com/Patoke)：逆向 GOTY 成就、Disco 等。
- [@rspforhp](https://www.github.com/octokatherine)：逆向 0.9.9 版 PvZ。
- [@ruslan831](https://github.com/ruslan831)：归档 [0.9.9 版 PvZ 逆向工程](https://github.com/ruslan831/PlantsVsZombies-decompilation)。
- [@PortAudio](https://github.com/PortAudio/portaudio)：便携音频 I/O 库。
- [@PopLib](https://github.com/teampopwork/PopLib)：SDL3 输入与图形函数。
- [@FFmpeg](https://www.ffmpeg.org/)：音视频库。
- [@Codotaku](https://www.youtube.com/@Codotaku)：SDL3 下 <100 行实现 mp4 播放的启发。
- [@headshot](thub.com/headshot2017)：x64 支持时参考的 define 改动。
- GLFW 团队：出色的窗口库。
- 实现 SDL3 的 PvZ 中文社区 hook/injection 开发者：灵感来源与参考。
- FFmpeg-Build 贡献者（尤其 x86 版）：否则 x86 无法播放视频。
- PopCap：创造伟大的 PvZ 系列并开源引擎。
- 所有为本项目做出贡献的人。

---

> 二次修改说明：本仓库由 **豆包（Doubao）** 基于 [InLiothixie/stabledecompile](https://github.com/InLiothixie/stabledecompile) 二次修改，新增对**中文年度版 PAK** 的原生支持（UTF-16 文本、中文字形、`LayerSetExInfo` 命令、4:3 分辨率、`dependency.pak` 中文文本修复）。上游作者与团队并未参与也不对本 Fork 的改动负责。若需上游功能或支持，请访问上游仓库及其 Discord 社区。
