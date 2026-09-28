# 实验1 计算器 —— 真实 GUI 自动化测试
# 用真实的鼠标点击（Win32 mouse_event）与真实的键盘输入（SendKeys）
# 驱动运行中的 lab1.exe，并通过 UI Automation 读取显示框内容。
# 每个用例都重新启动一次程序，保证互不干扰。
#
# 用法： powershell -ExecutionPolicy Bypass -File tests\gui_automation.ps1

$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Drawing

Add-Type @"
using System;using System.Runtime.InteropServices;
public class Native {
  [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
  [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint pid);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int cmd);
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x,int y);
  [DllImport("user32.dll")] public static extern void mouse_event(uint flags,uint dx,uint dy,uint data,UIntPtr extra);
}
"@

$exe = Join-Path $PSScriptRoot "..\build\Desktop_Qt_5_15_2_MinGW_64_bit_Debug\debug\lab1.exe"
$exe = (Resolve-Path $exe).Path

# 运行时需要 Qt 与 MinGW 的 DLL（与 Qt Creator 中 Desktop Qt 5.15.2 MinGW 64-bit Kit 一致）
$env:PATH = "D:\QT\5.15.2\mingw81_64\bin;D:\QT\Tools\mingw810_64\bin;" + $env:PATH

$dbgLog = Join-Path $PSScriptRoot 'gui_test_debug.log'
[IO.File]::WriteAllText($dbgLog, '', [Text.Encoding]::UTF8)
function D([string]$m) {
    [IO.File]::AppendAllText($dbgLog, ((Get-Date -Format 'HH:mm:ss.fff') + ' ' + $m + "`r`n"), [Text.Encoding]::UTF8)
}

$script:results = @()

function Start-Calculator {
    [Native]::SetProcessDPIAware() | Out-Null
    Get-Process lab1 -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
    $script:proc = Start-Process -FilePath $exe -PassThru
    Start-Sleep -Seconds 2
    if ($script:proc.HasExited) { throw "lab1.exe 启动失败" }

    $root = [System.Windows.Automation.AutomationElement]::RootElement
    $cond = New-Object System.Windows.Automation.PropertyCondition(
        [System.Windows.Automation.AutomationElement]::ProcessIdProperty, $script:proc.Id)
    $script:win = $root.FindFirst([System.Windows.Automation.TreeScope]::Children, $cond)
    if ($script:win -eq $null) { throw "找不到计算器窗口" }
    Bring-ToFront | Out-Null
    Start-Sleep -Milliseconds 300
}

function Stop-Calculator {
    if ($script:proc -and -not $script:proc.HasExited) { Stop-Process -Id $script:proc.Id -Force }
}

function Get-ForegroundPid {
    $h = [Native]::GetForegroundWindow()
    if ($h -eq [IntPtr]::Zero) { return 0 }
    $pid2 = [uint32]0
    [Native]::GetWindowThreadProcessId($h, [ref]$pid2) | Out-Null
    return [int]$pid2
}

function Bring-ToFront {
    [Native]::ShowWindow($script:proc.MainWindowHandle, 9) | Out-Null   # SW_RESTORE
    [Native]::SetForegroundWindow($script:proc.MainWindowHandle) | Out-Null
    Start-Sleep -Milliseconds 250
    $fg = Get-ForegroundPid
    if ($fg -ne $script:proc.Id) {
        [Native]::SetForegroundWindow($script:proc.MainWindowHandle) | Out-Null
        Start-Sleep -Milliseconds 350
        $fg = Get-ForegroundPid
    }
    if ($fg -ne $script:proc.Id) { D ("WARN foreground=" + $fg + " expect=" + $script:proc.Id) }
    return ($fg -eq $script:proc.Id)
}

function Get-DisplayValue {
    $edit = $script:win.FindFirst([System.Windows.Automation.TreeScope]::Descendants,
        (New-Object System.Windows.Automation.PropertyCondition(
            [System.Windows.Automation.AutomationElement]::ControlTypeProperty,
            [System.Windows.Automation.ControlType]::Edit)))
    if ($edit -eq $null) { return $null }
    try {
        $pattern = $edit.GetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern)
        return $pattern.Current.Value
    } catch {
        return $edit.Current.Name
    }
}

function Find-Button([string]$automationId) {
    $all = $script:win.FindAll([System.Windows.Automation.TreeScope]::Descendants,
        (New-Object System.Windows.Automation.PropertyCondition(
            [System.Windows.Automation.AutomationElement]::ControlTypeProperty,
            [System.Windows.Automation.ControlType]::Button)))
    foreach ($e in $all) {
        $id = $e.Current.AutomationId
        if ($id -eq $automationId -or $id.EndsWith('.' + $automationId)) { return $e }
    }
    return $null
}

function Click-Button([string]$automationId) {
    $el = Find-Button $automationId
    if ($el -eq $null) { throw "找不到按钮 $automationId" }

    $r = $el.Current.BoundingRectangle
    if ($r.Width -le 0) { throw "按钮 $automationId 不可见" }
    $x = [int]($r.X + $r.Width / 2)
    $y = [int]($r.Y + $r.Height / 2)

    Bring-ToFront | Out-Null
    [Native]::SetCursorPos($x, $y) | Out-Null
    Start-Sleep -Milliseconds 80
    [Native]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)   # LEFTDOWN
    Start-Sleep -Milliseconds 60
    [Native]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)   # LEFTUP
    Start-Sleep -Milliseconds 180
    D ("click " + $automationId + " at " + $x + "," + $y + " -> display='" + (Get-DisplayValue) + "'")
}

function Send-Keys([string]$keys) {
    Bring-ToFront | Out-Null
    [System.Windows.Forms.SendKeys]::SendWait($keys)
    Start-Sleep -Milliseconds 250
    D ("send '" + $keys + "' -> display='" + (Get-DisplayValue) + "'")
}

# 按钮名序列翻译成点击动作（鼠标路径）
$buttonOf = @{
    '0'='btn0'; '1'='btn1'; '2'='btn2'; '3'='btn3'; '4'='btn4'
    '5'='btn5'; '6'='btn6'; '7'='btn7'; '8'='btn8'; '9'='btn9'
    '.'='btnDot'; '+'='btnAdd'; '-'='btnSub'; '*'='btnMul'; '/'='btnDiv'
    '='='btnEqual'; 'C'='btnClear'; '<'='btnBackspace'
}

function Invoke-Mouse([string]$sequence) {
    foreach ($ch in $sequence.ToCharArray()) {
        $key = [string]$ch
        if (-not $buttonOf.ContainsKey($key)) { throw "未知按钮符号: $key" }
        Click-Button $buttonOf[$key]
    }
}

function Add-Result([string]$id,[string]$name,[string]$mode,[string]$seq,
                    [string]$expected,[string]$actual,[string]$note='') {
    $pass = ($actual -eq $expected)
    $script:results += [pscustomobject]@{
        ID=$id; Name=$name; Mode=$mode; Input=$seq
        Expected=$expected; Actual=$actual; Note=$note
        Status=$(if($pass){'PASS'}else{'FAIL'})
    }
}

function Invoke-Case([string]$id,[string]$name,[string]$mode,[string]$seq,[string]$expected) {
    D "==== $id ($name) ===="
    Start-Calculator
    $init = Get-DisplayValue
    if ($init -ne '0') {
        Stop-Calculator
        Add-Result $id $name $mode $seq $expected ('ERROR 初始显示=' + $init) '启动异常'
        return
    }
    $note = ''
    try {
        if ($mode -eq 'mouse') { Invoke-Mouse $seq } else { Send-Keys $seq }
        $actual = Get-DisplayValue
    } catch {
        $actual = 'ERROR'
        $note = $_.Exception.Message
    }
    Stop-Calculator
    Start-Sleep -Milliseconds 200
    Add-Result $id $name $mode $seq $expected $actual $note
}

try {
    [Native]::SetProcessDPIAware() | Out-Null

    # ---- T01~T11 鼠标路径 ----
    Invoke-Case 'T01' '加法'            'mouse' '1+2='     '3'
    Invoke-Case 'T02' '减法'            'mouse' '10-3='    '7'
    Invoke-Case 'T03' '乘法'            'mouse' '5*6='     '30'
    Invoke-Case 'T04' '除法'            'mouse' '20/4='    '5'
    Invoke-Case 'T05' '小数运算'        'mouse' '1.5+2.5=' '4'
    Invoke-Case 'T06' '重复小数点'      'mouse' '1.2.3'    '1.23'
    Invoke-Case 'T07' '除零'            'mouse' '10/0='    '不能除以0'
    Invoke-Case 'T08' '退格'            'mouse' '12345<<'  '123'
    Invoke-Case 'T09' '清除'            'mouse' '12345C'   '0'
    Invoke-Case 'T10' '连续计算'        'mouse' '10+5=+3=' '18'
    Invoke-Case 'T10b' '连续计算继续'   'mouse' '10+5=+3=*2=' '36'
    Invoke-Case 'T11' '计算后重新输入'  'mouse' '10+5=2'   '2'

    # T07 附加：除零后能恢复
    D '==== T07b 除零后恢复 ===='
    Start-Calculator
    Invoke-Mouse '10/0='
    $afterErr = Get-DisplayValue
    Click-Button 'btn1'
    Click-Button 'btn2'
    $afterRec = Get-DisplayValue
    Stop-Calculator
    Add-Result 'T07b' '除零后可恢复' 'mouse' '10/0= 然后点 1、2' '12' $afterRec "错误提示=$afterErr"

    # ---- T12~T17 键盘路径（完全不用鼠标） ----
    Invoke-Case 'T12' '键盘运算'        'key' '12{+}5='         '17'
    Invoke-Case 'T13' '键盘退格'        'key' '12345{BS}{BS}'   '123'
    Invoke-Case 'T14' '键盘清除'        'key' '12345{ESC}'      '0'
    Invoke-Case 'T15' '键盘小数与回车'  'key' '1.5{+}2.5{ENTER}' '4'
    Invoke-Case 'T16' '键盘乘法'        'key' '5*6{ENTER}'      '30'
    Invoke-Case 'T17' '键盘除零'        'key' '10/0='           '不能除以0'
    Invoke-Case 'T18' '键盘连续计算'    'key' '10{+}5{ENTER}{+}3{ENTER}' '18'
}
catch {
    D ("[ERROR] " + $_.Exception.Message)
    Write-Host ("[ERROR] " + $_.Exception.Message)
    Stop-Calculator
}
finally {
    Stop-Calculator
}

$pass = @($script:results | Where-Object { $_.Status -eq 'PASS' }).Count
$total = $script:results.Count

$csv = Join-Path $PSScriptRoot 'gui_test_results.csv'
$script:results | Export-Csv -NoTypeInformation -Encoding UTF8 -Path $csv
$txt = Join-Path $PSScriptRoot 'gui_test_results.txt'
$table = $script:results | Format-Table -AutoSize | Out-String -Width 220
[IO.File]::WriteAllText($txt, ($table + "TOTAL: $pass / $total PASS`r`n"), [Text.Encoding]::UTF8)
Write-Host "TOTAL: $pass / $total PASS"
Write-Host "CSV: $csv"
