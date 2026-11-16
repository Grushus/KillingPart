@echo off
setlocal
rem Killing Part: builds the editor code, then opens the project.
rem Close Unreal before running this.

rem Lives in Tools\; the project is one folder up.
cd /d "%~dp0.."
set "ROOT=%CD%\"
set "PROJ=%ROOT%KillingPart.uproject"
set "LOG=%ROOT%Saved\BuildLog.txt"

tasklist /fi "imagename eq UnrealEditor.exe" 2>nul | find /i "UnrealEditor.exe" >nul
if not errorlevel 1 (
  echo Unreal is still open. Close it first, then run this again.
  echo.
  pause
  exit /b 1
)

set "UE_DIR="
for /f "tokens=2*" %%A in ('reg query "HKLM\SOFTWARE\EpicGames\Unreal Engine\5.9" /v InstalledDirectory 2^>nul ^| find /i "InstalledDirectory"') do set "UE_DIR=%%B"
if not defined UE_DIR set "UE_DIR=C:\Program Files\Epic Games\UE_5.9"

if not exist "%UE_DIR%\Engine\Build\BatchFiles\Build.bat" (
  echo Couldn't find Unreal Engine 5.9. Looked in:
  echo   %UE_DIR%
  echo.
  echo Tell Claude where the engine is installed.
  pause
  exit /b 1
)

if not exist "%ROOT%Saved" mkdir "%ROOT%Saved"

echo Building Killing Part with:
echo   %UE_DIR%
echo.
echo This takes a few minutes the first time. The window stays quiet while it works.
echo.

call "%UE_DIR%\Engine\Build\BatchFiles\Build.bat" KillingPartEditor Win64 Development -Project="%PROJ%" -WaitMutex > "%LOG%" 2>&1

if errorlevel 1 (
  echo BUILD FAILED. These are the last lines of Saved\BuildLog.txt:
  echo.
  powershell -NoProfile -Command "Get-Content -LiteralPath '%LOG%' -Tail 40"
  echo.
  echo Send Claude a screenshot of this window, or the file Saved\BuildLog.txt
  echo.
  pause
  exit /b 1
)

echo Build succeeded. Opening Killing Part with the MCP server on...
start "" "%UE_DIR%\Engine\Binaries\Win64\UnrealEditor.exe" "%PROJ%" -ModelContextProtocolStartServer
endlocal
