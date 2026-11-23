#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QString>

// 数据库访问单例：连接管理 + 错误翻译 + 操作日志
// 参考：https://stackoverflow.com/questions/1008019/c-singleton-design-pattern
class DbManager
{
public:
    static DbManager &instance();

    bool open(QString *errorText = nullptr);
    void close();
    bool isOpen() const;
    QSqlDatabase db() const;

    QString lastError() const { return m_lastError; }
    QString databaseInfo() const;

    // 写一条操作日志（与业务操作放在同一事务里提交）
    bool logEvent(const QString &userId, const QString &event, QString *errorText = nullptr);

    static QString sha256(const QString &text);
    static QString uuid();
    static QString translateError(const QSqlError &error);
    static QString translateError(const QString &raw);

private:
    DbManager() = default;
    DbManager(const DbManager &) = delete;
    DbManager &operator=(const DbManager &) = delete;

    bool detectDriver(QString *driver, QString *errorText) const;
    QString iniValue(const QString &key, const QString &fallback) const;
    void ensureIniFile() const;

    bool    m_open = false;
    QString m_lastError;
    QString m_driver;
    QString m_host;
    int     m_port = 3306;
    QString m_database;
    QString m_user;
    QString m_password;
};

#endif // DBMANAGER_H
