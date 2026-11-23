#include "patienteditdialog.h"
#include "dbmanager.h"

#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QVBoxLayout>

PatientEditDialog::PatientEditDialog(QWidget *parent)
    : QDialog(parent)
{
    buildUi();
    setInsertMode();
}

void PatientEditDialog::buildUi()
{
    setWindowTitle(QStringLiteral("编辑患者信息"));
    setModal(true);
    setMinimumWidth(440);

    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->setVerticalSpacing(10);
    form->setHorizontalSpacing(12);

    m_idEdit = new QLineEdit(this);
    m_idEdit->setReadOnly(true);
    m_idEdit->setStyleSheet(QStringLiteral("background:#f2f2f2;"));

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setMaxLength(20);
    m_nameEdit->setPlaceholderText(QStringLiteral("必填，最多 20 字"));

    m_idCardEdit = new QLineEdit(this);
    m_idCardEdit->setMaxLength(18);
    m_idCardEdit->setPlaceholderText(QStringLiteral("18 位身份证号（必填、唯一）"));

    m_sexCombo = new QComboBox(this);
    m_sexCombo->addItem(QStringLiteral("未知"), 0);
    m_sexCombo->addItem(QStringLiteral("男"), 1);
    m_sexCombo->addItem(QStringLiteral("女"), 2);

    m_dobEdit = new QDateEdit(this);
    m_dobEdit->setCalendarPopup(true);
    m_dobEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    m_dobEdit->setDateRange(QDate(1900, 1, 1), QDate::currentDate());
    m_dobEdit->setDate(QDate(2000, 1, 1));

    m_heightSpin = new QDoubleSpinBox(this);
    m_heightSpin->setRange(0, 250);
    m_heightSpin->setDecimals(1);
    m_heightSpin->setSuffix(QStringLiteral(" cm"));
    m_heightSpin->setSpecialValueText(QStringLiteral("未填写"));

    m_weightSpin = new QDoubleSpinBox(this);
    m_weightSpin->setRange(0, 300);
    m_weightSpin->setDecimals(1);
    m_weightSpin->setSuffix(QStringLiteral(" kg"));
    m_weightSpin->setSpecialValueText(QStringLiteral("未填写"));

    m_phoneEdit = new QLineEdit(this);
    m_phoneEdit->setMaxLength(11);
    m_phoneEdit->setPlaceholderText(QStringLiteral("11 位手机号，可留空"));

    form->addRow(QStringLiteral("ID："), m_idEdit);
    form->addRow(QStringLiteral("姓名："), m_nameEdit);
    form->addRow(QStringLiteral("身份证："), m_idCardEdit);
    form->addRow(QStringLiteral("性别："), m_sexCombo);
    form->addRow(QStringLiteral("出生日期："), m_dobEdit);
    form->addRow(QStringLiteral("身高："), m_heightSpin);
    form->addRow(QStringLiteral("体重："), m_weightSpin);
    form->addRow(QStringLiteral("手机号："), m_phoneEdit);

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

    connect(saveBtn, &QPushButton::clicked, this, &PatientEditDialog::save);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void PatientEditDialog::setInsertMode()
{
    m_insertMode = true;
    m_id.clear();
    m_idEdit->setText(QStringLiteral("保存时自动生成"));
    m_nameEdit->clear();
    m_idCardEdit->clear();
    m_sexCombo->setCurrentIndex(0);
    m_dobEdit->setDate(QDate(2000, 1, 1));
    m_heightSpin->setValue(0);
    m_weightSpin->setValue(0);
    m_phoneEdit->clear();
    m_message->clear();
    setWindowTitle(QStringLiteral("添加患者信息"));
    m_nameEdit->setFocus();
}

void PatientEditDialog::setUpdateRecord(const QSqlRecord &record)
{
    m_insertMode = false;
    m_id = record.value(QStringLiteral("ID")).toString();
    m_idEdit->setText(m_id);
    m_nameEdit->setText(record.value(QStringLiteral("NAME")).toString());
    m_idCardEdit->setText(record.value(QStringLiteral("ID_CARD")).toString());
    m_sexCombo->setCurrentIndex(m_sexCombo->findData(record.value(QStringLiteral("SEX")).toInt()));

    const QDate dob = record.value(QStringLiteral("DOB")).toDate();
    if (dob.isValid())
        m_dobEdit->setDate(dob);
    m_heightSpin->setValue(record.value(QStringLiteral("HEIGHT")).toDouble());
    m_weightSpin->setValue(record.value(QStringLiteral("WEIGHT")).toDouble());
    m_phoneEdit->setText(record.value(QStringLiteral("MOBILEPHONE")).toString());
    m_message->clear();
    setWindowTitle(QStringLiteral("修改患者信息"));
    m_nameEdit->setFocus();
    m_nameEdit->selectAll();
}

bool PatientEditDialog::idCardExists(const QString &idCard, const QString &excludeId,
                                     QString *existingName)
{
    QSqlQuery query(DbManager::instance().db());
    query.prepare(QStringLiteral(
        "SELECT NAME FROM Patient WHERE ID_CARD = ? AND ID <> ? LIMIT 1"));
    query.addBindValue(idCard);
    query.addBindValue(excludeId);
    if (!query.exec())
        return false;
    if (query.next()) {
        if (existingName)
            *existingName = query.value(0).toString();
        return true;
    }
    return false;
}

bool PatientEditDialog::validate(QString *error)
{
    const QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        m_nameEdit->setFocus();
        *error = QStringLiteral("姓名不能为空。");
        return false;
    }

    const QString idCard = m_idCardEdit->text().trimmed().toUpper();
    if (!QRegularExpression(QStringLiteral("^[0-9]{17}[0-9X]$")).match(idCard).hasMatch()) {
        m_idCardEdit->setFocus();
        *error = QStringLiteral("身份证号必须是 18 位：前 17 位为数字，末位为数字或 X。");
        return false;
    }

    const QString phone = m_phoneEdit->text().trimmed();
    if (!phone.isEmpty()
        && !QRegularExpression(QStringLiteral("^1[3-9][0-9]{9}$")).match(phone).hasMatch()) {
        m_phoneEdit->setFocus();
        *error = QStringLiteral("手机号格式不正确，应为 11 位且以 1 开头。");
        return false;
    }

    QString existing;
    if (idCardExists(idCard, m_insertMode ? QString() : m_id, &existing)) {
        m_idCardEdit->setFocus();
        m_idCardEdit->selectAll();
        *error = QStringLiteral("身份证号“%1”已存在（患者：%2），不能重复录入。")
                     .arg(idCard, existing);
        return false;
    }

    if (!m_dobEdit->date().isValid() || m_dobEdit->date() > QDate::currentDate()) {
        m_dobEdit->setFocus();
        *error = QStringLiteral("出生日期不能晚于今天。");
        return false;
    }
    return true;
}

void PatientEditDialog::save()
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

    const QString name   = m_nameEdit->text().trimmed();
    const QString idCard = m_idCardEdit->text().trimmed().toUpper();
    const int     sex    = m_sexCombo->currentData().toInt();
    const QDate   dob    = m_dobEdit->date();
    const QString phone  = m_phoneEdit->text().trimmed();

    int age = QDate::currentDate().year() - dob.year();
    if (QDate::currentDate() < dob.addYears(age))
        --age;
    if (age < 0)
        age = 0;

    const QVariant height = m_heightSpin->value() > 0.0
                                ? QVariant(m_heightSpin->value())
                                : QVariant();
    const QVariant weight = m_weightSpin->value() > 0.0
                                ? QVariant(m_weightSpin->value())
                                : QVariant();
    const QVariant phoneValue = phone.isEmpty() ? QVariant() : QVariant(phone);

    QSqlQuery query(DbManager::instance().db());
    if (m_insertMode) {
        m_id = DbManager::uuid();
        query.prepare(QStringLiteral(
            "INSERT INTO Patient "
            "(ID, ID_CARD, NAME, SEX, DOB, HEIGHT, WEIGHT, MOBILEPHONE, AGE) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"));
        query.addBindValue(m_id);
        query.addBindValue(idCard);
        query.addBindValue(name);
        query.addBindValue(sex);
        query.addBindValue(dob);
        query.addBindValue(height);
        query.addBindValue(weight);
        query.addBindValue(phoneValue);
        query.addBindValue(age);
    } else {
        query.prepare(QStringLiteral(
            "UPDATE Patient SET ID_CARD = ?, NAME = ?, SEX = ?, DOB = ?, "
            "HEIGHT = ?, WEIGHT = ?, MOBILEPHONE = ?, AGE = ? WHERE ID = ?"));
        query.addBindValue(idCard);
        query.addBindValue(name);
        query.addBindValue(sex);
        query.addBindValue(dob);
        query.addBindValue(height);
        query.addBindValue(weight);
        query.addBindValue(phoneValue);
        query.addBindValue(age);
        query.addBindValue(m_id);
    }

    if (!query.exec()) {
        m_message->setText(DbManager::translateError(query.lastError()));
        return;
    }
    if (!m_insertMode && query.numRowsAffected() <= 0) {
        m_message->setText(QStringLiteral("没有更新到任何记录，该患者可能已被删除。"));
        return;
    }

    m_savedId = m_id;
    m_savedName = name;
    accept();
}
