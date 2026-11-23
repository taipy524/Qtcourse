#include "registerdialog.h"
#include "dbmanager.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QVBoxLayout>

RegisterDialog::RegisterDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("注册新用户"));
    setModal(true);
    setMinimumWidth(420);

    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->setVerticalSpacing(10);

    m_username = new QLineEdit(this);
    m_username->setPlaceholderText(QStringLiteral("3-20 位字母或数字"));
    m_fullname = new QLineEdit(this);
    m_fullname->setPlaceholderText(QStringLiteral("真实姓名"));
    m_password = new QLineEdit(this);
    m_password->setEchoMode(QLineEdit::Password);
    m_password->setPlaceholderText(QStringLiteral("至少 6 位"));
    m_confirm = new QLineEdit(this);
    m_confirm->setEchoMode(QLineEdit::Password);
    m_confirm->setPlaceholderText(QStringLiteral("再次输入密码"));

    form->addRow(QStringLiteral("用户名："), m_username);
    form->addRow(QStringLiteral("姓  名："), m_fullname);
    form->addRow(QStringLiteral("密  码："), m_password);
    form->addRow(QStringLiteral("确认密码："), m_confirm);

    m_message = new QLabel(this);
    m_message->setWordWrap(true);
    m_message->setMinimumHeight(22);
    m_message->setStyleSheet(QStringLiteral("color:#c62828;"));

    auto *saveBtn = new QPushButton(QStringLiteral("注 册"), this);
    auto *cancelBtn = new QPushButton(QStringLiteral("取 消"), this);
    saveBtn->setDefault(true);

    auto *buttons = new QHBoxLayout;
    buttons->addStretch(1);
    buttons->addWidget(cancelBtn);
    buttons->addWidget(saveBtn);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(m_message);
    layout->addLayout(buttons);

    connect(saveBtn, &QPushButton::clicked, this, &RegisterDialog::doRegister);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_confirm, &QLineEdit::returnPressed, this, &RegisterDialog::doRegister);
}

void RegisterDialog::doRegister()
{
    const QString username = m_username->text().trimmed();
    const QString fullname = m_fullname->text().trimmed();
    const QString password = m_password->text();
    const QString confirm  = m_confirm->text();

    auto fail = [this](const QString &text) {
        m_message->setText(text);
    };

    if (username.isEmpty() || fullname.isEmpty() || password.isEmpty()) {
        fail(QStringLiteral("用户名、姓名、密码均为必填项。"));
        return;
    }
    if (!QRegularExpression(QStringLiteral("^[A-Za-z0-9_]{3,20}$")).match(username).hasMatch()) {
        fail(QStringLiteral("用户名只能包含 3-20 位字母、数字或下划线。"));
        return;
    }
    if (fullname.size() > 20) {
        fail(QStringLiteral("姓名不能超过 20 个字符。"));
        return;
    }
    if (password.size() < 6) {
        fail(QStringLiteral("密码至少 6 位。"));
        return;
    }
    if (password != confirm) {
        fail(QStringLiteral("两次输入的密码不一致。"));
        return;
    }

    QSqlQuery check(DbManager::instance().db());
    check.prepare(QStringLiteral("SELECT COUNT(*) FROM `User` WHERE USERNAME = ?"));
    check.addBindValue(username);
    if (!check.exec() || !check.next()) {
        fail(DbManager::translateError(check.lastError()));
        return;
    }
    if (check.value(0).toInt() > 0) {
        fail(QStringLiteral("用户名“%1”已存在，请换一个。").arg(username));
        return;
    }

    m_createdUserId = DbManager::uuid();
    m_createdUserName = fullname;
    m_createdLoginName = username;

    QSqlQuery insert(DbManager::instance().db());
    insert.prepare(QStringLiteral(
        "INSERT INTO `User` (ID, FULLNAME, USERNAME, PASSWORD) VALUES (?, ?, ?, ?)"));
    insert.addBindValue(m_createdUserId);
    insert.addBindValue(fullname);
    insert.addBindValue(username);
    insert.addBindValue(DbManager::sha256(password));
    if (!insert.exec()) {
        fail(DbManager::translateError(insert.lastError()));
        return;
    }

    DbManager::instance().logEvent(m_createdUserId,
                                   QStringLiteral("注册账号 %1（%2）").arg(username, fullname));
    accept();
}
