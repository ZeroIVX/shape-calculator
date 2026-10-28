# 05 · 构建与运行

## 环境要求

| 项目 | 最低要求 | 说明 |
|---|---|---|
| 操作系统 | Windows 10 | 需支持 `DwmSetWindowAttribute`（v2 用于浅色标题栏） |
| Visual Studio | 2019 / 2022 / 2026 | 必须勾选「使用 C++ 的桌面开发」工作负载 |
| MSVC 工具集 | 14.x | 脚本通过 `vcvars64.bat` / `vcvars32.bat` 加载 |
| Windows SDK | 10.x | 随工作负载安装；缺失时脚本尝试自动补齐路径 |
| EasyX | 已内置 | 位于 `third_party/easyx`，无需安装到系统 |

脚本按以下顺序探测编译器：

1. `vswhere.exe -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64`
2. 依次枚举 `Microsoft Visual Studio\{18,17,16}\{Community,Professional,Enterprise,BuildTools}`
3. 逐个 `call vcvars*.bat` 并用 `where cl` 验证，第一个成功的即采用

## 一键脚本

两个版本目录下各有一份内容相同的 `run.bat`。

| 命令 | 作用 |
|---|---|
| `run.bat` | 以 x64 编译并运行 |
| `run.bat x86` | 以 x86（32 位）编译 |
| `run.bat build` | 只编译，不运行（适合 CI 或快速语法检查） |
| `run.bat clean` | 删除 `build` 目录后重新编译 |
| `run.bat -h` | 显示帮助 |

参数可组合，例如 `run.bat x86 clean build`。

产物固定输出到版本目录下的 `build\ShapeCalc.exe`，中间文件 `ShapeCalc.obj` 同目录。`build/` 已在 `.gitignore` 中排除。

> 脚本通过 `%~dp0..\..\third_party\easyx` 定位图形库，因此**必须保持仓库目录结构完整**。若要把单个版本目录拷走独立使用，需同时复制 `third_party/easyx` 到版本目录下并改用本地路径。

## 手动编译

在「Developer Command Prompt for VS」中执行：

```bat
cd src\v2

cl /nologo /EHsc /MD /O2 /W3 ^
   /I"..\..\third_party\easyx\include" ^
   ShapeCalc.cpp ^
   /Fobuild\ShapeCalc.obj ^
   /Fe"build\ShapeCalc.exe" ^
   /link ^
   /LIBPATH:"..\..\third_party\easyx\lib\x64" ^
   user32.lib gdi32.lib msimg32.lib winmm.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib
```

### 参数说明

| 参数 | 作用 |
|---|---|
| `/EHsc` | 启用标准 C++ 异常模型 |
| `/MD` | 动态链接 CRT，与 EasyX 预编译库的运行时一致（**不可改为 `/MT`**） |
| `/O2` | 最大速度优化 |
| `/W3` | 警告等级 3，v2 在此级别下零告警 |
| `/I` `/LIBPATH` | 指向 EasyX 头文件与库目录 |
| 尾部系统库 | 命令行编译不会自动链接这些库，必须显式给出，否则出现 `__imp_*` 未解析符号（详见 [06](06-engineering-notes.md)） |

**不要定义 `UNICODE` / `_UNICODE`**。源码中的中文字面量直接传给 `drawtext`、`SetWindowText` 等窄字符接口，若按 Unicode 编译会报 `C2664`（`const char[]` 无法转换为 `LPCWSTR`）。

## 常见问题

### 1. `LNK1104: 无法打开文件 "build\ShapeCalc.exe"`

上一次运行的程序未退出，可执行文件被占用。关闭窗口后重试，或用任务管理器结束 `ShapeCalc.exe`，也可执行 `run.bat clean`。

### 2. `C1083: 无法打开包括文件 "stdio.h"`

`vcvars` 未能定位 Windows SDK。脚本内置兜底逻辑会扫描 `%ProgramFiles(x86)%\Windows Kits\10\Include` 下版本号最大的目录并补充 `INCLUDE` / `LIB`；若仍失败，说明 SDK 组件未安装，需在 Visual Studio Installer 中勾选「Windows 10/11 SDK」。

### 3. 界面中文显示为乱码

源码为 GBK 编码，需在非 936 代码页环境下正确编译。`run.bat` 首行执行 `chcp 936`；手动编译时若控制台代码页不同，会沿用旧编码解析源文件导致乱码。可在编译前执行 `chcp 936`，或在 VS Code 中确认文件以 GBK 打开。

### 4. 字体显示为宋体点阵、笔画发虚

v2 已固定使用英文族名 `Microsoft YaHei` + `CLEARTYPE_QUALITY`。若被改回中文族名 `微软雅黑`，GDI 会静默回退到宋体点阵——定位过程见 [06](06-engineering-notes.md)。

### 5. `cl` 不是内部或外部命令

`vcvars*.bat` 未被成功调用。请通过 `run.bat` 启动（脚本内部会调用），或在手动编译前先执行 `call "...\VC\Auxiliary\Build\vcvars64.bat"`。

## 目录与产物

```
src/v2/
├─ ShapeCalc.cpp       源码
├─ run.bat             构建脚本
├─ README.md
└─ build/              构建产物（已忽略）
   ├─ ShapeCalc.exe
   └─ ShapeCalc.obj
```

---

<sub>Zero · 2026</sub>
