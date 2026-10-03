param([string]$X="0",[string]$Y="0",[int]$Sleep=400)
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class M {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x,int y);
  [DllImport("user32.dll")] public static extern void mouse_event(uint f,uint dx,uint dy,uint d,IntPtr e);
  public struct RECT { public int Left,Top,Right,Bottom; }
}
"@
$p = Get-Process PlantsVsZombies -ErrorAction Stop | Select-Object -First 1
$h = $p.MainWindowHandle
if($h -eq [IntPtr]::Zero){ $h = $p.Handle }
[M]::SetForegroundWindow($h) | Out-Null
Start-Sleep -Milliseconds 200
$r = New-Object M+RECT
[M]::GetWindowRect($h,[ref]$r) | Out-Null
$w = $r.Right - $r.Left; $hh = $r.Bottom - $r.Top
$fx = [double]$X; $fy = [double]$Y
$px = $r.Left + [int]($w * $fx); $py = $r.Top + [int]($hh * $fy)
[M]::SetCursorPos($px,$py) | Out-Null
Start-Sleep -Milliseconds 120
[M]::mouse_event(0x2,0,0,0,[IntPtr]::Zero)   # left down
Start-Sleep -Milliseconds 60
[M]::mouse_event(0x4,0,0,0,[IntPtr]::Zero)   # left up
Start-Sleep -Milliseconds $Sleep
Write-Output "clicked ($fx,$fy) -> abs ($px,$py)"
