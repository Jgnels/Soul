@echo off
setlocal
set "SOUL_PLAY_PYTHON=%USERPROFILE%\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe"
if not exist "%SOUL_PLAY_PYTHON%" (
  echo Soul's verified Python runtime is unavailable. No game or save was changed.
  pause
  exit /b 1
)
"%SOUL_PLAY_PYTHON%" "%~dp0play_four_faction_alpha.py" %*
if errorlevel 1 pause
