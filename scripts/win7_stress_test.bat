@echo off
setlocal enabledelayedexpansion

:: =====================================================================
::   Windows 7 Stress Test Script
::   XingDa JieSuan C++ v3.0.0
::
::   Repeated processing test for:
::   - Memory leak detection
::   - File handle leak detection
::   - Temp file cleanup verification
::   - Stability validation
::
::   Usage: cmd: win7_stress_test.bat [rounds]
::          Default 50 rounds, e.g.: win7_stress_test.bat 200
::
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
set "LOG_DIR=%PROJECT_DIR%\test_logs\stress"
set "TEST_DATA_DIR=%PROJECT_DIR%\test_data"
set "REPORT=%LOG_DIR%\_STRESS_REPORT.txt"

:: Rounds - default 50
set "ROUNDS=%~1"
if "%ROUNDS%"=="" set "ROUNDS=50"
if %ROUNDS% lss 1 set "ROUNDS=1"
if %ROUNDS% gtr 10000 set "ROUNDS=10000"

:: Create log directory
if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"

:: Write report header
echo ================================================================ > "%REPORT%"
echo   XingDa JieSuan C++ Win7 Stress Test Report >> "%REPORT%"
echo   Time: %date% %time% >> "%REPORT%"
echo   Target rounds: %ROUNDS% >> "%REPORT%"
echo ================================================================ >> "%REPORT%"
echo. >> "%REPORT%"

echo ================================================================
echo   XingDa JieSuan - Win7 Stress Test
echo   Rounds: %ROUNDS%
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

:: Find first test PDF
set "TEST_PDF="
for /r "%TEST_DATA_DIR%" %%f in (*.pdf) do (
    if "!TEST_PDF!"=="" set "TEST_PDF=%%f"
)

if "%TEST_PDF%"=="" (
    echo ================================================================
    echo   [INFO] No PDF files in test_data\
    echo   Will use --help mode for stress test (no PDF mode)
    echo ================================================================
    set "TEST_PDF=NONE"
    echo [INFO] No test PDF, using --help mode >> "%REPORT%"
) else (
    echo   [ OK ] Test PDF: %TEST_PDF%
    echo Test PDF: %TEST_PDF% >> "%REPORT%"
)

echo.
echo. >> "%REPORT%"

:: ---- Initialize counters ---------------------------------------------
set "PASS_COUNT=0"
set "FAIL_COUNT=0"
set "CRASH_COUNT=0"
set "MEM_LEAK_SUSPECT=0"
set "FIRST_RUNTIME="
set "LAST_RUNTIME="

:: Get initial system state
for /f "tokens=2 delims==" %%m in ('wmic OS get TotalVisibleMemorySize /value 2^>nul') do set "INIT_MEM=%%m"
for /f "tokens=2 delims==" %%f in ('wmic OS get FreePhysicalMemory /value 2^>nul') do set "INIT_FREE=%%f"

echo   [INFO] Initial free memory: !INIT_FREE! KB / !INIT_MEM! KB
echo Initial free memory: !INIT_FREE! KB / !INIT_MEM! KB >> "%REPORT%"
echo.

:: =====================================================================
:: Main loop
:: =====================================================================

set "START_TIME=%time%"
for /l %%r in (1,1,%ROUNDS%) do (
    set "ROUND_START=%time%"
    
    if "%TEST_PDF%"=="NONE" (
        cmd /c ""%EXE%" --help > "%LOG_DIR%\rnd_%%r.log" 2>&1"
    ) else (
        :: Alternate argument combos
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
    
    :: Classify result
    if !ERR! equ 0 (
        set /a PASS_COUNT+=1
        if "!FIRST_RUNTIME!"=="" (
            set "FIRST_RUNTIME=!time!"
        )
        set "LAST_RUNTIME=!time!"
    ) else if !ERR! lss 0 (
        set /a CRASH_COUNT+=1
        set /a FAIL_COUNT+=1
        echo   [CRASH] Round %%r: exit !ERR!
        echo Round %%r: CRASH (exit !ERR!) >> "%REPORT%"
        findstr /i "error panic fail" "%LOG_DIR%\rnd_%%r.log" >> "%REPORT%" 2>nul
    ) else (
        set /a FAIL_COUNT+=1
        echo   [FAIL]  Round %%r: exit !ERR!
        echo Round %%r: FAIL (exit !ERR!) >> "%REPORT%"
        findstr /i "error fail" "%LOG_DIR%\rnd_%%r.log" >> "%REPORT%" 2>nul
    )
    
    :: Progress every 10 rounds
    set /a "PROGRESS=%%r %% 10"
    if !PROGRESS! equ 0 (
        for /f "tokens=2 delims==" %%m in ('wmic OS get FreePhysicalMemory /value 2^>nul') do set "CUR_FREE=%%m"
        echo   [%%r/%ROUNDS%] Pass: !PASS_COUNT! / Fail: !FAIL_COUNT! / Crash: !CRASH_COUNT! / Free mem: !CUR_FREE! KB
    )
    
    :: Memory leak detection (every 25 rounds)
    set /a "MEM_CHECK=%%r %% 25"
    if !MEM_CHECK! equ 0 (
        for /f "tokens=2 delims==" %%m in ('wmic OS get FreePhysicalMemory /value 2^>nul') do set "CUR_FREE=%%m"
        
        if defined INIT_FREE if defined CUR_FREE (
            set /a "MEM_USED_INIT=!INIT_MEM! - !INIT_FREE!"
            set /a "MEM_USED_CUR=!INIT_MEM! - !CUR_FREE!"
            set /a "MEM_DIFF=!MEM_USED_CUR! - !MEM_USED_INIT!"
            if !MEM_DIFF! gtr 100000 (
                set /a MEM_LEAK_SUSPECT+=1
                echo   [WARN] Round %%r: Possible memory leak (used +!MEM_DIFF! KB^)
                echo Round %%r: Suspected memory leak (used +!MEM_DIFF! KB^) >> "%REPORT%"
            )
        )
    )
)

set "END_TIME=%time%"

:: =====================================================================
:: Final system state
:: =====================================================================
echo.
echo ================================================================
echo   Final System State
echo ================================================================

for /f "tokens=2 delims==" %%m in ('wmic OS get FreePhysicalMemory /value 2^>nul') do set "FINAL_FREE=%%m"
echo   Final free memory: !FINAL_FREE! KB
echo Final free memory: !FINAL_FREE! KB >> "%REPORT%"

:: Check temp file residue
echo.
echo   Checking temp files...
set "TEMP_LEAK=0"
for /r "%LOG_DIR%" %%f in (*.png *.ppm) do set /a TEMP_LEAK+=1
if !TEMP_LEAK! gtr 0 (
    echo   [WARN] Found !TEMP_LEAK! orphan temp file(s)
    echo Orphan temp files: !TEMP_LEAK! >> "%REPORT%"
) else (
    echo   [ OK ] No orphan temp files
    echo No orphan temp files >> "%REPORT%"
)

:: Check output files
set "XLS_COUNT=0"
for /r "%LOG_DIR%" %%f in (*.xls) do set /a XLS_COUNT+=1

echo.
echo ╔══════════════════════════════════════════════════════════════╗
echo ║        WIN7 STRESS TEST COMPLETE                              ║
echo ╚══════════════════════════════════════════════════════════════╝
echo.
echo ================================================================
echo   Stress Test Summary
echo ================================================================
echo   Total rounds:    %ROUNDS%
echo   Passed:          %PASS_COUNT%
echo   Failed:          %FAIL_COUNT% (including %CRASH_COUNT% crashes)
echo   Excel files:     %XLS_COUNT%
echo   Suspected leaks: %MEM_LEAK_SUSPECT%
echo   Orphan temp:     %TEMP_LEAK%
echo.
echo   Start time: %START_TIME%
echo   End time:   %END_TIME%
echo ================================================================
echo.

:: Write final summary
echo. >> "%REPORT%"
echo ================================================================ >> "%REPORT%"
echo   Stress Test Summary >> "%REPORT%"
echo ================================================================ >> "%REPORT%"
echo   Total rounds:    %ROUNDS% >> "%REPORT%"
echo   Passed:          %PASS_COUNT% >> "%REPORT%"
echo   Failed:          %FAIL_COUNT% (including %CRASH_COUNT% crashes) >> "%REPORT%"
echo   Excel files:     %XLS_COUNT% >> "%REPORT%"
echo   Suspected leaks: %MEM_LEAK_SUSPECT% >> "%REPORT%"
echo   Orphan temp:     %TEMP_LEAK% >> "%REPORT%"
echo   Start time: %START_TIME% >> "%REPORT%"
echo   End time:   %END_TIME% >> "%REPORT%"

:: Stability assessment
echo. >> "%REPORT%"
echo ================================================================ >> "%REPORT%"
echo   Stability Assessment >> "%REPORT%"
echo ================================================================ >> "%REPORT%"

set /a "TOTAL_FAIL=PASS_COUNT + FAIL_COUNT"
if !TOTAL_FAIL! equ 0 set "TOTAL_FAIL=1"
set /a "PASS_RATE=PASS_COUNT * 100 / TOTAL_FAIL"

if !CRASH_COUNT! gtr 0 (
    echo   Verdict: UNSTABLE - crashes detected >> "%REPORT%"
    echo   Advice: Check crash logs in test_logs\stress\rnd_*.log >> "%REPORT%"
) else if !MEM_LEAK_SUSPECT! gtr 2 (
    echo   Verdict: SUSPECTED MEMORY LEAK >> "%REPORT%"
    echo   Advice: Monitor memory in Task Manager over extended runs >> "%REPORT%"
) else if !PASS_RATE! lss 95 (
    echo   Verdict: Low pass rate (!PASS_RATE!%%^)  >> "%REPORT%"
    echo   Advice: Review failure logs >> "%REPORT%"
) else (
    echo   Verdict: STABLE ✓ (!PASS_RATE!%% pass rate^) >> "%REPORT%"
)

:: Archive failure logs
if !FAIL_COUNT! gtr 0 (
    echo. >> "%REPORT%"
    echo ================================================================ >> "%REPORT%"
    echo   Failed Round Logs >> "%REPORT%"
    echo ================================================================ >> "%REPORT%"
    for /l %%r in (1,1,%ROUNDS%) do (
        findstr /c:"[FAIL" /c:"[CRASH" "%LOG_DIR%\rnd_%%r.log" >nul 2>&1
        if not errorlevel 1 (
            echo --- Round %%r --- >> "%REPORT%"
            type "%LOG_DIR%\rnd_%%r.log" >> "%REPORT%" 2>nul
            echo. >> "%REPORT%"
        )
    )
)

echo. >> "%REPORT%"
echo ================================================================ >> "%REPORT%"
echo   End of Report >> "%REPORT%"
echo ================================================================ >> "%REPORT%"

echo   Detailed logs: %LOG_DIR%\
echo   Report:        %REPORT%
echo.
echo   Press any key to exit...
pause >nul
exit /b 0