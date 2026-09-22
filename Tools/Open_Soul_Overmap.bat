@echo off
setlocal
set "ROOT=%~dp0.."
set "REVIEW=%ROOT%\Evidence\WorldOvermap\soul_world_overmap_review.html"
if not exist "%REVIEW%" (
  echo Soul overmap review not found:
  echo %REVIEW%
  pause
  exit /b 1
)
start "" "%REVIEW%"
endlocal
