@echo off
setlocal enabledelayedexpansion

:: =====================================================================
::   Windows 7 Compatibility Test Script
::   XingDa JieSuan C++ v3.0.0
::
::   Runs all PDFs in test_data\ through multiple argument combos.
::   Each test writes a separate log; a summary report is generated.
::
::   Usage: Double-click, or cmd: win7_test.bat
::   Prerequisites: Put test PDFs into test_data\ directory
:: =====================================================================

set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%.."
cd /d "%PROJECT_DIR%"

:: ---- Config -----------------------------------------------------------
set "EXE="
for %%d in (
    "build\Release"
    "build\Debug"
    "build\bin\Release"
    "build\bin\Debug"
    "build"
) do (
    if "!EXE!"=="" if exist "%PROJECT_DIR%\%%d\XingDaLianTieJieSuan.exe" (
        set "EXE=%PROJECT_DIR%\%%d\XingDaLianTieJieSuan.exe"
    )
)
if "!EXE!"=="" set "EXE=%PROJECT_DIR%\build\bin\Release\XingDaLianTieJieSuan.exe"
set "LOG_DIR=%PROJECT_DIR%\test_logs\tests"
set "TEST_DATA_DIR=%PROJECT_DIR%\test_data"
set "REPORT=%LOG_DIR%\_TEST_REPORT.txt"
set "TEST_COUNT=0"
set "PASS_COUNT=0"
set "FAIL_COUNT=0"
set "CRASH_COUNT=0"
set "SKIP_COUNT=0"

:: Create log directory
if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"

:: Write report header
echo ================================================================ > "%REPORT%"
echo   XingDa JieSuan C++ Win7 Compatibility Test Report >> "%REPORT%"
echo   Time: %date% %time% >> "%REPORT%"
echo ================================================================ >> "%REPORT%"
echo. >> "%REPORT%"

echo ================================================================
echo   XingDa JieSuan - Win7 Compatibility Tests
echo   Mode: Exhaustive PDF x arg combos
echo ================================================================
echo.

:: Check EXE exists
if not exist "%EXE%" (
    echo [FATAL] Executable not found
    echo         Searched in: build\Release\, build\Debug\, etc.
    echo.
    echo   The project has not been built yet.
    echo   Run scripts\build_and_start.bat first to compile,
    echo   or build manually:
    echo     cmake -B build
    echo     cmake --build build --config Release
    echo.
    echo [FATAL] EXE not found >> "%REPORT%"
    pause
    exit /b 1
)

echo   [ OK ] Program: %EXE%
echo. >> "%REPORT%"
echo Program: %EXE% >> "%REPORT%"

:: ---- Count PDFs ---------------------------------------------------
set "PDF_FOUND=0"
for /r "%TEST_DATA_DIR%" %%f in (*.pdf) do set /a PDF_FOUND+=1

if %PDF_FOUND% equ 0 (
    echo ================================================================
    echo   [INFO] No PDF files found in test_data\
    echo.
    echo   Please place test PDFs into:
    echo     %TEST_DATA_DIR%\
    echo   Then re-run this script.
    echo ================================================================
    echo [INFO] No test PDFs found >> "%REPORT%"
    pause
    exit /b 0
)

echo   [INFO] Found %PDF_FOUND% PDF file(s)
echo Found %PDF_FOUND% PDF file(s) >> "%REPORT%"
echo.

:: =====================================================================
:: Test helper
:: =====================================================================
goto :skip_helper

:run_test
set /a TEST_COUNT+=1
set "TEST_NAME=%~1"
set "TEST_ARGS=%~2"

:: Safe filename
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

:: Classify result
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
    echo   [ CRASH ] Negative exit code: %TEST_ERR%
) else (
    set "RESULT=FAIL"
    set /a FAIL_COUNT+=1
    echo   [ FAIL ] exit %TEST_ERR%
)

:: Extract error keywords on failure
if /i not "!RESULT!"=="PASS" (
    echo   -- Log snippet --
    findstr /i "error Error ERROR panic fail" "%LOG_FILE%" 2>nul
    if errorlevel 1 echo     (no keywords matched, see full log)
    echo   -- Full log: %LOG_FILE%
)

:: Write to report
echo. >> "%REPORT%"
echo [Test #%TEST_COUNT%] %TEST_NAME% >> "%REPORT%"
echo   Command: %EXE% %TEST_ARGS% >> "%REPORT%"
echo   Result: !RESULT! (exit %TEST_ERR%) >> "%REPORT%"
echo   Log: %LOG_FILE% >> "%REPORT%"
if /i not "!RESULT!"=="PASS" (
    echo   -- Error info -- >> "%REPORT%"
    findstr /i "error Error panic fail" "%LOG_FILE%" >> "%REPORT%" 2>nul
)
goto :eof

:skip_helper

:: =====================================================================
:: Phase 1: Basic processing (default args per PDF)
:: =====================================================================
echo ================================================================
echo   Phase 1: Basic PDF Processing
echo ================================================================
echo. >> "%REPORT%"
echo === Phase 1: Basic Processing === >> "%REPORT%"

for /r "%TEST_DATA_DIR%" %%f in (*.pdf) do (
    set "PDF_PATH=%%f"
    set "PDF_NAME=%%~nxf"
    echo   Processing: !PDF_NAME!
    call :run_test "01-Basic-!PDF_NAME!" "!PDF_PATH! -o %LOG_DIR%\basic_output"
)

:: =====================================================================
:: Phase 2: Argument combos
:: =====================================================================
echo ================================================================
echo   Phase 2: Argument Combos
echo ================================================================
echo. >> "%REPORT%"
echo === Phase 2: Argument Combos === >> "%REPORT%"

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
:: Phase 3: Edge cases
:: =====================================================================
echo ================================================================
echo   Phase 3: Edge Cases
echo ================================================================
echo. >> "%REPORT%"
echo === Phase 3: Edge Cases === >> "%REPORT%"

call :run_test "03-MissingFile" "nonexistent_file_12345.pdf" 
call :run_test "03-WildcardPath" "*.pdf"
call :run_test "03-NonPDF" "%EXE%"

:: =====================================================================
:: Phase 4: Repeated stress
:: =====================================================================
echo ================================================================
echo   Phase 4: Repeated Stress (5 rounds)
echo ================================================================
echo. >> "%REPORT%"
echo === Phase 4: Repeated Stress === >> "%REPORT%"

set "STRESS_ROUNDS=5"
set "FIRST_PDF="
for /r "%TEST_DATA_DIR%" %%f in (*.pdf) do (
    if "!FIRST_PDF!"=="" set "FIRST_PDF=%%f"
)

if not "%FIRST_PDF%"=="" (
    echo   [INFO] Running %STRESS_ROUNDS% rounds on !FIRST_PDF!

    set "STRESS_FAIL=0"
    for /l %%r in (1,1,%STRESS_ROUNDS%) do (
        cmd /c ""%EXE%" "!FIRST_PDF!" --validate-only >nul 2>&1"
        set ERR=!errorlevel!
        if !ERR! neq 0 (
            set /a STRESS_FAIL+=1
            echo   [FAIL] Round %%r: exit !ERR!
        )
    )
    echo   %STRESS_ROUNDS% rounds: %STRESS_FAIL% failures
    echo %STRESS_ROUNDS% rounds (validate): %STRESS_FAIL% failures >> "%REPORT%"

    :: Excel generation stress
    set "STRESS_FAIL2=0"
    for /l %%r in (1,1,3) do (
        cmd /c ""%EXE%" "!FIRST_PDF!" -o "%LOG_DIR%\stress" >nul 2>&1"
        if !errorlevel! neq 0 (
            set /a STRESS_FAIL2+=1
            echo   [FAIL] Excel round %%r: exit !errorlevel!
        )
    )
    echo   3 Excel generations: %STRESS_FAIL2% failures
    echo 3 Excel generations: %STRESS_FAIL2% failures >> "%REPORT%"
) else (
    call :run_test "04-NoPDF-ForStress" "--help"
)

:: =====================================================================
:: Phase 5: Output verification
:: =====================================================================
echo ================================================================
echo   Phase 5: Output Verification
echo ================================================================
echo. >> "%REPORT%"
echo === Phase 5: Output Verification === >> "%REPORT%"

set "XLS_COUNT=0"
for /r "%LOG_DIR%" %%f in (*.xls) do set /a XLS_COUNT+=1
echo   Excel files: %XLS_COUNT%
echo Excel file count: %XLS_COUNT% >> "%REPORT%"

set "TXT_COUNT=0"
for /r "%LOG_DIR%" %%f in (*.txt) do (
    set "FN=%%~nxf"
    if not "!FN!"=="_TEST_REPORT.txt" set /a TXT_COUNT+=1
)
echo   Text exports: %TXT_COUNT%
echo Text file count: %TXT_COUNT% >> "%REPORT%"

:: =====================================================================
:: Summary report
:: =====================================================================
echo.
echo ╔══════════════════════════════════════════════════════════════╗
echo ║        WIN7 COMPATIBILITY TEST COMPLETE                      ║
echo ╚══════════════════════════════════════════════════════════════╝
echo.
echo ================================================================
echo   Test Summary
echo ================================================================
echo   Total tests: %TEST_COUNT%
echo   Passed:      %PASS_COUNT%
echo   Failed:      %FAIL_COUNT% (including %CRASH_COUNT% crashes)
echo   Excel files: %XLS_COUNT%
echo ================================================================
echo.
echo   Detailed logs: %LOG_DIR%\
echo   Report:        %REPORT%
echo.

:: Write summary
echo. >> "%REPORT%"
echo ================================================================ >> "%REPORT%"
echo   Final Summary >> "%REPORT%"
echo ================================================================ >> "%REPORT%"
echo   Total tests: %TEST_COUNT% >> "%REPORT%"
echo   Passed:      %PASS_COUNT% >> "%REPORT%"
echo   Failed:      %FAIL_COUNT% (including %CRASH_COUNT% crashes) >> "%REPORT%"

echo. >> "%REPORT%"
echo ================================================================ >> "%REPORT%"
echo   Failure List >> "%REPORT%"
echo ================================================================ >> "%REPORT%"
findstr /c:"[ FAIL" /c:"[ CRASH" "%REPORT%" >> "%REPORT%" 2>nul
if errorlevel 1 echo   (none) >> "%REPORT%"

echo. >> "%REPORT%"
echo ================================================================ >> "%REPORT%"
echo   End of Report >> "%REPORT%"
echo ================================================================ >> "%REPORT%"

echo.
echo   Press any key to exit...
pause >nul
exit /b 0