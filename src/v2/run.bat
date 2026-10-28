@echo off
chcp 936 >nul 2>&1
setlocal enabledelayedexpansion

rem ================================================================
rem  ShapeCalc 一键编译启动脚本
rem
rem  用法：
rem    双击本文件         以 x64 编译并运行
rem    run.bat x86        以 x86（32 位）编译并运行
rem    run.bat build      只编译，不运行
rem    run.bat clean      先清理 build 目录再编译
rem    run.bat -h         查看帮助
rem
rem  依赖：Visual Studio「使用 C++ 的桌面开发」工作负载
rem        EasyX 库已随仓库放在 third_party/easyx，无需另行安装
rem        请保持本文件所在的版本目录结构完整（不要单独移动）
rem ================================================================

title ShapeCalc - 编译启动

set "ROOT=%~dp0"
set "EASYX=%~dp0..\..\third_party\easyx"
set "OUTDIR=build"
set "EXENAME=ShapeCalc.exe"
set "ARCH=x64"
set "DO_CLEAN="
set "BUILDONLY="
set "SRC="
set "VCVARS="
set "VSROOTV="
set "EXINC="
set "EXLIB="

rem ---------------------- 解析命令行参数 ----------------------
:args
if "%~1"=="" goto main
set "ARG=%~1"
if /i "%ARG%"=="x86"    goto arg_x86
if /i "%ARG%"=="/x86"   goto arg_x86
if /i "%ARG%"=="-x86"   goto arg_x86
if /i "%ARG%"=="clean"  goto arg_clean
if /i "%ARG%"=="/clean" goto arg_clean
if /i "%ARG%"=="-clean" goto arg_clean
if /i "%ARG%"=="build"  goto arg_build
if /i "%ARG%"=="/build" goto arg_build
if /i "%ARG%"=="-build" goto arg_build
if /i "%ARG%"=="-h"     goto help
if /i "%ARG%"=="/h"     goto help
if /i "%ARG%"=="-help"  goto help
if /i "%ARG%"=="--help" goto help
if /i "%ARG%"=="/?"     goto help
echo [警告] 未识别的参数：%ARG%，已忽略。
goto arg_next

:arg_x86
set "ARCH=x86"
goto arg_next

:arg_clean
set "DO_CLEAN=1"
goto arg_next

:arg_build
set "BUILDONLY=1"
goto arg_next

:arg_next
shift
goto args

rem -------------------------- 主流程 --------------------------
:main
cd /d "%ROOT%"

echo ================================================================
echo   ShapeCalc          目标平台：%ARCH%
echo   目录：%ROOT%
echo ================================================================
echo.

if defined DO_CLEAN (
    if exist "%OUTDIR%" (
        rmdir /s /q "%OUTDIR%" 2>nul
        echo [清理] 已删除 %OUTDIR% 目录
        echo.
    )
)
if not exist "%OUTDIR%" mkdir "%OUTDIR%"

rem ---------- 1/4 查找源文件 ----------
if not exist "%ROOT%ShapeCalc.cpp" (
    echo [错误] 当前目录下没有找到 ShapeCalc.cpp。
    goto fail
)
set "SRC=ShapeCalc.cpp"
echo [1/4] 源文件：%SRC%

rem ---------- 2/4 查找 Visual Studio 编译环境 ----------
set "VCNAME=vcvars64.bat"
if /i "%ARCH%"=="x86" set "VCNAME=vcvars32.bat"

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        if not defined VCVARS call :tryvc "%%i\VC\Auxiliary\Build\%VCNAME%"
    )
)
if not defined VCVARS (
    for %%r in ("%ProgramFiles%\Microsoft Visual Studio" "%ProgramFiles(x86)%\Microsoft Visual Studio") do (
        for %%v in (18 17 16) do (
            for %%e in (Community Professional Enterprise BuildTools) do (
                if not defined VCVARS call :tryvc "%%~r\%%v\%%e\VC\Auxiliary\Build\%VCNAME%"
            )
        )
    )
)
if not defined VCVARS (
    echo [错误] 未找到可用的 Visual Studio C++ 编译环境。
    echo        请安装 Visual Studio 并勾选「使用 C++ 的桌面开发」工作负载。
    goto fail
)
call "%VCVARS%" >nul 2>&1
echo [2/4] 编译环境：%VCVARS%

rem 兜底：个别环境 vcvars 未能定位 Windows SDK，这里手工补上
set "SDKROOT=%ProgramFiles(x86)%\Windows Kits\10"
if not exist "%SDKROOT%\Include" set "SDKROOT=%ProgramFiles%\Windows Kits\10"
echo %INCLUDE% | find /i "Windows Kits" >nul 2>&1
if errorlevel 1 if exist "%SDKROOT%\Include" (
    set "SDKVER="
    for /f "delims=" %%d in ('dir /b /ad /o-n "%SDKROOT%\Include" 2^>nul') do (
        if not defined SDKVER set "SDKVER=%%d"
    )
    if defined SDKVER (
        set "INCLUDE=!INCLUDE!;%SDKROOT%\Include\!SDKVER!\ucrt;%SDKROOT%\Include\!SDKVER!\shared;%SDKROOT%\Include\!SDKVER!\um"
        set "LIB=!LIB!;%SDKROOT%\Lib\!SDKVER!\ucrt\%ARCH%;%SDKROOT%\Lib\!SDKVER!\um\%ARCH%"
        echo        已补充 Windows SDK 路径：!SDKVER!
    )
)

rem ---------- 3/4 定位 EasyX ----------
if exist "%EASYX%\include\graphics.h" (
    set "EXINC=%EASYX%\include"
    set "EXLIB=%EASYX%\lib\%ARCH%"
)
if not defined EXINC (
    for %%i in ("%VCVARS%") do set "VCDIR=%%~dpi"
    for %%i in ("!VCDIR!..\..\..") do set "VSROOTV=%%~fi"
    if exist "!VSROOTV!\VC\Auxiliary\VS\include\graphics.h" (
        set "EXINC=!VSROOTV!\VC\Auxiliary\VS\include"
        set "EXLIB=!VSROOTV!\VC\Auxiliary\VS\lib\%ARCH%"
    )
)
if not defined EXINC (
    echo [错误] 未找到 EasyX 库（graphics.h）。
    echo        请确认 third_party/easyx 目录完整，或单独安装 EasyX。
    echo        下载地址：https://easyx.cn/download
    goto fail
)
echo [3/4] EasyX 库：%EXLIB%

rem ---------- 4/4 编译链接 ----------
echo [4/4] 正在编译（首次约需几秒）...
echo.
cl /nologo /EHsc /MD /O2 /W3 /I"%EXINC%" "%SRC%" /Fobuild\ShapeCalc.obj /Fe"%OUTDIR%\%EXENAME%" /link /LIBPATH:"%EXLIB%" user32.lib gdi32.lib msimg32.lib winmm.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib
if errorlevel 1 (
    echo.
    echo [失败] 编译或链接未通过，请查看上面的错误信息。
    echo        常见原因：未安装 Windows SDK 组件、EasyX 库与目标平台不匹配。
    goto fail
)

echo.
if defined BUILDONLY (
    echo ================================================================
    echo   编译成功，已跳过运行（build 参数）。
    echo   可执行文件：%OUTDIR%\%EXENAME%
    echo ================================================================
    echo.
    pause
    endlocal
    exit /b 0
)
echo ================================================================
echo   编译成功，正在启动程序……
echo   窗口中点击「退出」按钮即可结束程序。
echo ================================================================
echo.
"%OUTDIR%\%EXENAME%"
set "RC=%ERRORLEVEL%"
if not "%RC%"=="0" (
    echo.
    echo [提示] 程序结束，返回码：%RC%
    pause >nul
)
endlocal
exit /b 0

rem -------------------------- 帮助 --------------------------
:help
echo.
echo   ShapeCalc 编译启动脚本
echo.
echo   用法：run.bat [参数]
echo.
echo     （无参数）   以 x64 编译并运行
echo     x86          以 x86（32 位）编译并运行
echo     build        只编译，不运行
echo     clean        先删除 build 目录再编译
echo     -h           显示本帮助
echo.
echo   说明：
echo     - 编译结果输出到 build 目录，可执行文件为 %EXENAME%
echo     - EasyX 库位于仓库 third_party/easyx，无需单独安装
echo     - 需要 Visual Studio「使用 C++ 的桌面开发」工作负载
echo.
pause
endlocal
exit /b 0

rem ------------------------ 失败退出 ------------------------
:fail
echo.
echo 按任意键关闭窗口……
pause >nul
endlocal
exit /b 1

rem ------------------ 子过程：尝试一个 vcvars ------------------
:tryvc
if defined VCVARS exit /b 0
if not exist "%~1" exit /b 1
call "%~1" >nul 2>&1
where cl >nul 2>&1
if errorlevel 1 exit /b 1
set "VCVARS=%~1"
exit /b 0
