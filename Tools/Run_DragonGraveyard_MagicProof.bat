@echo off
setlocal
set "UE_SKIP_UBT_SDK_SETUP=1"
set "UE=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
set "PROJECT=D:\RefinedBadger\Worktrees\Soul-dragon-graveyard-proof-20260922\Soul.uproject"
set "MAP=/Game/Dragon_graveyard/Level/L_showcase_level?game=/Script/SoulRealtimeBattle.SoulRealtimeArenaGameMode"
set "LOG=D:\RefinedBadger\Worktrees\Soul-dragon-graveyard-proof-20260922\Evidence\DragonGraveyard\runtime_magic_proof.log"

"%UE%" "%PROJECT%" "%MAP%" -game -unattended -nosplash -nullrhi -nosound ^
  -SoulRealtimeMagicProof -SoulRealtimeExternalEnvironment -SoulRealtimeVisualUnits ^
  -SoulArenaOriginX=2000 -SoulArenaOriginY=-15000 -SoulArenaOriginZ=0 ^
  "-abslog=%LOG%"
exit /b %ERRORLEVEL%
