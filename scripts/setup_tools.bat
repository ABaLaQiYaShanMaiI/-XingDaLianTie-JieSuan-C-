@echo off
setlocal enabledelayedexpansion

:: =====================================================================
::   XingDa JieSuan - External Tool Auto-Detection & Setup
::   XingDa JieSuan C++ v3.0.0
::
::   Automatically detects installed pdftotext (Poppler) and
::   tesseract-OCR, copies them to tools\ portable directory.
::
::   Enables the program to run on any Windows machine without
::   requiring the user to manually add tools to system PATH.
::
::   Usage: Double-click
:: =====================================================================

set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%.."
set "TOOLS_DIR=%PROJECT_DIR%\tools"

cd /d "%PROJECT_DIR%"

echo ================================================================
echo   XingDa JieSuan - External Tool Auto-Setup
echo   XingDa JieSuan C++ v3.0.0
echo ================================================================
echo.
echo   Project dir: %PROJECT_DIR%
echo   Tools dir:   %TOOLS_DIR%
echo.

:: Create tools directory
if not exist "%TOOLS_DIR%" mkdir "%TOOLS_DIR%"

set "LOG_FILE=%TOOLS_DIR%\setup_log.txt"
echo XingDa JieSuan - Tool Setup Log > "%LOG_FILE%"
echo Time: %date% %time% >> "%LOG_FILE%"
echo ================================================================ >> "%LOG_FILE%"
echo. >> "%LOG_FILE%"

:: =====================================================================
:: 1. Detect pdftotext (Poppler)
:: =====================================================================
echo --- 1. pdftotext (Poppler) ---
echo --- 1. pdftotext (Poppler) --- >> "%LOG_FILE%"

set "PDFTOTEXT_FOUND=0"

:: Check if already in tools/
if exist "%TOOLS_DIR%\pdftotext.exe" (
    echo   [ OK ] tools\pdftotext.exe already present
    echo   [ OK ] tools\pdftotext.exe already present >> "%LOG_FILE%"
    set "PDFTOTEXT_FOUND=1"
    goto :check_tesseract
)

:: Search common install locations
for %%d in (
    "C:\Program Files\poppler\bin"
    "C:\Program Files (x86)\poppler\bin"
    "C:\poppler\bin"
    "%USERPROFILE%\Downloads\poppler*\bin"
) do (
    if exist %%d\pdftotext.exe (
        echo   Found: %%d\pdftotext.exe
        echo   Found: %%d\pdftotext.exe >> "%LOG_FILE%"
        copy /y %%d\pdftotext.exe "%TOOLS_DIR%\" >nul 2>&1
        if exist %%d\pdftoppm.exe (
            copy /y %%d\pdftoppm.exe "%TOOLS_DIR%\" >nul 2>&1
            echo   Copied: pdftoppm.exe
            echo   Copied: pdftoppm.exe >> "%LOG_FILE%"
        )
        echo   [ OK ] pdftotext.exe copied to tools\
        echo   [ OK ] pdftotext.exe copied to tools\ >> "%LOG_FILE%"
        set "PDFTOTEXT_FOUND=1"
        goto :check_tesseract
    )
)

:: Check system PATH
where pdftotext >nul 2>&1
if !errorlevel! equ 0 (
    for /f "delims=" %%i in ('where pdftotext') do (
        echo   Found (PATH): %%i
        echo   Found (PATH): %%i >> "%LOG_FILE%"
        copy /y "%%i" "%TOOLS_DIR%\" >nul 2>&1
    )
    echo   [ OK ] pdftotext.exe copied from PATH to tools\
    echo   [ OK ] pdftotext.exe copied from PATH to tools\ >> "%LOG_FILE%"
    set "PDFTOTEXT_FOUND=1"
    goto :check_tesseract
)

echo   [WARN] pdftotext.exe not found
echo   [WARN] pdftotext.exe not found >> "%LOG_FILE%"

:check_tesseract

:: =====================================================================
:: 2. Detect tesseract-OCR
:: =====================================================================
echo.
echo --- 2. Tesseract-OCR ---
echo. >> "%LOG_FILE%"
echo --- 2. Tesseract-OCR --- >> "%LOG_FILE%"

set "TESSERACT_FOUND=0"

:: Check if already portable
if exist "%TOOLS_DIR%\tesseract\tesseract.exe" (
    echo   [ OK ] tools\tesseract\tesseract.exe already present
    echo   [ OK ] tools\tesseract\tesseract.exe already present >> "%LOG_FILE%"
    set "TESSERACT_FOUND=1"
    goto :summary
)

:: Search common install locations
for %%d in (
    "C:\Program Files\Tesseract-OCR"
    "C:\Program Files (x86)\Tesseract-OCR"
    "%LOCALAPPDATA%\Tesseract-OCR"
    "%LOCALAPPDATA%\Programs\Tesseract-OCR"
) do (
    if exist "%%d\tesseract.exe" (
        echo   Found: %%d\tesseract.exe
        echo   Found: %%d\tesseract.exe >> "%LOG_FILE%"
        
        set "TESS_DIR=%TOOLS_DIR%\tesseract"
        if not exist "!TESS_DIR!" mkdir "!TESS_DIR!"
        
        :: Copy exe
        copy /y "%%d\tesseract.exe" "!TESS_DIR!\" >nul 2>&1
        echo     Copied: tesseract.exe
        echo     Copied: tesseract.exe >> "%LOG_FILE%"
        
        :: Copy all dlls
        for %%f in ("%%d\*.dll") do (
            copy /y "%%f" "!TESS_DIR!\" >nul 2>&1
            echo     Copied: %%~nxf
            echo     Copied: %%~nxf >> "%LOG_FILE%"
        )
        
        :: Copy tessdata language packs
        if exist "%%d\tessdata" (
            if not exist "!TESS_DIR!\tessdata" mkdir "!TESS_DIR!\tessdata"
            xcopy /e /y "%%d\tessdata\*" "!TESS_DIR!\tessdata\" >nul 2>&1
            for %%t in ("!TESS_DIR!\tessdata\*.traineddata") do (
                echo     Lang pack: %%~nxt
                echo     Lang pack: %%~nxt >> "%LOG_FILE%"
            )
        )
        
        set "TESSERACT_FOUND=1"
        goto :summary
    )
)

:: Check system PATH
where tesseract >nul 2>&1
if !errorlevel! equ 0 (
    for /f "delims=" %%i in ('where tesseract') do (
        echo   Found (PATH): %%i
        set "TESS_ROOT=%%~dpi"
        set "TESS_ROOT=!TESS_ROOT:~0,-1!"
        
        set "TESS_DIR=%TOOLS_DIR%\tesseract"
        if not exist "!TESS_DIR!" mkdir "!TESS_DIR!"
        
        copy /y "%%i" "!TESS_DIR!\" >nul 2>&1
        echo     Copied: tesseract.exe
        
        :: Try copying dlls and tessdata from install dir
        if exist "!TESS_ROOT!\*.dll" (
            for %%f in ("!TESS_ROOT!\*.dll") do (
                copy /y "%%f" "!TESS_DIR!\" >nul 2>&1
            )
            echo     Copied: *.dll
        )
        if exist "!TESS_ROOT!\tessdata" (
            if not exist "!TESS_DIR!\tessdata" mkdir "!TESS_DIR!\tessdata"
            xcopy /e /y "!TESS_ROOT!\tessdata\*" "!TESS_DIR!\tessdata\" >nul 2>&1
            echo     Copied: tessdata\
        )
        
        echo   [ OK ] Tesseract copied from PATH to tools\
        echo   [ OK ] Tesseract copied from PATH to tools\ >> "%LOG_FILE%"
        set "TESSERACT_FOUND=1"
        goto :summary
    )
)

echo   [WARN] tesseract.exe not found
echo   [WARN] tesseract.exe not found >> "%LOG_FILE%"

:summary

:: =====================================================================
:: Summary
:: =====================================================================
echo.
echo ================================================================
echo   Setup Summary
echo ================================================================
echo.
echo. >> "%LOG_FILE%"
echo ================================================================ >> "%LOG_FILE%"
echo   Setup Summary >> "%LOG_FILE%"
echo ================================================================ >> "%LOG_FILE%"
echo. >> "%LOG_FILE%"

:: pdftotext status
if %PDFTOTEXT_FOUND% equ 1 (
    echo   [ OK ] pdftotext configured - PDF text extraction available
    echo   [ OK ] pdftotext configured >> "%LOG_FILE%"
) else (
    echo   [ !! ] pdftotext NOT configured - PDF core functionality unavailable!
    echo   [ !! ] pdftotext NOT configured >> "%LOG_FILE%"
    echo.
    echo   pdftotext is required for PDF processing.
    echo   Download Poppler for Windows:
    echo     https://github.com/oschwartz10612/poppler-windows/releases/
    echo   Extract and copy bin\pdftotext.exe to tools\ directory.
)

echo.
echo. >> "%LOG_FILE%"

:: Tesseract status
if %TESSERACT_FOUND% equ 1 (
    echo   [ OK ] Tesseract configured - OCR functionality available
    echo   [ OK ] Tesseract configured >> "%LOG_FILE%"
) else (
    echo   [ -- ] Tesseract not configured - OCR unavailable (core functions unaffected)
    echo   [ -- ] Tesseract not configured >> "%LOG_FILE%"
    echo.
    echo   For scanned/image-based PDFs, install Tesseract-OCR:
    echo     https://github.com/UB-Mannheim/tesseract/wiki
    echo   Re-run this script after installation for auto-config.
)

echo.
echo ================================================================
echo   tools\ directory contents:
echo ================================================================
dir /b /s "%TOOLS_DIR%" 2>nul
echo.
echo   Log: %LOG_FILE%
echo.

if %PDFTOTEXT_FOUND% equ 0 (
    echo ╔════════════════════════════════════════════════════════════╗
    echo ║  [!] WARNING: pdftotext not configured!                 ║
    echo ║                                                          ║
    echo ║  PDF text extraction is a core function.                 ║
    echo ║  The program WILL NOT RUN without pdftotext.             ║
    echo ║                                                          ║
    echo ║  Download (choose poppler-xx.x.x.zip):                   ║
    echo ║  https://github.com/oschwartz10612/poppler-windows/      ║
    echo ║                     releases/latest                      ║
    echo ║                                                          ║
    echo ║  After download, extract and place bin\pdftotext.exe in: ║
    echo ║  %TOOLS_DIR%\                                            ║
    echo ║                                                          ║
    echo ║  Then re-run this script to verify.                      ║
    echo ╚════════════════════════════════════════════════════════════╝
)

echo.
echo   Press any key to exit...
pause >nul
exit /b 0