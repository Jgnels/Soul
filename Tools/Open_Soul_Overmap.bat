@echo off
setlocal
set "ROOT=%~dp0.."
set "REVIEW=%ROOT%\Evidence\WorldOvermap\soul_world_overmap_inspector.html"
if not exist "%REVIEW%" (
  echo Soul overmap inspector not found:
  echo %REVIEW%
  pause
  exit /b 1
)
start "" "%REVIEW%"
endlocal
