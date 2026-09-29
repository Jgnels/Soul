@echo off
rem Worker-local Terrain V2 preview. Omit -SoulTerrainV2 for c36e371 presentation.
rem Explicit 45 FPS control: 60 FPS/uncapped qualification hit this machine's 85 C cutoff.
set "SOUL_TERRAIN_PROJECT=%~dp0..\Soul.uproject"
"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "%SOUL_TERRAIN_PROJECT%" "/Engine/Maps/Entry?game=/Script/Soul.SoulFounderPlaytestGameMode" -game -windowed -ResX=1920 -ResY=1080 -ForceRes -d3d11 -DisablePlugins=AndroidFileServer,NwiroIntegrationKit -DDC=InstalledNoZenLocalFallback -SoulTerrainV2 "-ExecCmds=t.MaxFPS 45,r.VSync 0"
