# 实验三：QtSQL 数据库模块与 View-Model 结构的使用

医院诊疗系统 —— 用户信息管理（Qt Widgets + Qt SQL + MySQL 8）

---

## 1. 环境要求

| 项 | 版本 / 说明 |
|---|---|
| Qt | 5.15.2 MinGW 64-bit（本机：`D:\Qt\5.15.2\mingw81_64`，编译器 `D:\Qt\Tools\mingw810_64`） |
| 数据库 | MySQL 8.0（本机服务名 `MySQL80`，端口 3306） |
| ODBC 驱动 | **MySQL Connector/ODBC 8.0 或 9.x 的 64 位版本（必须）** |
| 管理工具 | MySQL Workbench / Navicat（仅用于建库、看数据和截图，程序不依赖） |

> 程序通过 Qt 的 `QODBC` 驱动连接 MySQL，所以 **必须有 MySQL 的 ODBC 驱动**；
> 只装 MySQL Server 是不够的。驱动名由程序自动探测（`dbconfig.ini` 也可手动指定）。

---

## 2. 数据库准备

### 2.1 执行建库脚本

用 root 在 Workbench 中打开并执行 `db/hospital.sql`（或命令行）：

```bash
mysql -uroot -p < db/hospital.sql
```

脚本会自动完成：

- 建库 `hospital`（utf8mb4）
- 建表 `User / Department / Doctor / Patient / History`（含主键、唯一键、外键、CHECK）
- 建视图 `v_patient`、`v_history`
- 建低权限账号 `hospital@localhost`（只给 SELECT/INSERT/UPDATE/DELETE）
- 导入种子数据：2 个账号、4 个科室、5 名医生、10 名患者、2 条日志
- 输出自检结果（表数量、各表行数、患者视图前 5 条）

### 2.2 生成 EER 图（报告用）

Workbench → `Database` → `Reverse Engineer` → 连接 `hospital` → 导出 PNG。

### 2.3 连接参数

连接参数从 **可执行文件同目录** 的 `dbconfig.ini` 读取，首次运行会自动生成：

```ini
[database]
host=127.0.0.1
port=3306
name=hospital
user=hospital
password=Hospital@2026
driver=
```

- `driver` 留空时自动探测 `MySQL ODBC * Unicode Driver`；
  若自动探测失败（例如驱动名特殊），把 Workbench/ODBC 数据源里的完整驱动名填进去，例如：
  `driver=MySQL ODBC 8.0 Unicode Driver`
- 换电脑/换密码只改这个文件，不需要重新编译。

---

## 3. 编译与运行

### 3.1 命令行

```powershell
$env:Path = "D:\Qt\5.15.2\mingw81_64\bin;D:\Qt\Tools\mingw810_64\bin;" + $env:Path
mkdir "E:\yuwans QT\Lab_3\build"
cd "E:\yuwans QT\Lab_3\build"
qmake ..\Lab_3.pro -spec win32-g++
mingw32-make -j4
.\release\Lab_3.exe
```

### 3.2 Qt Creator

打开 `Lab_3/Lab_3.pro`，选择 `Desktop Qt 5.15.2 MinGW 64-bit` 套件，直接运行。

### 3.3 演示账号

| 账号 | 密码 | 说明 |
|---|---|---|
| `admin` | `123456` | 系统管理员 |
| `doctor` | `doctor123` | 医生账号 |

> 口令在数据库中以 SHA-256 存储，登录时由 `QCryptographicHash` 计算后比对。

---

## 4. 功能清单

### 必做

- [x] MVC 分离：`DbManager` 负责数据访问，页面只负责展示与交互
- [x] 用户登录检测（用户名不存在 / 密码错误分开提示）
- [x] 患者列表显示、查询、增加、删除、修改
- [x] 开发者学号 + 姓名显示在欢迎页底部与主窗口状态栏（`config.h`）
- [x] 数据库连接失败明确提示（含已装 ODBC 驱动列表、重试按钮）

### 选做（全部完成）

- [x] 医生信息管理（`QSqlRelationalTableModel`，科室外键自动显示科室名）
- [x] 科室信息管理（删除前检查是否被医生引用）
- [x] 用户操作记录保存与查询（登录、退出、注册、增删改、查询均写 `History`）

### 增强项

- [x] 注册新用户（用户名唯一、密码强度、两次确认）
- [x] 自定义代理：性别列在表格内直接下拉编辑（0/1/2 ↔ 未知/男/女）
- [x] 工具栏代码化：`QToolBar::addWidget()` 添加搜索框，`addAction()` 添加按钮
- [x] 查询无结果空态提示（`QStackedWidget` 切换）
- [x] 删除确认、修改后刷新、行数统计、年龄随出生日期同步

---

## 5. 目录结构

```
Lab_3/
├── Lab_3.pro                  qmake 工程（QT += widgets sql）
├── main.cpp                   程序入口
├── config.h                   学号/姓名、数据库默认参数
├── dbmanager.h/.cpp           数据库单例：连接、驱动探测、错误翻译、日志
├── mainwindow.h/.cpp          QStackedWidget 页面路由 + 状态栏
├── loginpage.h/.cpp           登录页
├── registerdialog.h/.cpp      注册对话框
├── welcomepage.h/.cpp         欢迎页（4 个模块入口）
├── patientpage.h/.cpp         患者管理（QSqlTableModel + 代理 + 过滤 + 空态）
├── patienteditdialog.h/.cpp   患者新增/修改（校验 + 参数化 SQL）
├── doctorpage.h/.cpp          医生管理（QSqlRelationalTableModel）
├── doctoreditdialog.h/.cpp    医生新增/修改（科室下拉）
├── departmentpage.h/.cpp      科室管理（删除前引用检查）
├── departmenteditdialog.h/.cpp
├── historypage.h/.cpp         操作日志（QSqlQueryModel + 筛选）
├── delegates/
│   └── comboboxdelegate.h/.cpp  自定义代理（性别 0/1/2 ↔ 文本）
├── icons/                     Tango 图标（PNG-64）
├── db/
│   └── hospital.sql           建库 + 建表 + 视图 + 种子数据 + 自检
├── resources.qrc              图标资源
└── README.md
```

---

## 6. 测试场景（报告“测试结果”直接引用）

| # | 场景 | 操作 | 预期结果 |
|---|---|---|---|
| 1 | 正常闭环 | 登录 → 查询 → 新增 → 修改 → 删除 → 刷新 | 表格与数据库一致 |
| 2 | 错误凭据 | 输入不存在的用户名 / 错误密码 | 分别提示“用户名不存在”“密码错误” |
| 3 | 重复身份证 | 新增患者时填写已存在的 18 位身份证 | 拦截并提示“已存在（患者：XXX）” |
| 4 | 缺失必填 | 姓名留空、身份证位数不对、手机号格式错误 | 逐项拦截并聚焦对应输入框 |
| 5 | 查询无结果 | 搜索不存在的关键字 | 显示“未找到匹配…的患者记录”，不白屏 |
| 6 | 连接失败 | 停用 MySQL80 服务（或改错 dbconfig.ini 密码）后启动 | 登录页红字提示 + 重试按钮，程序不崩溃 |
| 7 | 删除确认 | 删除时点“取消” | 记录仍在 |
| 8 | 过滤状态删除 | 先按姓名过滤，再删除 | 删除的是选中行（`mapToSource`），不会删错 |
| 9 | 修改刷新 | 修改身高体重后保存 | 表格立即刷新，无需重启 |
| 10 | 科室被引用 | 删除“内科”（下有医生） | 提示“该科室下仍有 N 名医生”，拒绝删除 |
| 11 | 表格内编辑 | 双击患者“性别”列选择“男/女” | 模型 `OnRowChange` 立即写库，刷新后仍为新值 |
| 12 | 数据一致性 | 连续执行 10 次增删改查后执行 `SELECT COUNT(*) FROM Patient` | 与界面“共 N 条记录”一致 |
| 13 | 操作日志 | 完成上述操作后打开“操作日志” | 登录/新增/修改/删除/查询记录齐全，含操作人与时间 |

---

## 7. 报告截图清单（对应模板 7 个步骤）

1. **设计数据库**：Workbench 执行 `hospital.sql` 结果 + EER 图 + `SHOW CREATE TABLE Patient`
2. **设计初步 UI**：登录页、欢迎页
3. **实现界面切换**：登录 → 欢迎 → 患者页 → 返回
4. **添加数据库支持**：`DbManager::open()` 代码 + 连接成功状态栏 + 失败提示截图
5. **实现登录控制**：成功与两种失败提示
6. **患者列表/查询/删除**：列表、无结果空态、删除确认、删除后刷新
7. **患者添加和修改**：编辑对话框、身份证重复拦截、修改后表格刷新
8. **选做**：医生页（科室名）、科室页（拒绝删除）、日志页
9. **Git 日志**：`git log --oneline --graph` 截图
10. **AI 辅助记录**：见第 9 节模板

---

## 8. 常见问题

| 现象 | 原因 / 处理 |
|---|---|
| 登录页提示“未找到 MySQL ODBC 驱动（64 位）” | 安装 MySQL Connector/ODBC x64；确认装的是 64 位 |
| 提示“无法连接 MySQL 服务” | `MySQL80` 服务未启动；或 host/port 不对 |
| 提示“数据库拒绝访问” | `dbconfig.ini` 里的账号密码错误（默认 `hospital/Hospital@2026`） |
| 提示“数据库不存在” | 还没执行 `db/hospital.sql` |
| 中文乱码 | 确认库/表是 `utf8mb4`，连接串带 `CHARSET=utf8mb4`（程序已内置） |
| 保存时报 `Data too long for column 'ID'` | Qt 的 QODBC 绑定字符串会多算 1 个字符；把对应列加长（本工程 UUID 列用 `VARCHAR(36)`、身份证 `VARCHAR(20)`） |
| Qt Creator 找不到 Qt SQL | 检查 Kit 是否为 5.15.2 MinGW，`.pro` 中 `QT += sql` |
| 表格里看不到“科室”名称 | 医生表必须用 `QSqlRelationalTableModel::setRelation`，且 Department 表有数据 |

---

## 9. AI 辅助开发记录模板（报告用）

> **问题**：在带 `QSortFilterProxyModel` 的患者列表里删除记录，偶尔删掉别的患者。
>
> **提示词**：Qt 里 `QTableView` 绑定了 `QSqlTableModel` + `QSortFilterProxyModel`，用 `view->currentIndex().row()` 删除，过滤/排序后删错行，怎么改？
>
> **AI 方案**：AI 建议改用 `QSqlTableModel::removeRow(proxyRow)`，并 `submitAll()`。
>
> **数据验证**：按关键字过滤到只剩 1 条，删除后执行 `SELECT COUNT(*)`，发现被删的是另一名患者。
>
> **发现的问题**：`removeRow` 的参数是 **源模型行号**，而 `proxyRow` 是代理行号；排序后两者不一致。
>
> **自己的修改**：先 `proxy->mapToSource(index)` 取源行号，从 `QSqlRecord` 读主键 `ID`，再用参数化 `DELETE FROM Patient WHERE ID = ?` 删除，并校验 `numRowsAffected()`；删除后 `select()` 并按主键恢复选中行。回归测试 3 次过滤删除均正确。

---

## 10. 待填写

- `config.h` 中的 `kStudentId` / `kStudentName` 目前是占位值，提交前改成真实学号姓名；
- 若使用不同数据库账号，修改 `dbconfig.ini`（不需要改代码）。

---

## 11. 本机实测记录（2026-11-23）

环境：Qt 5.15.2 MinGW 64-bit + MySQL 8.0.46 + MySQL Connector/ODBC 8.0.29 (x64)

| # | 用例 | 实测结果 |
|---|---|---|
| 1 | 启动即连接数据库 | 状态栏显示 `数据库：hospital@127.0.0.1:3306/hospital`，绿色 |
| 2 | `admin/123456` 登录 | 进入欢迎页，日志新增“登录系统” |
| 3 | 错误密码 | 页内红字“密码错误，请重新输入” |
| 4 | 患者列表 | 10 条，性别列显示 男/女（自定义代理） |
| 5 | 关键字过滤“赵” | 1 条，“共 1 条记录（已按关键字过滤）” |
| 6 | 关键字过滤“zzz” | 空态提示“未找到匹配…”，表格不白屏 |
| 7 | 新增患者（含身份证唯一校验） | 保存成功，列表 11 条，日志“新增患者 …” |
| 8 | 修改患者姓名 | 保存后表格立即刷新，日志“修改患者 …” |
| 9 | 删除并确认 | 列表回到 10 条，日志“删除患者 …” |
| 10 | 删除时点“否” | 记录保留，数量仍为 10 |
| 11 | 表格内改性别（自定义代理） | 数据库立即更新，日志恰好 1 条（已做防抖） |
| 12 | 删除“内科”（有 2 名医生） | 弹窗“该科室下仍有 2 名医生，不能删除” |
| 13 | 错误数据库密码启动 | 登录页红字“数据库拒绝访问：账号或密码错误”，重试按钮可用 |
| 14 | 未安装 ODBC 驱动启动 | 提示“未找到 MySQL ODBC 驱动（64 位）”并列出已装驱动 |

### 实测中发现并修复的问题

1. **`Data too long for column 'ID'`**：Qt 的 QODBC 绑定字符串参数时会多算 1 个字符，
   32 位 UUID 写入 `VARCHAR(32)` 溢出。
   *修复*：把 UUID/身份证/手机号等字段放宽（`VARCHAR(36)` / `VARCHAR(20)` / `VARCHAR(13)`），
   `hospital.sql` 同步更新，并增加 “Data too long” 的中文错误提示。
2. **表格内改性别导致日志写两次**：`QSqlTableModel::setData` 与提交各触发一次 `dataChanged`。
   *修复*：加 400ms 单次定时器防抖，确保一次修改只记一条日志。
3. **表格内编辑日志丢失**：把源模型索引误传给 `proxy->mapToSource()`，映射结果无效后提前返回。
   *修复*：信号直接连接源模型，索引按源索引处理（不再映射）。
