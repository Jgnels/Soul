@echo off
rem Approved derived terrain through Soul's game-owned campaign adapter.
rem Run Setup_Soul_Mesa.ps1 once. F5/F9 use the existing scoped RB Save slot.
set "SOUL_MESA_PROJECT=%~dp0..\Soul.uproject"
"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "%SOUL_MESA_PROJECT%" "/Engine/Maps/Entry?game=/Script/Soul.SoulFounderPlaytestGameMode" -game -windowed -ResX=1920 -ResY=1080 -ForceRes -d3d11 -DisablePlugins=AndroidFileServer,NwiroIntegrationKit -DDC=InstalledNoZenLocalFallback -SoulMesaTerrain "-ExecCmds=t.MaxFPS 30,r.VSync 0"
