# 作业二 验证截图脚本：真实运行程序 + UI Automation 点击 + 截图
# 用法： powershell -ExecutionPolicy Bypass -File tools\capture_samp4_13.ps1
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Drawing

$ErrorActionPreference = "Stop"

# ---- 原生方法：前台激活 + 窗口截图（PrintWindow，可截被遮挡窗口）----
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win32 {
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int X, int Y);
    [DllImport("user32.dll")] public static extern void mouse_event(uint dwFlags, uint dx, uint dy, uint dwData, UIntPtr dwExtraInfo);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hwnd, IntPtr hdcBlt, uint nFlags);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
}
"@

$root   = Split-Path -Parent $PSScriptRoot
$exe    = Join-Path $root "samp4_13TableWidget\samp4_13TableWidget\build\Desktop_Qt_5_15_2_MinGW_64_bit_Debug\debug\samp4_13.exe"
$shots  = Join-Path $root "samp4_13TableWidget\screenshots"
New-Item -ItemType Directory -Force -Path $shots | Out-Null

function Save-WindowShot($hwnd, $path) {
    [Win32+RECT]$r = New-Object Win32+RECT
    [void][Win32]::GetWindowRect($hwnd, [ref]$r)
    $w = $r.Right - $r.Left; $h = $r.Bottom - $r.Top
    $bmp = New-Object System.Drawing.Bitmap($w, $h)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $hdc = $g.GetHdc()
    [void][Win32]::PrintWindow($hwnd, $hdc, 2)   # PW_RENDERFULLCONTENT
    $g.ReleaseHdc($hdc); $g.Dispose()
    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    Write-Host "saved: $path"
}

function Find-ByName($root, $name) {
    $cond = New-Object System.Windows.Automation.PropertyCondition(
        [System.Windows.Automation.AutomationElement]::NameProperty, $name)
    return $root.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $cond)
}

function Invoke-ByName($root, $name) {
    $el = Find-ByName $root $name
    if ($null -eq $el) { Write-Host "NOT FOUND: $name"; return $false }
    $el.GetCurrentPattern(
        [System.Windows.Automation.InvokePattern]::Pattern).Invoke()
    Start-Sleep -Milliseconds 500
    return $true
}

# ---- 启动程序 ----
$env:Path = "D:\QT\5.15.2\mingw81_64\bin;" + $env:Path
$proc = Start-Process -FilePath $exe -PassThru
Start-Sleep -Seconds 3

# 找到主窗口（按进程 ID）
$cond = New-Object System.Windows.Automation.PropertyCondition(
    [System.Windows.Automation.AutomationElement]::ProcessIdProperty, $proc.Id)
$win = [System.Windows.Automation.AutomationElement]::RootElement.FindFirst(
    [System.Windows.Automation.TreeScope]::Children, $cond)
if ($null -eq $win) { throw "main window not found" }
$hwnd = [IntPtr]$win.Current.NativeWindowHandle
[void][Win32]::ShowWindow($hwnd, 5)
[void][Win32]::SetForegroundWindow($hwnd)
Start-Sleep -Milliseconds 800

# 1) 点击前
Save-WindowShot $hwnd (Join-Path $shots "1_before.png")

# 2) 点击工具栏“设置学生名单”
[void](Invoke-ByName $win "设置学生名单")
Start-Sleep -Milliseconds 800
[void][Win32]::SetForegroundWindow($hwnd)
Save-WindowShot $hwnd (Join-Path $shots "2_after_click.png")

# 3) 选中“黄润深”所在行，状态栏显示其籍贯（广东佛山）
$cell = Find-ByName $win "黄润深"
if ($null -ne $cell) {
    $rc = $cell.Current.BoundingRectangle
    [void][Win32]::SetForegroundWindow($hwnd)
    Start-Sleep -Milliseconds 200
    [void][Win32]::SetCursorPos([int]($rc.X + $rc.Width / 2), [int]($rc.Y + $rc.Height / 2))
    [Win32]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)   # left down
    [Win32]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)   # left up
}
Start-Sleep -Milliseconds 700
[void][Win32]::SetForegroundWindow($hwnd)
Save-WindowShot $hwnd (Join-Path $shots "3_hometown.png")

# 4) 名单模式下点击旧按钮“设置行数”，验证防护提示
[void](Invoke-ByName $win "设置行数")
Start-Sleep -Milliseconds 300
[void][Win32]::SetForegroundWindow($hwnd)
Save-WindowShot $hwnd (Join-Path $shots "4_guard.png")

Start-Sleep -Milliseconds 500
$proc.CloseMainWindow() | Out-Null
Start-Sleep -Milliseconds 800
if (!$proc.HasExited) { $proc.Kill() }
Write-Host "done"
