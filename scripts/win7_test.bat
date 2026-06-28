@echo off
setlocal enabledelayedexpansion

:: =====================================================================
::   Windows 7 兼容性测试脚本
::   XingDa JieSuan C++ v3.0.0
::
::   对 test_data\ 下的所有 PDF 执行多种参数组合测试，
::   每条测试生成独立日志，最终生成汇总报告。
::
::   用法: 双击运行 或 cmd: win7_test.bat
::   前提: 将测试 PDF 放入 test_data\ 目录
:: =====================================================================

set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%.."
cd /d "%PROJECT_DIR%"

:: ---- 配置 -----------------------------------------------------------
set "EXE=%PROJECT_DIR%\build\bin\Release\XingDaLianTieJieSuan.exe"
set "LOG_DIR=%PROJECT_DIR%\test_logs\tests"
set "TEST_DATA_DIR=%PROJECT_DIR%\test_data"
set "REPORT=%LOG_DIR%\_TEST_REPORT.txt"
set "TEST_COUNT=0"
set "PASS_COUNT=0"
set "FAIL_COUNT=0"
set "CRASH_COUNT=0"
set "SKIP_COUNT=0"

:: 创建日志目录
if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"

:: 写入报告头
echo ================================================================ > "%REPORT%"
echo   XingDa JieSuan C++ Win7 兼容性测试报告 >> "%REPORT%"
echo   时间: %date% %time% >> "%REPORT%"
echo ================================================================ >> "%REPORT%"
echo. >> "%REPORT%"

echo ================================================================
echo   XingDa JieSuan - Win7 兼容性测试
echo   Mode: 穷举所有 PDF x 参数组合
echo ================================================================
echo.

:: 检查 EXE 是否存在
if not exist "%EXE%" (
    echo [FATAL] 未找到程序文件
    echo        路径: %EXE%
    echo.
    echo   请确保已构建项目，或将 EXE 路径修改为正确的构建输出位置。
    echo.
    echo [FATAL] EXE 未找到: %EXE% >> "%REPORT%"
    pause
    exit /b 1
)

echo   [ OK ] 程序: %EXE%
echo. >> "%REPORT%"
echo 程序: %EXE% >> "%REPORT%"

:: ---- 统计 PDF 数量 ------------------------------------------------
set "PDF_FOUND=0"
for /r "%TEST_DATA_DIR%" %%f in (*.pdf) do set /a PDF_FOUND+=1

if %PDF_FOUND% equ 0 (
    echo ================================================================
    echo   [INFO] test_data\ 下没有 PDF 文件
    echo.
    echo   请将测试用 PDF 结算单放入:
    echo     %TEST_DATA_DIR%\
    echo   然后重新运行本脚本。
    echo ================================================================
    echo [INFO] 未找到测试 PDF >> "%REPORT%"
    pause
    exit /b 0
)

echo   [INFO] 找到 %PDF_FOUND% 个 PDF 文件
echo 找到 %PDF_FOUND% 个 PDF 文件 >> "%REPORT%"
echo.

:: =====================================================================
:: 测试辅助函数
:: =====================================================================
goto :skip_helper

:run_test
set /a TEST_COUNT+=1
set "TEST_NAME=%~1"
set "TEST_ARGS=%~2"

:: 安全的文件名
set "SAFE_NAME=%TEST_NAME: =_%"
set "SAFE_NAME=%SAFE_NAME:/=_%"
set "SAFE_NAME=%SAFE_NAME:\=_%"
set "SAFE_NAME=%SAFE_NAME::=_%"
set "SAFE_NAME=%SAFE_NAME:|=_%"
set "LOG_FILE=%LOG_DIR%\%TEST_COUNT%_%SAFE_NAME%.log"

echo.
echo ------------------------------------------------------------------
echo [Test #%TEST_COUNT%] %TEST_NAME%
echo   Cmd: %EXE% %TEST_ARGS%

cmd /c ""%EXE%" %TEST_ARGS% > "%LOG_FILE%" 2>&1"
set TEST_ERR=%ERRORLEVEL%

:: 分类结果
set "RESULT=UNKNOWN"
if %TEST_ERR% equ 0 (
    set "RESULT=PASS"
    set /a PASS_COUNT+=1
    echo   [ PASS ] exit 0
) else if %TEST_ERR% equ -1073741819 (
    set "RESULT=CRASH"
    set /a CRASH_COUNT+=1
    set /a FAIL_COUNT+=1
    echo   [ CRASH ] 0xC0000005 - ACCESS VIOLATION
) else if %TEST_ERR% equ -1073741701 (
    set "RESULT=CRASH"
    set /a CRASH_COUNT+=1
    set /a FAIL_COUNT+=1
    echo   [ CRASH ] 0xC000007B - ARCH MISMATCH (x86/x64)
) else if %TEST_ERR% equ -1073741515 (
    set "RESULT=CRASH"
    set /a CRASH_COUNT+=1
    set /a FAIL_COUNT+=1
    echo   [ CRASH ] 0xC0000135 - DLL MISSING
) else if %TEST_ERR% lss 0 (
    set "RESULT=CRASH"
    set /a CRASH_COUNT+=1
    set /a FAIL_COUNT+=1
    echo   [ CRASH ] 负值退出码: %TEST_ERR%
) else (
    set "RESULT=FAIL"
    set /a FAIL_COUNT+=1
    echo   [ FAIL ] exit %TEST_ERR%
)

:: 失败时提取错误关键词
if /i not "!RESULT!"=="PASS" (
    echo   -- 日志片段 --
    findstr /i "error Error ERROR panic fail 异常 错误 失败" "%LOG_FILE%" 2>nul
    if errorlevel 1 echo     (未匹配到关键词，查看完整日志)
    echo   -- 完整日志: %LOG_FILE%
)

:: 写入报告
echo. >> "%REPORT%"
echo [Test #%TEST_COUNT%] %TEST_NAME% >> "%REPORT%"
echo   命令: %EXE% %TEST_ARGS% >> "%REPORT%"
echo   结果: !RESULT! (exit %TEST_ERR%) >> "%REPORT%"
echo   日志: %LOG_FILE% >> "%REPORT%"
if /i not "!RESULT!"=="PASS" (
    echo   -- 错误信息 -- >> "%REPORT%"
    findstr /i "error Error panic fail 异常" "%LOG_FILE%" >> "%REPORT%" 2>nul
)
goto :eof

:skip_helper

:: =====================================================================
:: Phase 1: 基本处理（每个 PDF 默认参数）
:: =====================================================================
echo ================================================================
echo   Phase 1: 基本 PDF 处理
echo ================================================================
echo. >> "%REPORT%"
echo === Phase 1: 基本处理 === >> "%REPORT%"

for /r "%TEST_DATA_DIR%" %%f in (*.pdf) do (
    set "PDF_PATH=%%f"
    set "PDF_NAME=%%~nxf"
    echo   处理: !PDF_NAME!
    call :run_test "01-Basic-!PDF_NAME!" "!PDF_PATH! -o %LOG_DIR%\basic_output"
)

:: =====================================================================
:: Phase 2: 参数组合测试
:: =====================================================================
echo ================================================================
echo   Phase 2: 参数组合
echo ================================================================
echo. >> "%REPORT%"
echo === Phase 2: 参数组合 === >> "%REPORT%"

for /r "%TEST_DATA_DIR%" %%f in (*.pdf) do (
    set "PDF_PATH=%%f"
    set "PDF_NAME=%%~nxf"

    call :run_test "02-Validate-!PDF_NAME!" "!PDF_PATH! --validate-only"
    call :run_test "02-DumpText-!PDF_NAME!" "!PDF_PATH! --dump-text -o %LOG_DIR%\dump"
    call :run_test "02-NoSummary-!PDF_NAME!" "!PDF_PATH! --no-summary -o %LOG_DIR%\nosum"
    call :run_test "02-SummaryOnly-!PDF_NAME!" "!PDF_PATH! --summary-only -o %LOG_DIR%\sumonly"
    call :run_test "02-Combo1-!PDF_NAME!" "!PDF_PATH! --validate-only --dump-text"
    call :run_test "02-Combo2-!PDF_NAME!" "!PDF_PATH! --no-summary --dump-text -o %LOG_DIR%\combo"
)

:: =====================================================================
:: Phase 3: 边界情况
:: =====================================================================
echo ================================================================
echo   Phase 3: 边界情况
echo ================================================================
echo. >> "%REPORT%"
echo === Phase 3: 边界情况 === >> "%REPORT%"

call :run_test "03-MissingFile" "nonexistent_file_12345.pdf" 
call :run_test "03-WildcardPath" "*.pdf"
call :run_test "03-NonPDF" "%EXE%"

:: =====================================================================
:: Phase 4: 连续重复压力测试
:: =====================================================================
echo ================================================================
echo   Phase 4: 连续重复测试 (5 轮)
echo ================================================================
echo. >> "%REPORT%"
echo === Phase 4: 连续重复测试 === >> "%REPORT%"

set "STRESS_ROUNDS=5"
set "FIRST_PDF="
for /r "%TEST_DATA_DIR%" %%f in (*.pdf) do (
    if "!FIRST_PDF!"=="" set "FIRST_PDF=%%f"
)

if not "%FIRST_PDF%"=="" (
    echo   [INFO] 对 !FIRST_PDF! 重复 %STRESS_ROUNDS% 次

    set "STRESS_FAIL=0"
    for /l %%r in (1,1,%STRESS_ROUNDS%) do (
        cmd /c ""%EXE%" "!FIRST_PDF!" --validate-only >nul 2>&1"
        set ERR=!errorlevel!
        if !ERR! neq 0 (
            set /a STRESS_FAIL+=1
            echo   [FAIL] 第 %%r 轮: exit !ERR!
        )
    )
    echo   重复 %STRESS_ROUNDS% 次: %STRESS_FAIL% 次失败
    echo 重复 %STRESS_ROUNDS% 次校验: %STRESS_FAIL% 次失败 >> "%REPORT%"

    :: Excel 生成压力
    set "STRESS_FAIL2=0"
    for /l %%r in (1,1,3) do (
        cmd /c ""%EXE%" "!FIRST_PDF!" -o "%LOG_DIR%\stress" >nul 2>&1"
        if !errorlevel! neq 0 (
            set /a STRESS_FAIL2+=1
            echo   [FAIL] Excel 第 %%r 次: exit !errorlevel!
        )
    )
    echo   3 次 Excel 生成: %STRESS_FAIL2% 次失败
    echo 3 次 Excel 生成: %STRESS_FAIL2% 次失败 >> "%REPORT%"
) else (
    call :run_test "04-NoPDF-ForStress" "--help"
)

:: =====================================================================
:: Phase 5: 输出验证
:: =====================================================================
echo ================================================================
echo   Phase 5: 输出产物验证
echo ================================================================
echo. >> "%REPORT%"
echo === Phase 5: 输出验证 === >> "%REPORT%"

set "XLSX_COUNT=0"
for /r "%LOG_DIR%" %%f in (*.xlsx) do set /a XLSX_COUNT+=1
echo   Excel 文件: %XLSX_COUNT% 个
echo Excel 文件数: %XLSX_COUNT% >> "%REPORT%"

set "TXT_COUNT=0"
for /r "%LOG_DIR%" %%f in (*.txt) do (
    set "FN=%%~nxf"
    if not "!FN!"=="_TEST_REPORT.txt" set /a TXT_COUNT+=1
)
echo   原始文本导出: %TXT_COUNT% 个
echo 文本文档数: %TXT_COUNT% >> "%REPORT%"

:: =====================================================================
:: 汇总报告
:: =====================================================================
echo.
echo ╔══════════════════════════════════════════════════════════════╗
echo ║        Win7 兼 容 性 测 试 完 成                             ║
echo ╚══════════════════════════════════════════════════════════════╝
echo.
echo ═══════════════════════════════════════════════════════════════
echo   测试汇总
echo ═══════════════════════════════════════════════════════════════
echo   总测试数:   %TEST_COUNT%
echo   通过:       %PASS_COUNT%
echo   失败:       %FAIL_COUNT% (含 %CRASH_COUNT% 次崩溃)
echo   Excel:      %XLSX_COUNT% 个
echo ═══════════════════════════════════════════════════════════════
echo.
echo   详细日志: %LOG_DIR%\
echo   汇总报告: %REPORT%
echo.

:: 写入汇总
echo. >> "%REPORT%"
echo ================================================================ >> "%REPORT%"
echo   最终汇总 >> "%REPORT%"
echo ================================================================ >> "%REPORT%"
echo   总测试:   %TEST_COUNT% >> "%REPORT%"
echo   通过:     %PASS_COUNT% >> "%REPORT%"
echo   失败:     %FAIL_COUNT% (含 %CRASH_COUNT% 次崩溃) >> "%REPORT%"

echo. >> "%REPORT%"
echo ================================================================ >> "%REPORT%"
echo   失败列表 >> "%REPORT%"
echo ================================================================ >> "%REPORT%"
findstr /c:"[ FAIL" /c:"[ CRASH" "%REPORT%" >> "%REPORT%" 2>nul
if errorlevel 1 echo   (无失败项) >> "%REPORT%"

echo. >> "%REPORT%"
echo ================================================================ >> "%REPORT%"
echo   报告结束 >> "%REPORT%"
echo ================================================================ >> "%REPORT%"

echo.
echo   按任意键退出...
pause >nul
exit /b 0