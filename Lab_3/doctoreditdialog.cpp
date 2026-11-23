#include "doctoreditdialog.h"
#include "dbmanager.h"

#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSqlError>
#include <QSqlQuery>
#include <QVBoxLayout>

DoctorEditDialog::DoctorEditDialog(QWidget *parent)
    : QDialog(parent)
{
    buildUi();
    setInsertMode();
}

void DoctorEditDialog::buildUi()
{
    setWindowTitle(QStringLiteral("编辑医生信息"));
    setModal(true);
    setMinimumWidth(420);

    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->setVerticalSpacing(10);

    m_idEdit = new QLineEdit(this);
    m_idEdit->setReadOnly(true);
    m_idEdit->setStyleSheet(QStringLiteral("background:#f2f2f2;"));

    m_empNoEdit = new QLineEdit(this);
    m_empNoEdit->setMaxLength(20);
    m_empNoEdit->setPlaceholderText(QStringLiteral("工号（唯一）"));

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setMaxLength(20);
    m_nameEdit->setPlaceholderText(QStringLiteral("医生姓名"));

    m_deptCombo = new QComboBox(this);

    form->addRow(QStringLiteral("ID："), m_idEdit);
    form->addRow(QStringLiteral("工号："), m_empNoEdit);
    form->addRow(QStringLiteral("姓名："), m_nameEdit);
    form->addRow(QStringLiteral("科室："), m_deptCombo);

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

    connect(saveBtn, &QPushButton::clicked, this, &DoctorEditDialog::save);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void DoctorEditDialog::loadDepartments()
{
    m_deptCombo->clear();
    QSqlQuery query(DbManager::instance().db());
    if (!query.exec(QStringLiteral("SELECT ID, NAME FROM Department ORDER BY ID")))
        return;
    while (query.next())
        m_deptCombo->addItem(query.value(1).toString(), query.value(0).toString());
}

void DoctorEditDialog::setInsertMode()
{
    m_insertMode = true;
    m_id.clear();
    m_idEdit->setText(QStringLiteral("保存时自动生成"));
    m_empNoEdit->clear();
    m_nameEdit->clear();
    loadDepartments();
    m_message->clear();
    setWindowTitle(QStringLiteral("添加医生"));
    m_empNoEdit->setFocus();
}

void DoctorEditDialog::setUpdateRecord(const QSqlRecord &record)
{
    m_insertMode = false;
    m_id = record.value(QStringLiteral("ID")).toString();
    m_idEdit->setText(m_id);
    m_empNoEdit->setText(record.value(QStringLiteral("EMPLOYEE_NO")).toString());
    m_nameEdit->setText(record.value(QStringLiteral("NAME")).toString());
    loadDepartments();
    const int index = m_deptCombo->findData(record.value(QStringLiteral("DEPARTMENT_ID")).toString());
    if (index >= 0)
        m_deptCombo->setCurrentIndex(index);
    m_message->clear();
    setWindowTitle(QStringLiteral("修改医生信息"));
    m_nameEdit->setFocus();
    m_nameEdit->selectAll();
}

bool DoctorEditDialog::validate(QString *error)
{
    const QString empNo = m_empNoEdit->text().trimmed();
    if (empNo.isEmpty()) {
        m_empNoEdit->setFocus();
        *error = QStringLiteral("工号不能为空。");
        return false;
    }

    const QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        m_nameEdit->setFocus();
        *error = QStringLiteral("姓名不能为空。");
        return false;
    }

    if (!m_deptCombo->currentData().isValid()) {
        *error = QStringLiteral("请先维护科室信息，再添加医生。");
        return false;
    }

    QSqlQuery check(DbManager::instance().db());
    check.prepare(QStringLiteral(
        "SELECT NAME FROM Doctor WHERE EMPLOYEE_NO = ? AND ID <> ? LIMIT 1"));
    check.addBindValue(empNo);
    check.addBindValue(m_insertMode ? QString() : m_id);
    if (check.exec() && check.next()) {
        m_empNoEdit->setFocus();
        m_empNoEdit->selectAll();
        *error = QStringLiteral("工号“%1”已被医生 %2 使用。")
                     .arg(empNo, check.value(0).toString());
        return false;
    }
    return true;
}

void DoctorEditDialog::save()
{
    if (!DbManager::instance().isOpen()) {
        m_message->setText(QStringLiteral("数据库未连接，无法保存。"));
        return;
    }

    QString error;
    if (!validate(&error)) {
        m_message->setText(error);
        return;
    }

    const QString empNo = m_empNoEdit->text().trimmed();
    const QString name  = m_nameEdit->text().trimmed();
    const QString dept  = m_deptCombo->currentData().toString();

    QSqlQuery query(DbManager::instance().db());
    if (m_insertMode) {
        m_id = DbManager::uuid();
        query.prepare(QStringLiteral(
            "INSERT INTO Doctor (ID, EMPLOYEE_NO, NAME, DEPARTMENT_ID) VALUES (?, ?, ?, ?)"));
        query.addBindValue(m_id);
        query.addBindValue(empNo);
        query.addBindValue(name);
        query.addBindValue(dept);
    } else {
        query.prepare(QStringLiteral(
            "UPDATE Doctor SET EMPLOYEE_NO = ?, NAME = ?, DEPARTMENT_ID = ? WHERE ID = ?"));
        query.addBindValue(empNo);
        query.addBindValue(name);
        query.addBindValue(dept);
        query.addBindValue(m_id);
    }

    if (!query.exec()) {
        m_message->setText(DbManager::translateError(query.lastError()));
        return;
    }
    if (!m_insertMode && query.numRowsAffected() <= 0) {
        m_message->setText(QStringLiteral("没有更新到任何记录，该医生可能已被删除。"));
        return;
    }

    m_savedId = m_id;
    m_savedName = name;
    accept();
}
