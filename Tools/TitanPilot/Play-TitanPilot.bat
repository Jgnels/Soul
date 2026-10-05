@echo off
setlocal
set "UE_SKIP_UBT_SDK_SETUP=1"
set "PROJECT=%~dp0..\..\Soul.uproject"
"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "%PROJECT%" /Game/Soul/Maps/Soul_TitanPilot -game -windowed -ResX=1280 -ResY=720 -d3d11 -nosplash -ExecCmds="r.RayTracing 0,r.DynamicGlobalIlluminationMethod 0,r.ReflectionMethod 0,r.Shadow.Virtual.Enable 0,sg.ViewDistanceQuality 1,sg.ShadowQuality 1,sg.EffectsQuality 0,sg.PostProcessQuality 0,r.Streaming.PoolSize 2048,t.MaxFPS 60"
endlocal
