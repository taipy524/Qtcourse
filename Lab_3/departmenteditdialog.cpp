#include "departmenteditdialog.h"
#include "dbmanager.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSqlError>
#include <QSqlQuery>
#include <QVBoxLayout>

DepartmentEditDialog::DepartmentEditDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("编辑科室"));
    setModal(true);
    setMinimumWidth(380);

    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->setVerticalSpacing(10);

    m_idEdit = new QLineEdit(this);
    m_idEdit->setReadOnly(true);
    m_idEdit->setStyleSheet(QStringLiteral("background:#f2f2f2;"));

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setMaxLength(30);
    m_nameEdit->setPlaceholderText(QStringLiteral("例如：内科、外科"));

    form->addRow(QStringLiteral("科室编号："), m_idEdit);
    form->addRow(QStringLiteral("科室名称："), m_nameEdit);

    m_message = new QLabel(this);
    m_message->setStyleSheet(QStringLiteral("color:#c62828;"));
    m_message->setWordWrap(true);
    m_message->setMinimumHeight(22);

    auto *saveBtn = new QPushButton(QStringLiteral("保 存"), this);
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

    connect(saveBtn, &QPushButton::clicked, this, &DepartmentEditDialog::save);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    setInsertMode();
}

void DepartmentEditDialog::setInsertMode()
{
    m_insertMode = true;
    m_id.clear();
    m_idEdit->setText(QStringLiteral("保存时自动生成"));
    m_nameEdit->clear();
    m_message->clear();
    setWindowTitle(QStringLiteral("添加科室"));
    m_nameEdit->setFocus();
}

void DepartmentEditDialog::setUpdateRecord(const QString &id, const QString &name)
{
    m_insertMode = false;
    m_id = id;
    m_idEdit->setText(id);
    m_nameEdit->setText(name);
    m_message->clear();
    setWindowTitle(QStringLiteral("修改科室"));
    m_nameEdit->setFocus();
    m_nameEdit->selectAll();
}

void DepartmentEditDialog::save()
{
    if (!DbManager::instance().isOpen()) {
        m_message->setText(QStringLiteral("数据库未连接，无法保存。"));
        return;
    }

    const QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        m_nameEdit->setFocus();
        m_message->setText(QStringLiteral("科室名称不能为空。"));
        return;
    }
    if (m_takenNames.contains(name, Qt::CaseInsensitive)) {
        m_nameEdit->setFocus();
        m_message->setText(QStringLiteral("科室“%1”已存在，不能重复添加。").arg(name));
        return;
    }

    QSqlQuery query(DbManager::instance().db());
    if (m_insertMode) {
        // 科室编号使用可读编码 DEP01、DEP02…，与医生表的外键类型保持一致
        QSqlQuery maxQuery(DbManager::instance().db());
        int next = 1;
        if (maxQuery.exec(QStringLiteral("SELECT ID FROM Department ORDER BY ID DESC LIMIT 1"))
            && maxQuery.next()) {
            const QString last = maxQuery.value(0).toString();
            bool ok = false;
            const int number = last.mid(3).toInt(&ok);
            if (ok)
                next = number + 1;
        }
        m_id = QStringLiteral("DEP%1").arg(next, 2, 10, QLatin1Char('0'));

        query.prepare(QStringLiteral("INSERT INTO Department (ID, NAME) VALUES (?, ?)"));
        query.addBindValue(m_id);
        query.addBindValue(name);
    } else {
        query.prepare(QStringLiteral("UPDATE Department SET NAME = ? WHERE ID = ?"));
        query.addBindValue(name);
        query.addBindValue(m_id);
    }

    if (!query.exec()) {
        m_message->setText(DbManager::translateError(query.lastError()));
        return;
    }
    if (!m_insertMode && query.numRowsAffected() <= 0) {
        m_message->setText(QStringLiteral("没有更新到任何记录，该科室可能已被删除。"));
        return;
    }

    m_savedId = m_id;
    m_savedName = name;
    accept();
}
