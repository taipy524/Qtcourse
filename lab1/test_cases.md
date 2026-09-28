# 测试记录 —— 实验1 带键盘事件的计算器

## 测试环境

| 项目 | 说明 |
| --- | --- |
| 被测程序 | `lab1/build/Desktop_Qt_5_15_2_MinGW_64_bit_Debug/debug/lab1.exe` |
| Qt 版本 | Qt 5.15.2 MinGW 64-bit |
| 采集方式 A | `tests/gui_automation.ps1`：真实鼠标点击 + 真实 SendKeys 键盘输入 + UI Automation 读取显示框 |
| 采集方式 B | `lab1.exe --selftest`：驱动真实按钮 `click()` 信号与真实 `keyPressEvent` |
| 原始数据 | `tests/gui_test_results.csv`、`tests/gui_test_results.txt`、`tests/selftest_results.txt` |

> 下表「实际结果」均为程序**真实运行后读取到的显示框内容**，两种采集方式结果一致。

## 任务书要求的测试（T01 ~ T14）

| 编号 | 测试 | 输入 | 预期结果 | 实际结果 | 是否通过 |
|---|---|---|---|---|---|
| T01 | 加法 | `1 + 2 =` | `3` | `3` | PASS |
| T02 | 减法 | `10 - 3 =` | `7` | `7` | PASS |
| T03 | 乘法 | `5 * 6 =` | `30` | `30` | PASS |
| T04 | 除法 | `20 / 4 =` | `5` | `5` | PASS |
| T05 | 小数 | `1.5 + 2.5 =` | `4` | `4` | PASS |
| T06 | 重复小数点 | `1 . 2 . 3` | 第二个小数点不生效（显示 `1.23`） | `1.23`（第二个 `.` 被忽略） | PASS |
| T07 | 除零 | `10 / 0 =` | 错误提示，程序不崩溃 | `不能除以0`，程序正常继续运行 | PASS |
| T08 | 退格 | `12345` ← ← | `123` | `123` | PASS |
| T09 | 清除 | `12345` → `C` | `0` | `0` | PASS |
| T10 | 连续计算 | `10 + 5 =` 再 `+ 3 =` | `18` | `18` | PASS |
| T11 | 计算后重新输入 | `10 + 5 =` 再输入 `2` | `2`（不是 `152`） | `2` | PASS |
| T12 | 键盘（全程不用鼠标） | `12 + 5 =` | `17` | `17` | PASS |
| T13 | 键盘退格 | `12345` `Backspace` `Backspace` | `123` | `123` | PASS |
| T14 | 键盘清除 | `12345` `Esc` | `0` | `0` | PASS |

## 补充测试

| 编号 | 测试 | 输入 | 预期结果 | 实际结果 | 是否通过 |
|---|---|---|---|---|---|
| T07b | 除零后可恢复 | `10 / 0 =` 之后输入 `1`、`2` | `12` | `12`（错误状态已清除） | PASS |
| T10b | 连续计算继续 | `10 + 5 = + 3 = * 2 =` | `36` | `36` | PASS |
| T15 | 键盘小数与回车 | `1 . 5 + 2 . 5` `Enter` | `4` | `4` | PASS |
| T16 | 键盘乘法 | `5 * 6` `Enter` | `30` | `30` | PASS |
| T17 | 键盘除零 | `10 / 0 =` | `不能除以0` | `不能除以0` | PASS |
| T18 | 键盘连续计算 | `10 + 5` `Enter` `+ 3` `Enter` | `18` | `18` | PASS |

## 汇总

| 采集方式 | 通过 / 总数 | 结果文件 |
|---|---|---|
| 真实 GUI 自动化测试 | 20 / 20 PASS | `tests/gui_test_results.txt` |
| `--selftest` 命令行自测 | 20 / 20 PASS（退出码 0） | `tests/selftest_results.txt` |

## 复现步骤

```bat
:: 方式 A：真实 GUI 自动化
powershell -ExecutionPolicy Bypass -File lab1\tests\gui_automation.ps1
type lab1\tests\gui_test_results.txt

:: 方式 B：命令行自测
cd lab1
build\Desktop_Qt_5_15_2_MinGW_64_bit_Debug\debug\lab1.exe --selftest
type tests\selftest_results.txt
```
