param([string]$Out="",[string]$Keys="",[int]$HoldKey=0,[int]$HoldMs=0,[int]$MouseX=0,[int]$MouseY=0,[int]$Wheel=0,[int]$PauseAfterMs=-1,[int]$ClickX=-1,[int]$ClickY=-1,[string]$Button='left',[int]$ModifierKey=0)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class SoulControlWindow {
 [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left,Top,Right,Bottom; }
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h,out RECT r);
 [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X,Y; }
 [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h,ref POINT p);
 [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
 [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
 [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h,out uint pid);
 [DllImport("kernel32.dll")] public static extern uint GetCurrentThreadId();
 [DllImport("user32.dll")] public static extern bool AttachThreadInput(uint a,uint b,bool attach);
 [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h,int n);
 [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint code,uint type);
 [DllImport("user32.dll")] public static extern void keybd_event(byte k,byte scan,uint flags,UIntPtr extra);
 [DllImport("user32.dll")] public static extern void mouse_event(uint f,int dx,int dy,int data,UIntPtr extra);
}
"@
$p=Get-Process UnrealEditor -ErrorAction Stop | Where-Object MainWindowHandle -ne 0
if(@($p).Count -ne 1){throw "Expected one visible Unreal instance"}
$processInfo=Get-CimInstance Win32_Process -Filter ('ProcessId='+$p.Id)
if($processInfo.CommandLine -notlike '*Soul-bannerlord-campaign-map-20260929*'){throw 'Visible Unreal is not the authorized Soul worker project'}
$fg=[SoulControlWindow]::GetForegroundWindow()
[uint32]$owner=0
$fgThread=[SoulControlWindow]::GetWindowThreadProcessId($fg,[ref]$owner)
$currentThread=[SoulControlWindow]::GetCurrentThreadId()
Write-Output "Target Unreal PID $($p.Id) foreground PID $owner"
try {
 [SoulControlWindow]::AttachThreadInput($currentThread,$fgThread,$true)|Out-Null
 [SoulControlWindow]::ShowWindow($p.MainWindowHandle,9)|Out-Null
 [SoulControlWindow]::keybd_event(18,[byte]([SoulControlWindow]::MapVirtualKey(18,0)),0,[UIntPtr]::Zero)
 [SoulControlWindow]::keybd_event(18,[byte]([SoulControlWindow]::MapVirtualKey(18,0)),2,[UIntPtr]::Zero)
 [SoulControlWindow]::SetForegroundWindow($p.MainWindowHandle)|Out-Null
} finally {
 [SoulControlWindow]::AttachThreadInput($currentThread,$fgThread,$false)|Out-Null
}
Start-Sleep -Milliseconds 350
if([SoulControlWindow]::GetForegroundWindow() -ne $p.MainWindowHandle){throw 'Unreal is not foreground; no input sent or screenshot captured'}
if($Keys){[System.Windows.Forms.SendKeys]::SendWait($Keys)}
if($HoldKey){try{[SoulControlWindow]::keybd_event($HoldKey,[byte]([SoulControlWindow]::MapVirtualKey($HoldKey,0)),0,[UIntPtr]::Zero);Start-Sleep -Milliseconds $HoldMs}finally{[SoulControlWindow]::keybd_event($HoldKey,[byte]([SoulControlWindow]::MapVirtualKey($HoldKey,0)),2,[UIntPtr]::Zero)}}
if($MouseX -or $MouseY){[SoulControlWindow]::mouse_event(1,$MouseX,$MouseY,0,[UIntPtr]::Zero)}
if($Wheel){
 [SoulControlWindow+RECT]$wheelRect=New-Object 'SoulControlWindow+RECT'
 [SoulControlWindow]::GetWindowRect($p.MainWindowHandle,[ref]$wheelRect)|Out-Null
 [System.Windows.Forms.Cursor]::Position=New-Object Drawing.Point (($wheelRect.Left+$wheelRect.Right)/2),(($wheelRect.Top+$wheelRect.Bottom)/2)
 [SoulControlWindow]::mouse_event(2048,0,0,$Wheel,[UIntPtr]::Zero)
}
if($ClickX -ge 0 -and $ClickY -ge 0){
 [SoulControlWindow+POINT]$point=New-Object 'SoulControlWindow+POINT'
 $point.X=$ClickX; $point.Y=$ClickY
 [SoulControlWindow]::ClientToScreen($p.MainWindowHandle,[ref]$point)|Out-Null
 [System.Windows.Forms.Cursor]::Position=New-Object Drawing.Point $point.X,$point.Y
 try {
  if($ModifierKey){[SoulControlWindow]::keybd_event($ModifierKey,[byte]([SoulControlWindow]::MapVirtualKey($ModifierKey,0)),0,[UIntPtr]::Zero)}
  Start-Sleep -Milliseconds 100
  $down=if($Button -eq 'right'){8}else{2}
  [SoulControlWindow]::mouse_event($down,0,0,0,[UIntPtr]::Zero)
  Start-Sleep -Milliseconds 100
  [SoulControlWindow]::mouse_event(($down*2),0,0,0,[UIntPtr]::Zero)
 } finally {if($ModifierKey){[SoulControlWindow]::keybd_event($ModifierKey,[byte]([SoulControlWindow]::MapVirtualKey($ModifierKey,0)),2,[UIntPtr]::Zero)}}
}
if($PauseAfterMs -ge 0){
 Start-Sleep -Milliseconds $PauseAfterMs
 if([SoulControlWindow]::GetForegroundWindow() -ne $p.MainWindowHandle){throw 'Focus lost before pause'}
 [SoulControlWindow]::keybd_event(80,[byte]([SoulControlWindow]::MapVirtualKey(80,0)),0,[UIntPtr]::Zero)
 Start-Sleep -Milliseconds 160
 [SoulControlWindow]::keybd_event(80,[byte]([SoulControlWindow]::MapVirtualKey(80,0)),2,[UIntPtr]::Zero)
}
Start-Sleep -Milliseconds 500
[SoulControlWindow+RECT]$r=New-Object 'SoulControlWindow+RECT'
[SoulControlWindow]::GetWindowRect($p.MainWindowHandle,[ref]$r)|Out-Null
if($Out){$b=New-Object Drawing.Bitmap ($r.Right-$r.Left),($r.Bottom-$r.Top);$g=[Drawing.Graphics]::FromImage($b);$g.CopyFromScreen($r.Left,$r.Top,0,0,$b.Size);$b.Save($Out,[Drawing.Imaging.ImageFormat]::Png);$g.Dispose();$b.Dispose();Write-Output $Out}
