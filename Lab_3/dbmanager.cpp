#include "dbmanager.h"
#include "config.h"

#include <algorithm>

#include <QCryptographicHash>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QSqlQuery>
#include <QStringList>
#include <QUuid>
#include <QDebug>

namespace {
const char *kConnectionName = "hospital_conn";
}

DbManager &DbManager::instance()
{
    static DbManager manager;
    return manager;
}

QString DbManager::uuid()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString DbManager::sha256(const QString &text)
{
    return QString::fromLatin1(
        QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha256).toHex());
}

void DbManager::ensureIniFile() const
{
    const QString path = QCoreApplication::applicationDirPath() + QStringLiteral("/dbconfig.ini");
    if (QFile::exists(path))
        return;

    QSettings ini(path, QSettings::IniFormat);
    ini.setIniCodec("UTF-8");
    ini.setValue(QStringLiteral("database/host"), DbDefaults::kHost);
    ini.setValue(QStringLiteral("database/port"), DbDefaults::kPort);
    ini.setValue(QStringLiteral("database/name"), DbDefaults::kDatabase);
    ini.setValue(QStringLiteral("database/user"), DbDefaults::kUser);
    ini.setValue(QStringLiteral("database/password"), DbDefaults::kPassword);
    ini.setValue(QStringLiteral("database/driver"), DbDefaults::kDriver);
    ini.sync();
}

QString DbManager::iniValue(const QString &key, const QString &fallback) const
{
    const QString path = QCoreApplication::applicationDirPath() + QStringLiteral("/dbconfig.ini");
    QSettings ini(path, QSettings::IniFormat);
    ini.setIniCodec("UTF-8");
    const QVariant value = ini.value(QStringLiteral("database/") + key);
    if (!value.isValid())
        return fallback;
    const QString text = value.toString();
    return text.isEmpty() ? fallback : text;
}

bool DbManager::detectDriver(QString *driver, QString *errorText) const
{
    const QString configured = iniValue(QStringLiteral("driver"), DbDefaults::kDriver);
    if (!configured.isEmpty()) {
        *driver = configured;
        return true;
    }

    // 读取 64 位 ODBC 驱动注册表，找到 MySQL Connector/ODBC 的 Unicode 驱动名
    QSettings reg(QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\ODBC\\ODBCINST.INI\\ODBC Drivers"),
                  QSettings::NativeFormat);
    QStringList candidates;
    const QStringList keys = reg.childKeys();
    for (const QString &name : keys) {
        if (name.contains(QStringLiteral("MySQL"), Qt::CaseInsensitive)
            && name.contains(QStringLiteral("Unicode"), Qt::CaseInsensitive)) {
            candidates.append(name);
        }
    }

    if (!candidates.isEmpty()) {
        // 驱动名里的版本号越大越优先
        std::sort(candidates.begin(), candidates.end(), [](const QString &a, const QString &b) {
            return a > b;
        });
        *driver = candidates.first();
        return true;
    }

    if (errorText) {
        *errorText = QStringLiteral(
            "未找到 MySQL ODBC 驱动（64 位）。\n\n"
            "请先安装 MySQL Connector/ODBC 8.0 或 9.x 的 64 位版本，"
            "然后在 dbconfig.ini 中填写 driver=驱动名。\n\n"
            "当前已安装的 ODBC 驱动：%1")
            .arg(keys.isEmpty() ? QStringLiteral("（无）") : keys.join(QStringLiteral("、")));
    }
    return false;
}

bool DbManager::open(QString *errorText)
{
    if (m_open)
        return true;

    ensureIniFile();

    m_host     = iniValue(QStringLiteral("host"), DbDefaults::kHost);
    m_port     = iniValue(QStringLiteral("port"), QString::number(DbDefaults::kPort)).toInt();
    m_database = iniValue(QStringLiteral("name"), DbDefaults::kDatabase);
    m_user     = iniValue(QStringLiteral("user"), DbDefaults::kUser);
    m_password = iniValue(QStringLiteral("password"), DbDefaults::kPassword);

    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QODBC"))) {
        m_lastError = QStringLiteral(
            "Qt 未加载 QODBC 驱动（plugins/sqldrivers 下缺少 qsqlodbc.dll）。\n"
            "可执行文件目录：%1").arg(QCoreApplication::applicationDirPath());
        if (errorText) *errorText = m_lastError;
        return false;
    }

    QString driver;
    QString driverError;
    if (!detectDriver(&driver, &driverError)) {
        m_lastError = driverError;
        if (errorText) *errorText = m_lastError;
        return false;
    }
    m_driver = driver;

    if (QSqlDatabase::contains(QLatin1String(kConnectionName))) {
        QSqlDatabase::removeDatabase(QLatin1String(kConnectionName));
    }

    QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QODBC"), QLatin1String(kConnectionName));
    const QString connStr =
        QStringLiteral("DRIVER={%1};SERVER=%2;PORT=%3;DATABASE=%4;UID=%5;PWD=%6;"
                       "OPTION=3;CHARSET=utf8mb4;")
            .arg(driver, m_host)
            .arg(m_port)
            .arg(m_database, m_user, m_password);
    database.setDatabaseName(connStr);
    database.setUserName(m_user);
    database.setPassword(m_password);
    database.setConnectOptions(QStringLiteral("SQL_ATTR_LOGIN_TIMEOUT=5;SQL_ATTR_CONNECTION_TIMEOUT=5"));

    if (!database.open()) {
        m_lastError = translateError(database.lastError());
        if (errorText) *errorText = m_lastError;
        return false;
    }

    // 连接后立刻验证字符集与库是否可用
    QSqlQuery probe(database);
    if (!probe.exec(QStringLiteral("SELECT 1")) || !probe.next()) {
        m_lastError = translateError(probe.lastError());
        database.close();
        if (errorText) *errorText = m_lastError;
        return false;
    }

    m_open = true;
    m_lastError.clear();
    qInfo() << "数据库连接成功，驱动：" << driver << "数据库：" << m_database;
    return true;
}

void DbManager::close()
{
    if (QSqlDatabase::contains(QLatin1String(kConnectionName))) {
        {
            QSqlDatabase database = QSqlDatabase::database(QLatin1String(kConnectionName), false);
            if (database.isOpen())
                database.close();
        }
        QSqlDatabase::removeDatabase(QLatin1String(kConnectionName));
    }
    m_open = false;
}

bool DbManager::isOpen() const
{
    if (!m_open)
        return false;
    if (!QSqlDatabase::contains(QLatin1String(kConnectionName)))
        return false;
    return QSqlDatabase::database(QLatin1String(kConnectionName), false).isOpen();
}

QSqlDatabase DbManager::db() const
{
    return QSqlDatabase::database(QLatin1String(kConnectionName), false);
}

QString DbManager::databaseInfo() const
{
    if (!m_open)
        return QStringLiteral("未连接");
    return QStringLiteral("%1@%2:%3/%4").arg(m_user, m_host).arg(m_port).arg(m_database);
}

bool DbManager::logEvent(const QString &userId, const QString &event, QString *errorText)
{
    if (!isOpen() || userId.isEmpty())
        return false;

    QSqlQuery query(db());
    query.prepare(QStringLiteral(
        "INSERT INTO History (ID, USER_ID, EVENT) VALUES (?, ?, ?)"));
    query.addBindValue(uuid());
    query.addBindValue(userId);
    query.addBindValue(event.left(200));
    if (!query.exec()) {
        const QString error = translateError(query.lastError());
        qWarning() << "写操作日志失败：" << error;
        if (errorText) *errorText = error;
        return false;
    }
    return true;
}

QString DbManager::translateError(const QSqlError &error)
{
    return translateError(error.text());
}

QString DbManager::translateError(const QString &raw)
{
    if (raw.isEmpty())
        return QStringLiteral("未知数据库错误。");

    if (raw.contains(QStringLiteral("Duplicate entry"), Qt::CaseInsensitive))
        return QStringLiteral("存在重复数据，保存失败。\n%1").arg(raw);
    if (raw.contains(QStringLiteral("foreign key constraint"), Qt::CaseInsensitive))
        return QStringLiteral("该记录被其他数据引用，无法完成操作。\n%1").arg(raw);
    if (raw.contains(QStringLiteral("Access denied"), Qt::CaseInsensitive))
        return QStringLiteral("数据库拒绝访问：账号或密码错误，请检查 dbconfig.ini。");
    if (raw.contains(QStringLiteral("Unknown database"), Qt::CaseInsensitive))
        return QStringLiteral("数据库不存在：请先执行 db/hospital.sql 建库。");
    if (raw.contains(QStringLiteral("Can't connect"), Qt::CaseInsensitive)
        || raw.contains(QStringLiteral("Connection failed"), Qt::CaseInsensitive)
        || raw.contains(QStringLiteral("Lost connection"), Qt::CaseInsensitive)) {
        return QStringLiteral("无法连接 MySQL 服务：请确认 MySQL80 服务已启动、3306 端口可访问。");
    }
    if (raw.contains(QStringLiteral("[MySQL][ODBC"), Qt::CaseInsensitive)
        && raw.contains(QStringLiteral("1045"))) {
        return QStringLiteral("数据库拒绝访问：账号或密码错误，请检查 dbconfig.ini。");
    }
    return raw;
}
