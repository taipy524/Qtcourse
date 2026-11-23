#ifndef DOCTOREDITDIALOG_H
#define DOCTOREDITDIALOG_H

#include <QDialog>
#include <QSqlRecord>

class QComboBox;
class QLabel;
class QLineEdit;

class DoctorEditDialog : public QDialog
{
    Q_OBJECT
public:
    explicit DoctorEditDialog(QWidget *parent = nullptr);

    void setInsertMode();
    void setUpdateRecord(const QSqlRecord &record);

    QString savedId() const { return m_savedId; }
    QString savedName() const { return m_savedName; }

private slots:
    void save();

private:
    void buildUi();
    void loadDepartments();
    bool validate(QString *error);

    QLineEdit *m_idEdit = nullptr;
    QLineEdit *m_empNoEdit = nullptr;
    QLineEdit *m_nameEdit = nullptr;
    QComboBox *m_deptCombo = nullptr;
    QLabel    *m_message = nullptr;

    QString m_id;
    QString m_savedId;
    QString m_savedName;
    bool    m_insertMode = true;
};

#endif // DOCTOREDITDIALOG_H
