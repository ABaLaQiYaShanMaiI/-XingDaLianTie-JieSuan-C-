@echo off
setlocal enabledelayedexpansion

:: =====================================================================
::   兴达炼铁结算单 - 外部工具自动检测 & 配置脚本
::   XingDa JieSuan C++ v3.0.0
::
::   自动检测系统已安装的 pdftotext (Poppler) 和
::   tesseract-OCR，复制到 tools\ 便携目录。
::
::   使程序可以在任何 Windows 机器上运行，无需
::   用户手动安装外部工具到系统 PATH。
::
::   用法: 双击运行
:: =====================================================================

set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%.."
set "TOOLS_DIR=%PROJECT_DIR%\tools"

cd /d "%PROJECT_DIR%"

echo ================================================================
echo   兴达炼铁结算单 - 外部工具自动配置
echo   XingDa JieSuan C++ v3.0.0
echo ================================================================
echo.
echo   项目目录: %PROJECT_DIR%
echo   工具目录: %TOOLS_DIR%
echo.

:: 创建 tools 目录
if not exist "%TOOLS_DIR%" mkdir "%TOOLS_DIR%"

set "LOG_FILE=%TOOLS_DIR%\setup_log.txt"
echo 兴达炼铁结算单 - 工具配置日志 > "%LOG_FILE%"
echo 时间: %date% %time% >> "%LOG_FILE%"
echo ================================================================ >> "%LOG_FILE%"
echo. >> "%LOG_FILE%"

:: =====================================================================
:: 1. 检测 pdftotext (Poppler)
:: =====================================================================
echo --- 1. pdftotext (Poppler) ---
echo --- 1. pdftotext (Poppler) --- >> "%LOG_FILE%"

set "PDFTOTEXT_FOUND=0"

:: 检查项目 tools/ 目录是否已有
if exist "%TOOLS_DIR%\pdftotext.exe" (
    echo   [ OK ] tools\pdftotext.exe 已存在
    echo   [ OK ] tools\pdftotext.exe 已存在 >> "%LOG_FILE%"
    set "PDFTOTEXT_FOUND=1"
    goto :check_tesseract
)

:: 搜索常见安装位置
for %%d in (
    "C:\Program Files\poppler\bin"
    "C:\Program Files (x86)\poppler\bin"
    "C:\poppler\bin"
    "%USERPROFILE%\Downloads\poppler*\bin"
) do (
    if exist %%d\pdftotext.exe (
        echo   找到: %%d\pdftotext.exe
        echo   找到: %%d\pdftotext.exe >> "%LOG_FILE%"
        copy /y %%d\pdftotext.exe "%TOOLS_DIR%\" >nul 2>&1
        if exist %%d\pdftoppm.exe (
            copy /y %%d\pdftoppm.exe "%TOOLS_DIR%\" >nul 2>&1
            echo   复制: pdftoppm.exe
            echo   复制: pdftoppm.exe >> "%LOG_FILE%"
        )
        echo   [ OK ] pdftotext.exe 已复制到 tools\
        echo   [ OK ] pdftotext.exe 已复制到 tools\ >> "%LOG_FILE%"
        set "PDFTOTEXT_FOUND=1"
        goto :check_tesseract
    )
)

:: 检查系统 PATH
where pdftotext >nul 2>&1
if !errorlevel! equ 0 (
    for /f "delims=" %%i in ('where pdftotext') do (
        echo   找到（PATH）: %%i
        echo   找到（PATH）: %%i >> "%LOG_FILE%"
        copy /y "%%i" "%TOOLS_DIR%\" >nul 2>&1
    )
    echo   [ OK ] pdftotext.exe 已从 PATH 复制到 tools\
    echo   [ OK ] pdftotext.exe 已从 PATH 复制到 tools\ >> "%LOG_FILE%"
    set "PDFTOTEXT_FOUND=1"
    goto :check_tesseract
)

echo   [WARN] 未找到 pdftotext.exe
echo   [WARN] 未找到 pdftotext.exe >> "%LOG_FILE%"

:check_tesseract

:: =====================================================================
:: 2. 检测 tesseract-OCR
:: =====================================================================
echo.
echo --- 2. Tesseract-OCR ---
echo. >> "%LOG_FILE%"
echo --- 2. Tesseract-OCR --- >> "%LOG_FILE%"

set "TESSERACT_FOUND=0"

:: 检查是否已有便携版
if exist "%TOOLS_DIR%\tesseract\tesseract.exe" (
    echo   [ OK ] tools\tesseract\tesseract.exe 已存在
    echo   [ OK ] tools\tesseract\tesseract.exe 已存在 >> "%LOG_FILE%"
    set "TESSERACT_FOUND=1"
    goto :summary
)

:: 搜索常见安装位置
for %%d in (
    "C:\Program Files\Tesseract-OCR"
    "C:\Program Files (x86)\Tesseract-OCR"
    "%LOCALAPPDATA%\Tesseract-OCR"
    "%LOCALAPPDATA%\Programs\Tesseract-OCR"
) do (
    if exist "%%d\tesseract.exe" (
        echo   找到: %%d\tesseract.exe
        echo   找到: %%d\tesseract.exe >> "%LOG_FILE%"
        
        set "TESS_DIR=%TOOLS_DIR%\tesseract"
        if not exist "!TESS_DIR!" mkdir "!TESS_DIR!"
        
        :: 复制 exe
        copy /y "%%d\tesseract.exe" "!TESS_DIR!\" >nul 2>&1
        echo     复制: tesseract.exe
        echo     复制: tesseract.exe >> "%LOG_FILE%"
        
        :: 复制所有 dll
        for %%f in ("%%d\*.dll") do (
            copy /y "%%f" "!TESS_DIR!\" >nul 2>&1
            echo     复制: %%~nxf
            echo     复制: %%~nxf >> "%LOG_FILE%"
        )
        
        :: 复制 tessdata 语言包
        if exist "%%d\tessdata" (
            if not exist "!TESS_DIR!\tessdata" mkdir "!TESS_DIR!\tessdata"
            xcopy /e /y "%%d\tessdata\*" "!TESS_DIR!\tessdata\" >nul 2>&1
            for %%t in ("!TESS_DIR!\tessdata\*.traineddata") do (
                echo     语言包: %%~nxt
                echo     语言包: %%~nxt >> "%LOG_FILE%"
            )
        )
        
        set "TESSERACT_FOUND=1"
        goto :summary
    )
)

:: 检查系统 PATH
where tesseract >nul 2>&1
if !errorlevel! equ 0 (
    for /f "delims=" %%i in ('where tesseract') do (
        echo   找到（PATH）: %%i
        set "TESS_ROOT=%%~dpi"
        set "TESS_ROOT=!TESS_ROOT:~0,-1!"
        
        set "TESS_DIR=%TOOLS_DIR%\tesseract"
        if not exist "!TESS_DIR!" mkdir "!TESS_DIR!"
        
        copy /y "%%i" "!TESS_DIR!\" >nul 2>&1
        echo     复制: tesseract.exe
        
        :: 尝试从安装目录复制 dll 和 tessdata
        if exist "!TESS_ROOT!\*.dll" (
            for %%f in ("!TESS_ROOT!\*.dll") do (
                copy /y "%%f" "!TESS_DIR!\" >nul 2>&1
            )
            echo     复制: *.dll
        )
        if exist "!TESS_ROOT!\tessdata" (
            if not exist "!TESS_DIR!\tessdata" mkdir "!TESS_DIR!\tessdata"
            xcopy /e /y "!TESS_ROOT!\tessdata\*" "!TESS_DIR!\tessdata\" >nul 2>&1
            echo     复制: tessdata\
        )
        
        echo   [ OK ] Tesseract 已从 PATH 复制到 tools\
        echo   [ OK ] Tesseract 已从 PATH 复制到 tools\ >> "%LOG_FILE%"
        set "TESSERACT_FOUND=1"
        goto :summary
    )
)

echo   [WARN] 未找到 tesseract.exe
echo   [WARN] 未找到 tesseract.exe >> "%LOG_FILE%"

:summary

:: =====================================================================
:: 汇总报告
:: =====================================================================
echo.
echo ================================================================
echo   配置汇总
echo ================================================================
echo.
echo. >> "%LOG_FILE%"
echo ================================================================ >> "%LOG_FILE%"
echo   配置汇总 >> "%LOG_FILE%"
echo ================================================================ >> "%LOG_FILE%"
echo. >> "%LOG_FILE%"

:: pdftotext 状态
if %PDFTOTEXT_FOUND% equ 1 (
    echo   [ OK ] pdftotext 已配置 - PDF 文本提取可用
    echo   [ OK ] pdftotext 已配置 >> "%LOG_FILE%"
) else (
    echo   [ !! ] pdftotext 未配置 - PDF 核心功能将不可用！
    echo   [ !! ] pdftotext 未配置 >> "%LOG_FILE%"
    echo.
    echo   pdftotext 是处理 PDF 的必要工具。
    echo   请下载 Poppler for Windows:
    echo     https://github.com/oschwartz10612/poppler-windows/releases/
    echo   解压后将 bin\pdftotext.exe 复制到 tools\ 目录即可。
)

echo.
echo. >> "%LOG_FILE%"

:: Tesseract 状态
if %TESSERACT_FOUND% equ 1 (
    echo   [ OK ] Tesseract 已配置 - OCR 功能可用
    echo   [ OK ] Tesseract 已配置 >> "%LOG_FILE%"
) else (
    echo   [ -- ] Tesseract 未配置 - OCR 功能不可用（不影响核心功能）
    echo   [ -- ] Tesseract 未配置 >> "%LOG_FILE%"
    echo.
    echo   如需处理扫描版/图片型 PDF, 请安装 Tesseract-OCR:
    echo     https://github.com/UB-Mannheim/tesseract/wiki
    echo   安装后重新运行本脚本即可自动配置。
)

echo.
echo ================================================================
echo   tools\ 目录内容:
echo ================================================================
dir /b /s "%TOOLS_DIR%" 2>nul
echo.
echo   日志: %LOG_FILE%
echo.

if %PDFTOTEXT_FOUND% equ 0 (
    echo ╔════════════════════════════════════════════════════════════╗
    echo ║  [!] 警告：pdftotext 未配置！                          ║
    echo ║                                                          ║
    echo ║  PDF 文本提取是核心功能，没有 pdftotext 程序将无法运行。 ║
    echo ║                                                          ║
    echo ║  下载地址（选择 poppler-xx.x.x.zip）:                    ║
    echo ║  https://github.com/oschwartz10612/poppler-windows/      ║
    echo ║                     releases/latest                      ║
    echo ║                                                          ║
    echo ║  下载后解压，将 bin\pdftotext.exe 放入:                  ║
    echo ║  %TOOLS_DIR%\                                            ║
    echo ║                                                          ║
    echo ║  然后重新运行本脚本验证。                                 ║
    echo ╚════════════════════════════════════════════════════════════╝
)

echo.
echo   按任意键退出...
pause >nul
exit /b 0