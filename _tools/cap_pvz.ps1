Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
public class WU {
  public delegate bool EnumWindowsProc(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc cb, IntPtr l);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern int GetWindowTextLength(IntPtr h);
  [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int n);
  public struct RECT { public int Left, Top, Right, Bottom; }
}
"@

$targetPid = [int]$args[0]
$script:found = New-Object System.Collections.ArrayList

$cb = [WU+EnumWindowsProc]{
  param($h, $l)
  $wpid = 0
  [WU]::GetWindowThreadProcessId($h, [ref]$wpid) | Out-Null
  if ($wpid -eq $script:targetPid -and [WU]::IsWindowVisible($h)) {
    $len = [WU]::GetWindowTextLength($h)
    $sb = New-Object System.Text.StringBuilder ($len + 1)
    [WU]::GetWindowText($h, $sb, $sb.Capacity) | Out-Null
    $r = New-Object WU+RECT
    [WU]::GetWindowRect($h, [ref]$r) | Out-Null
    $null = $script:found.Add([pscustomobject]@{ H = $h; Title = $sb.ToString(); Left = $r.Left; Top = $r.Top; Right = $r.Right; Bottom = $r.Bottom })
  }
  return $true
}
$script:targetPid = $targetPid
[WU]::EnumWindows($cb, [IntPtr]::Zero) | Out-Null

if ($script:found.Count -eq 0) { Write-Output "NO_VISIBLE_WINDOW"; exit 0 }
foreach ($w in $script:found) {
  Write-Output ("H=0x{0:X} Title='{1}' rect={2},{3},{4},{5}" -f $w.H, $w.Title, $w.Left, $w.Top, $w.Right, $w.Bottom)
}
$w = $script:found[0]
# bring to front and restore if minimized
[WU]::ShowWindow($w.H, 9) | Out-Null
[WU]::SetForegroundWindow($w.H) | Out-Null
Start-Sleep -Milliseconds 600
# capture window rect
$W = $w.Right - $w.Left
$Ht = $w.Bottom - $w.Top
$bmp = New-Object System.Drawing.Bitmap($W, $Ht)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($w.Left, $w.Top, 0, 0, (New-Object System.Drawing.Size($W, $Ht)))
$out = $args[1]
$bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()
Write-Output ("SAVED {0} {1}x{2}" -f $out, $W, $Ht)
