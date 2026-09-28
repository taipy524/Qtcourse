# 实验1：带键盘事件的计算器

## 实验名称

Qt Widgets 简易计算器（带键盘事件）—— 实验1

## 实验目的

1. 掌握 Qt Widgets 界面设计方法，学会使用 `QGridLayout`、`QVBoxLayout` 组织控件；
2. 掌握 Qt 信号与槽机制，能够用 `connect` 把界面控件和业务逻辑关联起来；
3. 掌握键盘事件处理方法，能够重写 `keyPressEvent` 处理按键；
4. 理解"鼠标按钮与键盘事件共用同一套核心逻辑"的程序结构，避免重复代码；
5. 养成分阶段编译、分阶段测试的开发习惯，并能够编写测试记录与项目文档。

## 开发环境

| 项目 | 版本/说明 |
| --- | --- |
| 操作系统 | Windows（64 位） |
| IDE | Qt Creator |
| Qt 版本 | Qt 5.15.2 |
| 套件 Kit | Desktop Qt 5.15.2 MinGW 64-bit |
| 编译器 | MinGW g++ 8.1.0（`D:\QT\Tools\mingw810_64`） |
| 构建系统 | qmake（`lab1.pro`） |
| C++ 标准 | C++17（`CONFIG += c++17`） |
| 第三方库 | 无（仅使用 Qt 自带模块 `QT += widgets`） |

## 项目说明

项目仍然是 Qt Creator 中原有的 `lab1` 工程，未新建、未复制、未改动其他项目。

```text
lab1/
├── lab1.pro                 # qmake 工程文件
├── main.cpp                 # 程序入口，含 --selftest 命令行自测入口
├── mainwindow.h             # 主窗口类声明：核心业务函数与内部状态
├── mainwindow.cpp           # 核心计算逻辑 + 按钮信号连接 + 键盘事件
├── mainwindow.ui            # 界面布局（QLineEdit 显示框 + 20 个按钮）
├── README.md                # 本文档
├── test_cases.md            # 测试记录
├── tests/
│   ├── gui_automation.ps1   # 真实 GUI 自动化测试脚本（鼠标点击 + 键盘输入）
│   ├── gui_test_results.csv # GUI 自动化测试结果（原始数据）
│   ├── gui_test_results.txt # GUI 自动化测试结果（表格）
│   └── selftest_results.txt # `--selftest` 自测结果
└── build/                   # 构建目录（已由 .gitignore 忽略）
```

## 功能说明

| 功能 | 说明 |
| --- | --- |
| 数字输入 | `0`~`9`，依次拼接；显示为 `0` 时输入 `5` 得到 `5`，不会出现 `05` |
| 小数输入 | 支持 `1.5`、`12.25`、`0.5`；同一个操作数最多一个小数点，`1.2.3` 的第二个点被忽略 |
| 四则运算 | `+`、`-`、`*`、`/`（界面显示 `+ − × ÷`） |
| 浮点计算 | 支持小数计算，结果显示去掉无意义尾随零（`2.000000` 显示为 `2`） |
| 等号 `=` | 完成一次计算，回车键 `Enter` 等价 |
| 清除 `C` | 界面显示 `0`，同时清空内部全部计算状态 |
| 退格 `←` | 删除最后一位，删空后回落到 `0`，不会出现空字符串 |
| 除零处理 | `10 / 0 =` 显示「不能除以0」，程序不崩溃、不卡死，可继续使用 |
| 连续计算 | `10 + 5 =` 得 `15`，继续 `+ 3 =` 得 `18`，再 `* 2 =` 得 `36`；`1 + 2 + 3 =` 也能正确得到 `6` |
| 计算后重新输入 | 得到结果后直接输入数字，从头开始新操作数（`10 + 5 = 2` 显示 `2`，不是 `152`） |
| 异常输入 | 连续多个运算符、无操作数按 `=`、`1.2.3` 等都不会产生非法状态 |

### 核心结构

```text
                 ┌── 鼠标按钮  → btn1      → inputDigit("1")
核心业务函数 ────┤
                 └── 键盘事件  → keyPress → inputDigit("1")
```

`mainwindow.h` 中声明的统一业务函数：

```cpp
void inputDigit(const QString &digit);   // 数字输入
void inputDot();                         // 小数点输入
void inputOperator(const QString &op);   // 运算符输入
void calculateResult();                  // 等号
bool performCalculation();               // 真正执行一次二元运算（连续计算与等号共用）
void clearCalculator();                  // 清除
void backspace();                        // 退格
void showError();                        // 错误提示（除以 0）
```

内部状态：`m_first`（第一个操作数）、`m_op`（当前运算符）、`m_waiting`（等待第二个操作数）、`m_calculated`（刚完成计算）、`m_error`（错误状态）。

## 键盘操作说明

| 按键 | 功能 |
| --- | --- |
| `0` ~ `9` | 输入数字（小键盘同样支持） |
| `.` | 小数点 |
| `+` | 加 |
| `-` | 减 |
| `*` | 乘 |
| `/` | 除 |
| `=` | 等号 |
| `Enter` / 回车 | 等号 |
| `Backspace` | 退格 |
| `Esc` | 清除 |

焦点处理：显示框、所有按钮、菜单栏均设置为 `Qt::NoFocus`，主窗口 `MainWindow` 使用 `Qt::StrongFocus` 并在 `showEvent()` 中重新获取焦点，因此无论是否点击过按钮，键盘事件都能到达 `MainWindow::keyPressEvent()`，不存在控件抢占焦点导致键盘失效的问题。

## 构建与运行

### Qt Creator

直接打开 `lab1.pro`，选择套件 **Desktop Qt 5.15.2 MinGW 64-bit**，编译运行即可。

### 命令行（与 Qt Creator 使用同一套 Kit）

```bat
cd lab1\build\Desktop_Qt_5_15_2_MinGW_64_bit_Debug
D:\QT\5.15.2\mingw81_64\bin\qmake.exe ..\..\lab1.pro -spec win32-g++ "CONFIG+=debug"
D:\QT\Tools\mingw810_64\bin\mingw32-make.exe
set PATH=D:\QT\5.15.2\mingw81_64\bin;D:\QT\Tools\mingw810_64\bin;%PATH%
debug\lab1.exe
```

## 测试说明

共两种方式，均在真实运行的程序上完成，结果记录在 `test_cases.md`。

### 方式一：真实 GUI 自动化测试（`tests\gui_automation.ps1`）

- 用 Windows 鼠标事件（`mouse_event`）**真实点击**界面按钮；
- 用 `SendKeys` 向窗口**真实发送键盘按键**；
- 通过 UI Automation 读取显示框 `display` 的内容；
- 每个用例单独启动一次程序，互不干扰。

```bat
powershell -ExecutionPolicy Bypass -File tests\gui_automation.ps1
```

结果输出到 `tests\gui_test_results.csv` 与 `tests\gui_test_results.txt`。

### 方式二：命令行自测（`lab1.exe --selftest`）

程序内建自测入口，直接驱动真实窗口的按钮 `click()` 信号与 `keyPressEvent`，覆盖同一批用例，结果写入 `selftest_results.txt`，退出码 `0` 表示全部通过。

```bat
lab1.exe --selftest
```

### 测试结果

| 方式 | 结果 |
| --- | --- |
| 真实 GUI 自动化 | **20 / 20 PASS** |
| `--selftest` 命令行自测 | **20 / 20 PASS**（退出码 0） |

详细记录见 [test_cases.md](test_cases.md)。
