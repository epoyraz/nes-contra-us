@echo off
rem Builds the playable native port (SDL window) into contra_play.exe.
rem Uses the WinLibs MinGW64 + SDL2 toolchain. Run from this folder.
rem Requires baserom.nes in this folder at runtime.
setlocal
set "MG=C:\Users\enesp\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64"

echo Building contra_play.exe (faithful ROM enemy system)...
"%MG%\bin\gcc.exe" -O2 -std=c11 ^
  -DCONTRA_USE_ROM_ENEMY_SYSTEM=1 -DCONTRA_USE_ROM_ENEMY_SYSTEM_L2=1 ^
  -Iport\contra_core\include -I"%MG%\include\SDL2" ^
  port\platform_sdl\main.c port\contra_core\src\core.c ^
  -L"%MG%\lib" -lmingw32 -lSDL2main -lSDL2 -mwindows ^
  -o contra_play.exe
if errorlevel 1 ( echo BUILD FAILED & exit /b 1 )

if not exist SDL2.dll copy /Y "%MG%\bin\SDL2.dll" SDL2.dll >nul
echo.
echo BUILD OK: contra_play.exe
echo Double-click contra_play.exe (or run it here). baserom.nes must be in this folder.
echo Controls: Arrows = move, X/Space = jump, Z/Ctrl = shoot, Enter = Start, Esc = quit.
