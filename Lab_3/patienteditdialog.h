#ifndef PATIENTEDITDIALOG_H
#define PATIENTEDITDIALOG_H

#include <QDialog>
#include <QSqlRecord>

class QComboBox;
class QDateEdit;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;

class PatientEditDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PatientEditDialog(QWidget *parent = nullptr);

    void setInsertMode();
    void setUpdateRecord(const QSqlRecord &record);

    QString savedId() const { return m_savedId; }
    QString savedName() const { return m_savedName; }

private slots:
    void save();

private:
    void buildUi();
    bool validate(QString *error);
    bool idCardExists(const QString &idCard, const QString &excludeId, QString *existingName);

    QLineEdit      *m_idEdit = nullptr;
    QLineEdit      *m_nameEdit = nullptr;
    QLineEdit      *m_idCardEdit = nullptr;
    QComboBox      *m_sexCombo = nullptr;
    QDateEdit      *m_dobEdit = nullptr;
    QDoubleSpinBox *m_heightSpin = nullptr;
    QDoubleSpinBox *m_weightSpin = nullptr;
    QLineEdit      *m_phoneEdit = nullptr;
    QLabel         *m_message = nullptr;

    QString m_id;
    QString m_savedId;
    QString m_savedName;
    bool    m_insertMode = true;
};

#endif // PATIENTEDITDIALOG_H
