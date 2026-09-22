@echo off
setlocal
set "UE_SKIP_UBT_SDK_SETUP=1"
set "UE=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set "PROJECT=D:\RefinedBadger\Worktrees\Soul-realtime-battle-20260921\Soul.uproject"
set "MAP=/Game/Soul/Maps/Playtest/LV_Soul_RealtimeBattleArena"

if not exist "%UE%" (
  echo UE 5.8 not found: %UE%
  pause
  exit /b 1
)
if not exist "%PROJECT%" (
  echo Soul realtime battle worktree not found: %PROJECT%
  pause
  exit /b 1
)

start "" "%UE%" "%PROJECT%" %MAP% -game -windowed -ResX=1600 -ResY=900 -NoSplash -log
endlocal
