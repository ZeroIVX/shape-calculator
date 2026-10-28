# 06 · 工程笔记

记录开发过程中遇到并解决的问题，每个条目都是「现象 → 定位 → 根因 → 处理」的完整链条，供后续维护者参考，避免重复踩坑。

---

## 1. 界面文字笔画发虚、字形像宋体

**现象**：v2 首版渲染出的中文边缘为纯色硬边、无灰阶过渡，小字号笔画粘连。代码里明确写的是 `微软雅黑` + `ANTIALIASED_QUALITY`，看起来抗锯齿没生效。

**定位**：写了一个对照程序，同一句话用 12 组不同的 `LOGFONT` 组合各渲染一行，截图后放大 4 倍逐行比对。

| 组 | 字体名 | 质量 | 高度 | 字符集 | 实际渲染 |
|---|---|---|---|---|---|
| A | `微软雅黑` | ANTIALIASED | −26 | 0 | 宋体点阵 |
| B | `微软雅黑` | ANTIALIASED | −26 | DEFAULT | 宋体点阵 |
| C | `微软雅黑` | ANTIALIASED | +26 | DEFAULT | 宋体点阵 |
| D | `Microsoft YaHei` | CLEARTYPE | −26 | DEFAULT | **微软雅黑，边缘平滑** |
| E | `宋体` | ANTIALIASED | −26 | DEFAULT | 宋体 |
| F | `微软雅黑` | PROOF | −26 | GB2312 | 宋体点阵 |
| G | `settextstyle(24,0,"微软雅黑")` | — | — | — | 宋体点阵 |
| H | `settextstyle(-24,0,"微软雅黑")` | — | — | — | 宋体点阵 |

**根因**：GDI 用**中文族名在 `lfFaceName` 中匹配字体失败后会静默回退**到系统默认的宋体点阵，不报错、不抛异常。所有使用中文族名的组合都走了回退路径，只有英文族名真正命中了雅黑字体文件。

**处理**：

```cpp
LOGFONT f;
memset(&f, 0, sizeof(f));                              // 整体清零，避免继承脏字段
_tcscpy_s(f.lfFaceName, _T("Microsoft YaHei"));        // 必须用英文族名
f.lfHeight  = -px;
f.lfWidth   = 0;
f.lfWeight  = bold ? FW_BOLD : FW_NORMAL;
f.lfCharSet = DEFAULT_CHARSET;
f.lfQuality = CLEARTYPE_QUALITY;
settextstyle(&f);
```

**连带问题**：`CLEARTYPE_QUALITY` 编译报「未声明的标识符」。原因是 EasyX 头文件在未定义 `WINVER` 时默认 `#define WINVER 0x0400`，`wingdi.h` 据此裁剪了该常量。解决方式是在 `#include <graphics.h>` **之前**抬高目标平台版本：

```cpp
#ifndef WINVER
#define WINVER 0x0601
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
```

---

## 2. 命令行编译 EasyX 程序出现大量未解析符号

**现象**：

```
error LNK2019: 无法解析的外部符号 __imp_SetWindowTextA
error LNK2019: 无法解析的外部符号 __imp_TranslateMessage
error LNK2019: 无法解析的外部符号 __imp_CreateWindowExA
```

**根因**：`cl` 命令行**不会自动链接 GUI 相关系统库**。`user32.lib`、`gdi32.lib` 等默认依赖只存在于 Visual Studio 项目模板的「附加依赖项」里，不在编译器内建行为中。

**处理**：在 `/link` 之后显式列出：

```bat
/link /LIBPATH:"...\easyx\lib\x64" ^
user32.lib gdi32.lib msimg32.lib winmm.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib
```

`msimg32.lib` 容易被漏掉，EasyX 的渐变与位图混合需要它。

---

## 3. 按钮底色漏成投影灰

**现象**：按钮本应是白色卡片，实际整块显示为阴影色 `RGB(214,222,236)`。

**定位**：采样按钮中心像素，颜色与投影色完全一致，说明本体没有被填充。

**根因**：`EasyX` 的命名规则与直觉不同：

| 函数 | 行为 |
|---|---|
| `rectangle` / `roundrect` / `circle` | **只描边，不填充** |
| `fillrectangle` / `fillroundrect` | 填充 + 描边 |
| `solidrectangle` / `solidroundrect` | 填充，无描边 |

代码用了 `roundrect`，只画了边框，先画好的投影从内部透出。

**处理**：改用 `fillroundrect`。这与头文件注释中的 `Draw a filled rectangle with a border` 一致，但函数名本身不带 `fill` 前缀时容易误判。

---

## 4. 三角形非法输入显示 `-nan`

**现象**：输入三边 1、2、5（不满足三角形不等式），结果显示 `-nan` 而非错误提示。

**根因**：海伦公式对不合法三边会得到 `p-a < 0`，开方结果为 `NaN`。原代码用

```cpp
if (s < 0) { /* 显示 Error */ }
```

拦截，而 **IEEE 754 下任何与 `NaN` 的比较都返回 `false`**，`NaN < 0` 恒假，错误分支永远不执行。

**处理**：把判断提前到开方之前，直接校验三边关系：

```cpp
if (a + b <= c || a + c <= b || b + c <= a) return false;
```

同理修正了菱形面积公式（原实现按边长乘积计算，正确应为对角线乘积的一半）。

---

## 5. 在未安装 EasyX 的环境里构建

**现象**：目标机器装了 Visual Studio，但 `VC\Auxiliary\VS\include` 下没有 `graphics.h`，直接编译失败。EasyX 的官方分发是安装器，且需要图形界面交互。

**定位与处理**：EasyX 官方文档提到「所有版本的安装包都可以把 `.exe` 后缀改成 `.7z` 直接解压」。据此分析安装包结构：文件 365 KB，PE 节区仅有 4 个、最大原始大小 128 KB，合计约 169 KB，说明尾部存在约 196 KB 的 overlay。在该区域搜索 7z 签名 `37 7A BC AF 27 1C`，定位到偏移 `169568`，从该处截取即得到完整的 7z 数据，用系统自带的 `bsdtar`（`C:\Windows\System32\tar.exe`）解压：

```bat
tar -xf easyx.7z -C third_party/easyx
```

得到 `include/{easyx.h, graphics.h}` 与 `lib/{VC6, VC2008, VC2015}/{x64, x86}`。仓库只保留 `VC2015` 一套（兼容 VS2015 及以后的所有版本），按架构分目录存放。

**收益**：图形库随仓库分发，克隆即可编译，不写系统目录、不需要管理员权限，也便于锁定版本。

---

## 6. 按 Unicode 编译会失败

**现象**：定义 `UNICODE` / `_UNICODE` 后编译报 `C2664`：

```
无法将参数 2 从 "const char [11]" 转换为 "LPCWSTR"
```

**根因**：源码中的中文字面量直接传给 `drawtext`、`SetWindowText` 等 TCHAR 接口，字面量是窄字符串 `const char[]`，Unicode 编译下 `TCHAR` 为 `wchar_t`，类型不匹配。

**处理**：不定义 `UNICODE`，按多字节字符集编译。`easyx.h` 据此自动 `#pragma comment(lib, "EasyXa.lib")`（ANSI 版），与构建脚本的 `/LIBPATH` 指向一致。若改用 Unicode 版本，需把所有字面量改为 `_T()` 包裹并链接 `EasyXw.lib`——这属于破坏性改动，未采用。

---

## 7. `vcvars` 加载成功但缺少 SDK 路径

**现象**：构建脚本调用 `vcvars64.bat` 后 `where cl` 能找到编译器，但编译时报 `C1083: 无法打开包括文件 "stdio.h"`。

**根因**：`vcvars` 依赖注册表查询来确定 Windows SDK 版本。在受限环境（注册表访问被拦截、或以非交互方式调用）下，查询失败会导致 `vcvars` 提前返回，`INCLUDE` / `LIB` 中没有 SDK 目录，而 `PATH` 中的 `cl` 仍然可用——于是出现「编译器在、头文件不在」的组合。

**处理**：脚本在 `vcvars` 之后做一次检测与兜底：

```bat
echo %INCLUDE% | find /i "Windows Kits" >nul 2>&1
if errorlevel 1 if exist "%SDKROOT%\Include" (
    rem 取版本号最大的 SDK 目录，补齐 INCLUDE 与 LIB
)
```

正常情况下检测通过、不产生任何副作用；异常环境下自动修复路径，避免把问题推给使用者。

---

## 8. 键盘事件被处理两次

**现象**：按一次回车触发两次计算，`Tab` 跳两个字段。

**根因**：`Enter` 会同时产生 `WM_KEYDOWN`（`VK_RETURN`）与 `WM_CHAR`（字符 `\r`），`Tab` 同理产生 `\t`。两个分支各自响应导致重复执行。

**处理**：职责分离——`WM_CHAR` 只处理可见字符与退格，并在分支内显式过滤 `13` 与 `9`；`WM_KEYDOWN` 只处理功能键。两者不交叉。

---

## 附：EasyX 关键接口速查

| 接口 | 说明 |
|---|---|
| `peekmessage(&msg, EX_MOUSE \| EX_KEY \| EX_CHAR)` | 非阻塞取消息，filter 常量是 `EX_*`（值 1/2/4/8），不是 `EM_*` |
| `msg.x` / `msg.y` / `msg.vkcode` / `msg.ch` | `ExMessage` 用匿名 union，可直接访问成员 |
| `BeginBatchDraw` / `FlushBatchDraw` | 双缓冲开关与刷新 |
| `fillroundrect` / `solidroundrect` | 填充 + 描边 / 仅填充（圆角矩形） |
| `settextstyle(const LOGFONT*)` | 按 `LOGFONT` 设置字体，`lfHeight` 负值表示字符高度 |
| `fillpolygon(const POINT*, int)` | 填充多边形，用于手绘图形闭合后的着色 |

---

<sub>Zero · 2026</sub>
