@echo off
:: ══════════════════════════════════════════════════════════════════
::  installer\package.bat  –  Build exe then create NSIS installer
::  Lives in:  GameCatalogue\installer\
:: ══════════════════════════════════════════════════════════════════
setlocal

:: Go up to the project root so build.bat and source files are found
cd /d "%~dp0.."

echo.
echo [1/2] Building Game Catalogue.exe...
call build.bat
if errorlevel 1 (
    echo ERROR: build.bat failed. Aborting.
    pause & exit /b 1
)

echo.
echo [2/2] Creating NSIS installer...

set "MAKENSIS=makensis"
where makensis >nul 2>&1
if errorlevel 1 (
    if exist "C:\Program Files (x86)\NSIS\makensis.exe" (
        set "MAKENSIS=C:\Program Files (x86)\NSIS\makensis.exe"
    ) else if exist "C:\Program Files\NSIS\makensis.exe" (
        set "MAKENSIS=C:\Program Files\NSIS\makensis.exe"
    ) else (
        echo ERROR: NSIS not found. Download and install it from:
        echo   https://nsis.sourceforge.io/Download
        pause & exit /b 1
    )
)

"%MAKENSIS%" "installer\installer.nsi"
if errorlevel 1 (
    echo ERROR: NSIS compilation failed. Check output above.
    pause & exit /b 1
)

echo.
echo ══════════════════════════════════════════════════════════════
echo  Done!  →  GameCatalogue-Setup-Latest.exe
echo ══════════════════════════════════════════════════════════════
pause
