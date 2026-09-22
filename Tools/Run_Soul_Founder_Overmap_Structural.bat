@echo off
setlocal EnableExtensions
set "ROOT=%~dp0.."
set "UPROJECT=%ROOT%\Soul.uproject"
set "SCRIPT=%~dp0stage_soul_founder_overmap_structural.py"
set "UECMD=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"

if not exist "%UPROJECT%" (
  echo ERROR: Soul.uproject not found at "%UPROJECT%"
  exit /b 2
)
if not exist "%SCRIPT%" (
  echo ERROR: staging script not found at "%SCRIPT%"
  exit /b 3
)
if not exist "%UECMD%" (
  echo ERROR: UnrealEditor-Cmd.exe not found at "%UECMD%"
  exit /b 4
)

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$busy = Get-CimInstance Win32_Process | Where-Object { " ^
  "($_.Name -match '^UnrealEditor(-Cmd)?\.exe$') -or " ^
  "($_.Name -eq 'dotnet.exe' -and $_.CommandLine -match 'UnrealBuildTool') -or " ^
  "($_.Name -match '^(cl|link)\.exe$') " ^
  "}; if ($busy) { $busy | Select-Object ProcessId,Name,CommandLine | Format-Table -AutoSize; exit 42 }"
if errorlevel 42 (
  echo.
  echo BUSY: another UE/build lane is active. Structural founder staging was NOT launched.
  exit /b 42
)
if errorlevel 1 (
  echo ERROR: process guard failed. Refusing to launch UE.
  exit /b 5
)

echo Launching isolated structural founder-overmap staging...
"%UECMD%" "%UPROJECT%" -ExecutePythonScript="%SCRIPT%" -unattended -nop4 -nosplash -nullrhi -log
set "RESULT=%ERRORLEVEL%"
echo UE_EXIT=%RESULT%
exit /b %RESULT%
