@echo off
setlocal
set "SOUL_ROOT=%~dp0"
set "UE_EDITOR=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
if not exist "%UE_EDITOR%" (
  echo Unreal Editor 5.8 was not found at:
  echo %UE_EDITOR%
  pause
  exit /b 1
)
start "Soul Vertical Slice" /D "%SOUL_ROOT%" "%UE_EDITOR%" "%SOUL_ROOT%Soul.uproject" /Engine/Maps/Entry -game -windowed -ResX=1600 -ResY=900 -ForceRes -log -DisablePlugins=AndroidFileServer,NwiroIntegrationKit -DDC=InstalledNoZenLocalFallback -SoulRealtimeVisualUnits -SoulRealtimeMagicProof -SoulEvilVisualRoster -SoulActivePerSide=35 -SoulPlayerPool=100 -SoulEnemyPool=100
endlocal
