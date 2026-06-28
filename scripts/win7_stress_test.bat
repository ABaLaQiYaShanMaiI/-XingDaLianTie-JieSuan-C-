@echo off
setlocal enabledelayedexpansion

:: =====================================================================
::   Windows 7 压力测试脚本
::   XingDa JieSuan C++ v3.0.0
::
::   持续重复处理测试，用于：
::   - 检测内存泄漏（长时间运行后性能是否下降）
::   - 检测文件句柄泄漏
::   - 检测临时文件未清理
::   - 验证程序稳定性
::
::   用法: 在 cmd 中运行: win7_stress_test.bat [轮数]
::         默认 50 轮，可自定义如: win7_stress_test.bat 200
::
::   前提: 将测试 PDF 放入 test_data\ 目录
:: =====================================================================

set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%.."
cd /d "%PROJECT_DIR%"

:: ---- 配置 -----------------------------------------------------------
set "EXE=%PROJECT_DIR%\build\bin\Release\XingDaLianTieJieSuan.exe"
set "LOG_DIR=%PROJECT_DIR%\test_logs\stress"
set "TEST_DATA_DIR=%PROJECT_DIR%\test_data"
set "REPORT=%LOG_DIR%\_STRESS_REPORT.txt"

:: 轮数 - 默认 50 轮
set "ROUNDS=%~1"
if "%ROUNDS%"=="" set "ROUNDS=50"
if %ROUNDS% lss 1 set "ROUNDS=1"
if %ROUNDS% gtr 10000 set "ROUNDS=10000"

:: 创建日志目录
if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"

:: 写入报告头
echo ================================================================ > "%REPORT%"
echo   XingDa JieSuan C++ Win7 压力测试报告 >> "%REPORT%"
echo   时间: %date% %time% >> "%REPORT%"
echo   目标轮数: %ROUNDS% >> "%REPORT%"
echo ================================================================ >> "%REPORT%"
echo. >> "%REPORT%"

echo ================================================================
echo   XingDa JieSuan - Win7 压力测试
echo   轮数: %ROUNDS%
echo ================================================================
echo.

:: 检查 EXE 是否存在
if not exist "%EXE%" (
    echo [FATAL] 未找到程序文件: %EXE%
    echo [FATAL] EXE 未找到 >> "%REPORT%"
    pause
    exit /b 1
)

:: 找到第一个测试 PDF
set "TEST_PDF="
for /r "%TEST_DATA_DIR%" %%f in (*.pdf) do (
    if "!TEST_PDF!"=="" set "TEST_PDF=%%f"
)

if "%TEST_PDF%"=="" (
    echo ================================================================
    echo   [INFO] test_data\ 中没有 PDF 文件
    echo   将使用 --help 进行压力测试（无 PDF 模式）
    echo ================================================================
    set "TEST_PDF=NONE"
    echo [INFO] 无测试 PDF，使用 --help 模式 >> "%REPORT%"
) else (
    echo   [ OK ] 测试 PDF: %TEST_PDF%
    echo 测试 PDF: %TEST_PDF% >> "%REPORT%"
)

echo.
echo. >> "%REPORT%"

:: ---- 初始化计数器 ---------------------------------------------------
set "PASS_COUNT=0"
set "FAIL_COUNT=0"
set "CRASH_COUNT=0"
set "MEM_LEAK_SUSPECT=0"
set "FIRST_RUNTIME="
set "LAST_RUNTIME="

:: 获取初始系统状态
for /f "tokens=2 delims==" %%m in ('wmic OS get TotalVisibleMemorySize /value 2^>nul') do set "INIT_MEM=%%m"
for /f "tokens=2 delims==" %%f in ('wmic OS get FreePhysicalMemory /value 2^>nul') do set "INIT_FREE=%%f"

echo   [INFO] 初始可用内存: !INIT_FREE! KB / !INIT_MEM! KB
echo 初始可用内存: !INIT_FREE! KB / !INIT_MEM! KB >> "%REPORT%"
echo.

:: =====================================================================
:: 主循环
:: =====================================================================

set "START_TIME=%time%"
for /l %%r in (1,1,%ROUNDS%) do (
    set "ROUND_START=%time%"
    
    if "%TEST_PDF%"=="NONE" (
        cmd /c ""%EXE%" --help > "%LOG_DIR%\rnd_%%r.log" 2>&1"
    ) else (
        :: 交替使用不同的参数组合
        set /a "MODE=%%r %% 4"
        if !MODE! equ 0 (
            cmd /c ""%EXE%" "%TEST_PDF%" --validate-only > "%LOG_DIR%\rnd_%%r.log" 2>&1"
        ) else if !MODE! equ 1 (
            cmd /c ""%EXE%" "%TEST_PDF%" -o "%LOG_DIR%\stress_output" > "%LOG_DIR%\rnd_%%r.log" 2>&1"
        ) else if !MODE! equ 2 (
            cmd /c ""%EXE%" "%TEST_PDF%" --dump-text -o "%LOG_DIR%\stress_output" > "%LOG_DIR%\rnd_%%r.log" 2>&1"
        ) else (
            cmd /c ""%EXE%" "%TEST_PDF%" --no-summary -o "%LOG_DIR%\stress_output" > "%LOG_DIR%\rnd_%%r.log" 2>&1"
        )
    )
    
    set ERR=!errorlevel!
    
    :: 分类结果
    if !ERR! equ 0 (
        set /a PASS_COUNT+=1
        if "!FIRST_RUNTIME!"=="" (
            :: 记录首轮通过时间
            set "FIRST_RUNTIME=!time!"
        )
        set "LAST_RUNTIME=!time!"
    ) else if !ERR! lss 0 (
        set /a CRASH_COUNT+=1
        set /a FAIL_COUNT+=1
        echo   [CRASH] 第 %%r 轮: exit !ERR!
        echo 第 %%r 轮: CRASH (exit !ERR!) >> "%REPORT%"
        findstr /i "error panic fail" "%LOG_DIR%\rnd_%%r.log" >> "%REPORT%" 2>nul
    ) else (
        set /a FAIL_COUNT+=1
        echo   [FAIL]  第 %%r 轮: exit !ERR!
        echo 第 %%r 轮: FAIL (exit !ERR!) >> "%REPORT%"
        findstr /i "error fail" "%LOG_DIR%\rnd_%%r.log" >> "%REPORT%" 2>nul
    )
    
    :: 每 10 轮输出进度
    set /a "PROGRESS=%%r %% 10"
    if !PROGRESS! equ 0 (
        for /f "tokens=2 delims==" %%m in ('wmic OS get FreePhysicalMemory /value 2^>nul') do set "CUR_FREE=%%m"
        echo   [%%r/%ROUNDS%] 通过: !PASS_COUNT! / 失败: !FAIL_COUNT! / 崩溃: !CRASH_COUNT! / 可用内存: !CUR_FREE! KB
    )
    
    :: 内存泄漏检测（每 25 轮比较一次）
    set /a "MEM_CHECK=%%r %% 25"
    if !MEM_CHECK! equ 0 (
        for /f "tokens=2 delims==" %%m in ('wmic OS get FreePhysicalMemory /value 2^>nul') do set "CUR_FREE=%%m"
        
        :: 如果内存持续下降超过 10% 相对于初始值
        if defined INIT_FREE if defined CUR_FREE (
            set /a "MEM_USED_INIT=!INIT_MEM! - !INIT_FREE!"
            set /a "MEM_USED_CUR=!INIT_MEM! - !CUR_FREE!"
            set /a "MEM_DIFF=!MEM_USED_CUR! - !MEM_USED_INIT!"
            if !MEM_DIFF! gtr 100000 (
                set /a MEM_LEAK_SUSPECT+=1
                echo   [WARN] 第 %%r 轮: 可能的内存泄漏 (已用增加 !MEM_DIFF! KB^)
                echo 第 %%r 轮: 疑似内存泄漏 (已用内存增加 !MEM_DIFF! KB^) >> "%REPORT%"
            )
        )
    )
)

set "END_TIME=%time%"

:: =====================================================================
:: 最终系统状态
:: =====================================================================
echo.
echo ================================================================
echo   最终系统状态
echo ================================================================

for /f "tokens=2 delims==" %%m in ('wmic OS get FreePhysicalMemory /value 2^>nul') do set "FINAL_FREE=%%m"
echo   最终可用内存: !FINAL_FREE! KB
echo 最终可用内存: !FINAL_FREE! KB >> "%REPORT%"

:: 检查临时文件残留
echo.
echo   临时文件检查...
set "TEMP_LEAK=0"
for /r "%LOG_DIR%" %%f in (*.png *.ppm) do set /a TEMP_LEAK+=1
if !TEMP_LEAK! gtr 0 (
    echo   [WARN] 发现 !TEMP_LEAK! 个 OCR 临时文件残留
    echo 临时文件残留: !TEMP_LEAK! 个 >> "%REPORT%"
) else (
    echo   [ OK ] 无临时文件残留
    echo 无临时文件残留 >> "%REPORT%"
)

:: 检查输出文件
set "XLSX_COUNT=0"
for /r "%LOG_DIR%" %%f in (*.xlsx) do set /a XLSX_COUNT+=1

echo.
echo ╔══════════════════════════════════════════════════════════════╗
echo ║        Win7 压 力 测 试 完 成                                 ║
echo ╚══════════════════════════════════════════════════════════════╝
echo.
echo ═══════════════════════════════════════════════════════════════
echo   压 力 测 试 汇 总
echo ═══════════════════════════════════════════════════════════════
echo   总轮数:     %ROUNDS%
echo   通过:       %PASS_COUNT%
echo   失败:       %FAIL_COUNT% (含 %CRASH_COUNT% 次崩溃)
echo   Excel:      %XLSX_COUNT% 个
echo   疑似内存泄漏: %MEM_LEAK_SUSPECT% 次
echo   临时文件残留: %TEMP_LEAK% 个
echo.
echo   开始时间: %START_TIME%
echo   结束时间: %END_TIME%
echo ═══════════════════════════════════════════════════════════════
echo.

:: 写入最终汇总
echo. >> "%REPORT%"
echo ================================================================ >> "%REPORT%"
echo   压力测试汇总 >> "%REPORT%"
echo ================================================================ >> "%REPORT%"
echo   总轮数:     %ROUNDS% >> "%REPORT%"
echo   通过:       %PASS_COUNT% >> "%REPORT%"
echo   失败:       %FAIL_COUNT% (含 %CRASH_COUNT% 次崩溃) >> "%REPORT%"
echo   Excel:      %XLSX_COUNT% >> "%REPORT%"
echo   疑似内存泄漏: %MEM_LEAK_SUSPECT% >> "%REPORT%"
echo   临时文件残留: %TEMP_LEAK% >> "%REPORT%"
echo   开始时间: %START_TIME% >> "%REPORT%"
echo   结束时间: %END_TIME% >> "%REPORT%"

:: 稳定性评估
echo. >> "%REPORT%"
echo ================================================================ >> "%REPORT%"
echo   稳定性评估 >> "%REPORT%"
echo ================================================================ >> "%REPORT%"

set /a "TOTAL_FAIL=PASS_COUNT + FAIL_COUNT"
if !TOTAL_FAIL! equ 0 set "TOTAL_FAIL=1"
set /a "PASS_RATE=PASS_COUNT * 100 / TOTAL_FAIL"

if !CRASH_COUNT! gtr 0 (
    echo   评估: 不稳定 - 存在崩溃 >> "%REPORT%"
    echo   建议: 检查崩溃日志 test_logs\stress\rnd_*.log >> "%REPORT%"
) else if !MEM_LEAK_SUSPECT! gtr 2 (
    echo   评估: 疑似内存泄漏 >> "%REPORT%"
    echo   建议: 使用任务管理器长时间监控内存 >> "%REPORT%"
) else if !PASS_RATE! lss 95 (
    echo   评估: 通过率低 (!PASS_RATE!%%^)  >> "%REPORT%"
    echo   建议: 检查失败日志 >> "%REPORT%"
) else (
    echo   评估: 稳定 ✓ （!PASS_RATE!%% 通过率） >> "%REPORT%"
)

:: 失败日志存档
if !FAIL_COUNT! gtr 0 (
    echo. >> "%REPORT%"
    echo ================================================================ >> "%REPORT%"
    echo   失败轮次日志 >> "%REPORT%"
    echo ================================================================ >> "%REPORT%"
    for /l %%r in (1,1,%ROUNDS%) do (
        findstr /c:"[FAIL" /c:"[CRASH" "%LOG_DIR%\rnd_%%r.log" >nul 2>&1
        if not errorlevel 1 (
            echo --- 第 %%r 轮 --- >> "%REPORT%"
            type "%LOG_DIR%\rnd_%%r.log" >> "%REPORT%" 2>nul
            echo. >> "%REPORT%"
        )
    )
)

echo. >> "%REPORT%"
echo ================================================================ >> "%REPORT%"
echo   报告结束 >> "%REPORT%"
echo ================================================================ >> "%REPORT%"

echo   详细日志: %LOG_DIR%\
echo   汇总报告: %REPORT%
echo.
echo   按任意键退出...
pause >nul
exit /b 0