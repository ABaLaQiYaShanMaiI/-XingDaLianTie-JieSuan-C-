@echo off
title XingDa JieSuan v3.0.0

:: ====================================================================
::   XingDa JieSuan C++ v3.0.0 - One-Click Launcher
::   Auto-installs dependencies, builds, and launches.
::   Requires: Visual Studio 2019/2022 with C++ workload
:: ====================================================================

set "ROOT=%~dp0"
cd /d "%ROOT%"

:: ---- Find the built EXE (already compiled) ------------------------
set "EXE="
if exist "%ROOT%build\Release\XingDaLianTieJieSuan.exe"       set "EXE=%ROOT%build\Release\XingDaLianTieJieSuan.exe"
if not defined EXE if exist "%ROOT%build\Debug\XingDaLianTieJieSuan.exe"        set "EXE=%ROOT%build\Debug\XingDaLianTieJieSuan.exe"
if not defined EXE if exist "%ROOT%build_vs\Release\XingDaLianTieJieSuan.exe"   set "EXE=%ROOT%build_vs\Release\XingDaLianTieJieSuan.exe"
if not defined EXE if exist "%ROOT%build_vs\Debug\XingDaLianTieJieSuan.exe"     set "EXE=%ROOT%build_vs\Debug\XingDaLianTieJieSuan.exe"
if not defined EXE if exist "%ROOT%build\bin\Release\XingDaLianTieJieSuan.exe"  set "EXE=%ROOT%build\bin\Release\XingDaLianTieJieSuan.exe"
if not defined EXE if exist "%ROOT%build\bin\Debug\XingDaLianTieJieSuan.exe"    set "EXE=%ROOT%build\bin\Debug\XingDaLianTieJieSuan.exe"
if not defined EXE if exist "%ROOT%build\XingDaLianTieJieSuan.exe"              set "EXE=%ROOT%build\XingDaLianTieJieSuan.exe"
if not defined EXE if exist "%ROOT%XingDaLianTieJieSuan.exe"                    set "EXE=%ROOT%XingDaLianTieJieSuan.exe"

if defined EXE goto :menu

:: ====================================================================
:: --- SETUP PHASE: prep dependencies and build -----------------------
:: ====================================================================
cls
echo ================================================================
echo   XingDa JieSuan v3.0.0 - Setup
echo ================================================================
echo.
echo   First run detected. Setting up dependencies...
echo   This may take 5-15 minutes the first time.
echo.
echo   Project root: %ROOT%
echo.

:: ---- Step 1: Find cmake (from VS install if not on PATH) ---------
set "CMAKE=cmake"
where cmake >nul 2>&1
if %ERRORLEVEL% equ 0 goto :have_cmake

:: Search for VS 2022 / 2019 bundled cmake
for %%v in (
    "C:\Program Files\Microsoft Visual Studio\2022\Community"
    "C:\Program Files\Microsoft Visual Studio\2022\Professional"
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise"
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community"
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional"
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise"
) do (
    if not "%CMAKE%"=="cmake" goto :have_cmake
    for /f "delims=" %%f in ('dir /s /b "%%v\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" 2^>nul') do (
        set "CMAKE=%%f"
    )
)

if not "%CMAKE%"=="cmake" (
    echo   [OK] cmake: %CMAKE%
    goto :have_cmake
)

echo.
echo   CMake not found. You need one of:
echo     - Visual Studio 2019/2022 with C++ workload
echo     - CMake installed separately (https://cmake.org)
echo.
pause
exit /b 1

:have_cmake
echo   [OK] cmake: %CMAKE%

:: ---- Step 2: Setup vcpkg (auto-download if missing) --------------
if not exist "%ROOT%vcpkg\vcpkg.exe" (
    echo.
    echo   Downloading vcpkg (package manager) from GitHub...
    echo.

    :: Try git clone first
    where git >nul 2>&1
    if %ERRORLEVEL% equ 0 (
        echo   Cloning vcpkg via git...
        cd /d "%ROOT%"
        git clone --depth 1 https://github.com/microsoft/vcpkg.git vcpkg 2>&1
        if exist "%ROOT%vcpkg\bootstrap-vcpkg.bat" goto :bootstrap_vcpkg
    )

    :: Fallback: PowerShell + zip
    echo   Downloading vcpkg via PowerShell...
    powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri 'https://github.com/microsoft/vcpkg/archive/refs/heads/master.zip' -OutFile '%ROOT%vcpkg.zip'" 2>nul
    if exist "%ROOT%vcpkg.zip" (
        echo   Extracting...
        powershell -Command "Expand-Archive -Path '%ROOT%vcpkg.zip' -DestinationPath '%ROOT%' -Force" 2>nul
        if exist "%ROOT%vcpkg-master" (
            rmdir /s /q "%ROOT%vcpkg" 2>nul
            move "%ROOT%vcpkg-master" "%ROOT%vcpkg" >nul
        )
        del "%ROOT%vcpkg.zip" 2>nul
    )

    if not exist "%ROOT%vcpkg\bootstrap-vcpkg.bat" (
        echo.
        echo   [ERROR] Could not download vcpkg automatically.
        echo   Please install git or manually download vcpkg:
        echo     https://github.com/microsoft/vcpkg
        pause
        exit /b 1
    )
)

:bootstrap_vcpkg
if not exist "%ROOT%vcpkg\vcpkg.exe" (
    echo.
    echo   Bootstrapping vcpkg (compiling - takes ~1 min)...
    cd /d "%ROOT%vcpkg"
    call bootstrap-vcpkg.bat -disableMetrics
    cd /d "%ROOT%"
    if not exist "%ROOT%vcpkg\vcpkg.exe" (
        echo   [ERROR] vcpkg bootstrap failed.
        pause
        exit /b 1
    )
)

echo   [OK] vcpkg ready

:: ---- Step 3: Install dependencies ---------------------------------
echo.
echo   Installing C++ libraries (wxWidgets)...
echo   This downloads and compiles - may take 5-15 minutes.
echo.

:: Install wxwidgets (specify exact package, not manifest mode for compatibility)
"%ROOT%vcpkg\vcpkg.exe" install wxwidgets --triplet x64-windows
if %ERRORLEVEL% neq 0 (
    echo.
    echo   [WARN] vcpkg install had errors. Trying to continue...
    echo.
)

echo   [OK] Dependencies installed

:: ---- Step 4: Build project ----------------------------------------
echo.
echo   Building XingDa JieSuan...
echo.

if not exist "%ROOT%build" mkdir "%ROOT%build"
cd /d "%ROOT%build"

set "TOOLCHAIN=%ROOT%vcpkg\scripts\buildsystems\vcpkg.cmake"

"%CMAKE%" "%ROOT%" ^
    -DCMAKE_TOOLCHAIN_FILE="%TOOLCHAIN%" ^
    -G "Visual Studio 17 2022" -A x64 2>nul
if %ERRORLEVEL% neq 0 "%CMAKE%" "%ROOT%" -DCMAKE_TOOLCHAIN_FILE="%TOOLCHAIN%" -G "Visual Studio 16 2019" -A x64 2>nul
if %ERRORLEVEL% neq 0 "%CMAKE%" "%ROOT%" -DCMAKE_TOOLCHAIN_FILE="%TOOLCHAIN%" 2>nul

if %ERRORLEVEL% neq 0 (
    cd /d "%ROOT%"
    echo.
    echo.
    echo   [ERROR] CMake configuration failed.
    echo.
    echo   Possible causes:
    echo     - Visual Studio C++ workload not installed
    echo     - vcpkg wxwidgets install didn't complete
    echo.
    echo   Try opening "Developer Command Prompt for VS" and running:
    echo     cd /d "%ROOT%"
    echo     cmake -B build -DCMAKE_TOOLCHAIN_FILE="%TOOLCHAIN%"
    echo     cmake --build build --config Release
    echo.
    pause
    exit /b 1
)

"%CMAKE%" --build . --config Release -j 4
if %ERRORLEVEL% neq 0 "%CMAKE%" --build . --config Debug -j 4
cd /d "%ROOT%"

:: ---- Check result --------------------------------------------------
set "EXE="
if exist "%ROOT%build\Release\XingDaLianTieJieSuan.exe" set "EXE=%ROOT%build\Release\XingDaLianTieJieSuan.exe"
if not defined EXE if exist "%ROOT%build\Debug\XingDaLianTieJieSuan.exe" set "EXE=%ROOT%build\Debug\XingDaLianTieJieSuan.exe"

if defined EXE (
    if exist "%ROOT%classify_rules.yaml" (
        for %%F in ("%EXE%") do copy /Y "%ROOT%classify_rules.yaml" "%%~dpF" >nul 2>&1
    )
    echo.
    echo   ================================================================
    echo   Setup complete! Launching GUI...
    echo   ================================================================
    echo.
    start "" "%EXE%"
    exit /b
)

echo.
echo.
echo   [ERROR] Build did not produce an EXE.
echo   Check the output above for compilation errors.
echo.
echo   Common issues:
echo     - Missing Visual Studio C++ workload
echo     - wxWidgets not found by CMake (install failed)
echo.
echo   To manually debug, open "Developer Command Prompt for VS" and run:
echo     cd /d "%ROOT%"
echo     cmake -B build -DCMAKE_TOOLCHAIN_FILE="%TOOLCHAIN%"
echo     cmake --build build --config Release
echo.
pause
exit /b 1

:: ====================================================================
:: Main menu (project already built)
:: ====================================================================
:menu
cls
echo ================================================================
echo   XingDa JieSuan v3.0.0
echo   EXE: %EXE%
echo ================================================================
echo.
echo   [1] Launch GUI
echo   [2] Open output folder
echo   [3] Check tools (pdftotext / Tesseract)
echo   [4] Run Windows 7 compatibility tests
echo   [5] Run stress test (50 rounds)
echo   [6] Rebuild project
echo.
echo   Type a number and press Enter, or [q] to quit.
echo ================================================================
echo.
set "CHOICE="
set /p "CHOICE=^> "

if /i "%CHOICE%"=="q" exit /b
if "%CHOICE%"=="1" goto :launch
if "%CHOICE%"=="2" goto :openout
if "%CHOICE%"=="3" goto :envcheck
if "%CHOICE%"=="4" goto :tests
if "%CHOICE%"=="5" goto :stress
if "%CHOICE%"=="6" goto :rebuild

echo   Invalid choice. Press any key to retry...
pause >nul
goto :menu

:launch
echo   Launching GUI...
start "" "%EXE%"
exit /b

:openout
if not exist "%ROOT%output" mkdir "%ROOT%output"
start "" "%ROOT%output"
goto :menu

:envcheck
if exist "%ROOT%scripts\setup_tools.bat" call "%ROOT%scripts\setup_tools.bat"
goto :menu

:tests
if exist "%ROOT%scripts\win7_test.bat" call "%ROOT%scripts\win7_test.bat"
goto :menu

:stress
if exist "%ROOT%scripts\win7_stress_test.bat" call "%ROOT%scripts\win7_stress_test.bat" 50
goto :menu

:rebuild
echo   Rebuilding...
del /q "%ROOT%build\Release\XingDaLianTieJieSuan.exe" 2>nul
del /q "%ROOT%build\Debug\XingDaLianTieJieSuan.exe"   2>nul

:: Find cmake again
set "CMAKE=cmake"
where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    for %%v in (
        "C:\Program Files\Microsoft Visual Studio\2022\Community"
        "C:\Program Files\Microsoft Visual Studio\2022\Professional"
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise"
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community"
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional"
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise"
    ) do (
        if not "%CMAKE%"=="cmake" goto :rebuild_cmake_found
        for /f "delims=" %%f in ('dir /s /b "%%v\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" 2^>nul') do (
            set "CMAKE=%%f"
        )
    )
)
:rebuild_cmake_found

if "%CMAKE%"=="cmake" (
    echo   CMake not found. Open "Developer Command Prompt for VS" and:
    echo     cd /d "%ROOT%"
    echo     cmake -B build
    echo     cmake --build build --config Release
    pause
    goto :menu
)

if not exist "%ROOT%build" mkdir "%ROOT%build"
cd /d "%ROOT%build"

set "TOOLCHAIN=%ROOT%vcpkg\scripts\buildsystems\vcpkg.cmake"
if exist "%TOOLCHAIN%" (
    "%CMAKE%" "%ROOT%" -DCMAKE_TOOLCHAIN_FILE="%TOOLCHAIN%" -G "Visual Studio 17 2022" -A x64 2>nul
    if %ERRORLEVEL% neq 0 "%CMAKE%" "%ROOT%" -DCMAKE_TOOLCHAIN_FILE="%TOOLCHAIN%" -G "Visual Studio 16 2019" -A x64 2>nul
) else (
    "%CMAKE%" "%ROOT%" -G "Visual Studio 17 2022" -A x64 2>nul
    if %ERRORLEVEL% neq 0 "%CMAKE%" "%ROOT%" -G "Visual Studio 16 2019" -A x64 2>nul
)

"%CMAKE%" --build . --config Release -j 4
if %ERRORLEVEL% neq 0 "%CMAKE%" --build . --config Debug -j 4
cd /d "%ROOT%"

set "EXE="
if exist "%ROOT%build\Release\XingDaLianTieJieSuan.exe" set "EXE=%ROOT%build\Release\XingDaLianTieJieSuan.exe"
if not defined EXE if exist "%ROOT%build\Debug\XingDaLianTieJieSuan.exe" set "EXE=%ROOT%build\Debug\XingDaLianTieJieSuan.exe"

if not defined EXE (
    echo   Rebuild failed.
    pause
    goto :menu
)

if exist "%ROOT%classify_rules.yaml" (
    for %%F in ("%EXE%") do copy /Y "%ROOT%classify_rules.yaml" "%%~dpF" >nul 2>&1
)
echo   Rebuild succeeded: %EXE%
pause
goto :menu