@echo off
rem Local editor-game candidate; requires existing Mesa setup and licensed content mounts.
rem Separate profile keeps the regular campaign checkpoint untouched.
set "SOUL_CORRIDOR_ROOT=%~dp0.."
"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "%SOUL_CORRIDOR_ROOT%\Soul.uproject" "/Engine/Maps/Entry?game=/Script/Soul.SoulFounderPlaytestGameMode" -game -windowed -ResX=1280 -ResY=720 -ForceRes -d3d11 -DisablePlugins=AndroidFileServer,NwiroIntegrationKit -DDC=InstalledNoZenLocalFallback -SoulMesaTerrain -SoulEvilCorridor "-UserDir=%SOUL_CORRIDOR_ROOT%\Saved\EvilCorridorPlayer" "-ExecCmds=t.MaxFPS 30,r.VSync 0"
