#include "loginpage.h"
#include "dbmanager.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSqlError>
#include <QSqlQuery>
#include <QVBoxLayout>

LoginPage::LoginPage(QWidget *parent)
    : QWidget(parent)
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(40, 30, 40, 30);

    auto *title = new QLabel(QStringLiteral("欢迎使用诊疗测试系统"), this);
    QFont titleFont = title->font();
    titleFont.setPointSize(20);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);

    auto *subTitle = new QLabel(QStringLiteral("请使用系统账号登录"), this);
    subTitle->setAlignment(Qt::AlignCenter);
    subTitle->setStyleSheet(QStringLiteral("color:#666;"));

    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->setHorizontalSpacing(14);
    form->setVerticalSpacing(12);

    m_userEdit = new QLineEdit(this);
    m_userEdit->setPlaceholderText(QStringLiteral("请输入用户名"));
    m_userEdit->setMinimumWidth(240);

    m_pwdEdit = new QLineEdit(this);
    m_pwdEdit->setPlaceholderText(QStringLiteral("请输入密码"));
    m_pwdEdit->setEchoMode(QLineEdit::Password);

    form->addRow(QStringLiteral("用户名："), m_userEdit);
    form->addRow(QStringLiteral("密  码："), m_pwdEdit);

    m_loginBtn = new QPushButton(QStringLiteral("登  录"), this);
    m_registerBtn = new QPushButton(QStringLiteral("注  册"), this);
    m_loginBtn->setMinimumHeight(32);
    m_registerBtn->setMinimumHeight(32);
    m_loginBtn->setDefault(true);

    auto *buttons = new QHBoxLayout;
    buttons->addWidget(m_registerBtn);
    buttons->addWidget(m_loginBtn);

    m_retryBtn = new QPushButton(QStringLiteral("重试连接数据库"), this);
    m_retryBtn->setVisible(false);

    m_message = new QLabel(this);
    m_message->setAlignment(Qt::AlignCenter);
    m_message->setWordWrap(true);
    m_message->setMinimumHeight(24);

    m_dbState = new QLabel(this);
    m_dbState->setAlignment(Qt::AlignCenter);
    m_dbState->setStyleSheet(QStringLiteral("color:#666;"));

    auto *hint = new QLabel(QStringLiteral("演示账号：admin / 123456    医生账号：doctor / doctor123"), this);
    hint->setAlignment(Qt::AlignCenter);
    hint->setStyleSheet(QStringLiteral("color:#999;font-size:12px;"));

    auto *panel = new QWidget(this);
    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setSpacing(14);
    panelLayout->addWidget(title);
    panelLayout->addWidget(subTitle);
    panelLayout->addSpacing(18);
    panelLayout->addLayout(form);
    panelLayout->addLayout(buttons);
    panelLayout->addWidget(m_retryBtn);
    panelLayout->addWidget(m_message);
    panelLayout->addWidget(m_dbState);
    panelLayout->addSpacing(6);
    panelLayout->addWidget(hint);
    panel->setMaximumWidth(460);

    outer->addStretch(1);
    outer->addWidget(panel, 0, Qt::AlignHCenter);
    outer->addStretch(2);

    connect(m_loginBtn, &QPushButton::clicked, this, &LoginPage::doLogin);
    connect(m_pwdEdit, &QLineEdit::returnPressed, this, &LoginPage::doLogin);
    connect(m_userEdit, &QLineEdit::returnPressed, this, &LoginPage::doLogin);
    connect(m_registerBtn, &QPushButton::clicked, this, &LoginPage::registerRequested);
    connect(m_retryBtn, &QPushButton::clicked, this, &LoginPage::retryDatabaseRequested);

    setDatabaseState(false, QStringLiteral("正在连接数据库…"));
}

void LoginPage::setDatabaseState(bool ready, const QString &message)
{
    m_dbReady = ready;
    m_loginBtn->setEnabled(ready);
    m_registerBtn->setEnabled(ready);
    m_retryBtn->setVisible(!ready);
    m_dbState->setText(ready ? QStringLiteral("● 数据库已连接")
                             : QStringLiteral("● 数据库未连接"));
    m_dbState->setStyleSheet(ready ? QStringLiteral("color:#2e7d32;")
                                   : QStringLiteral("color:#c62828;"));
    if (!ready && !message.isEmpty()) {
        showMessage(message, true);
    } else if (ready) {
        showMessage(message.isEmpty() ? QStringLiteral("数据库连接成功，可以登录。")
                                      : message, false);
    }
}

void LoginPage::showMessage(const QString &text, bool error)
{
    m_message->setText(text);
    m_message->setStyleSheet(error ? QStringLiteral("color:#c62828;")
                                   : QStringLiteral("color:#2e7d32;"));
}

void LoginPage::reset()
{
    m_pwdEdit->clear();
    m_userEdit->setFocus();
    showMessage(QStringLiteral("已退出登录。"), false);
}

void LoginPage::setUserName(const QString &username)
{
    m_userEdit->setText(username);
    m_pwdEdit->clear();
    m_pwdEdit->setFocus();
}

void LoginPage::doLogin()
{
    if (!m_dbReady) {
        showMessage(QStringLiteral("数据库未连接，无法登录。"), true);
        return;
    }

    const QString username = m_userEdit->text().trimmed();
    const QString password = m_pwdEdit->text();
    if (username.isEmpty() || password.isEmpty()) {
        showMessage(QStringLiteral("用户名和密码不能为空。"), true);
        return;
    }

    QSqlQuery query(DbManager::instance().db());
    query.prepare(QStringLiteral(
        "SELECT ID, FULLNAME, PASSWORD FROM `User` WHERE USERNAME = ?"));
    query.addBindValue(username);
    if (!query.exec()) {
        showMessage(DbManager::translateError(query.lastError()), true);
        return;
    }

    if (!query.next()) {
        showMessage(QStringLiteral("用户名不存在，请检查后重试。"), true);
        return;
    }

    const QString userId   = query.value(0).toString();
    const QString fullName = query.value(1).toString();
    const QString stored   = query.value(2).toString();
    if (stored.compare(DbManager::sha256(password), Qt::CaseInsensitive) != 0) {
        showMessage(QStringLiteral("密码错误，请重新输入。"), true);
        m_pwdEdit->selectAll();
        m_pwdEdit->setFocus();
        return;
    }

    DbManager::instance().logEvent(userId, QStringLiteral("登录系统"));
    showMessage(QStringLiteral("登录成功，欢迎 %1！").arg(fullName), false);
    emit loginSucceeded(userId, fullName);
}
