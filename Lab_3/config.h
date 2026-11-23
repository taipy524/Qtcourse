#ifndef CONFIG_H
#define CONFIG_H

#include <QString>

// ---------------------------------------------------------------------------
// 开发者信息：实验要求“在界面的某个位置添加开发者的学号和姓名”
// 只改这两个常量即可，欢迎页与主窗口状态栏会自动同步。
// ---------------------------------------------------------------------------
namespace AppInfo {
static const QString kStudentId   = QStringLiteral("2022000000");   // TODO: 改成你的学号
static const QString kStudentName = QStringLiteral("请填写姓名");    // TODO: 改成你的姓名
static const QString kAppTitle    = QStringLiteral("医院诊疗系统 - 用户信息管理");
}

// 数据库默认参数（可被可执行文件同目录下的 dbconfig.ini 覆盖）
namespace DbDefaults {
static const QString kHost     = QStringLiteral("127.0.0.1");
static const int     kPort     = 3306;
static const QString kDatabase = QStringLiteral("hospital");
static const QString kUser     = QStringLiteral("hospital");
static const QString kPassword = QStringLiteral("Hospital@2026");
static const QString kDriver   = QString();   // 为空则自动探测 MySQL ODBC Unicode 驱动
}

#endif // CONFIG_H
