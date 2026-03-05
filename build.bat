@echo off
setlocal

:: ── Paths ─────────────────────────────────────────────────────────
set SDL2=C:/gcc/GCC/Libs/SDL2-2.30.0/x86_64-w64-mingw32
set TTF=C:/gcc/GCC/Libs/SDL2_ttf-devel-2.24.0-mingw/SDL2_ttf-2.24.0/x86_64-w64-mingw32

set INC=-I"%SDL2%/include" -I"%SDL2%/include/SDL2" ^
        -I"%TTF%/include"  -I"%TTF%/include/SDL2"  ^
        -I"src"

set LIB=-L"%SDL2%/lib" -L"%TTF%/lib"

set FLAGS=-O2 -mwindows
set LIBS=-lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf

:: ── Compile resource file (icon + version info) ──────────────────
echo [1/3] Compiling resources...
windres res\app.rc -o build\app_res.o --output-format=coff -I res
if errorlevel 1 ( echo ERROR: windres failed & pause & exit /b 1 )

:: ── Compile sources ───────────────────────────────────────────────
echo [2/3] Compiling sources...
gcc %FLAGS% -c src\main.c  -o build\main.o  %INC%
if errorlevel 1 ( echo ERROR: main.c failed  & pause & exit /b 1 )
gcc %FLAGS% -c src\games.c -o build\games.o %INC%
if errorlevel 1 ( echo ERROR: games.c failed & pause & exit /b 1 )

:: ── Link ──────────────────────────────────────────────────────────
echo [3/3] Linking...
gcc %FLAGS% -o "Game Catalogue.exe" build\main.o build\games.o build\app_res.o %LIB% %LIBS%
if errorlevel 1 ( echo ERROR: link failed & pause & exit /b 1 )

echo.
echo  Build successful  →  Game Catalogue.exe
echo.