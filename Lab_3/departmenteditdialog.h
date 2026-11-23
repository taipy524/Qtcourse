#ifndef DEPARTMENTEDITDIALOG_H
#define DEPARTMENTEDITDIALOG_H

#include <QDialog>

class QLabel;
class QLineEdit;

class DepartmentEditDialog : public QDialog
{
    Q_OBJECT
public:
    explicit DepartmentEditDialog(QWidget *parent = nullptr);

    void setInsertMode();
    void setUpdateRecord(const QString &id, const QString &name);
    void setTakenNames(const QStringList &names) { m_takenNames = names; }

    QString savedId() const { return m_savedId; }
    QString savedName() const { return m_savedName; }

private slots:
    void save();

private:
    QLineEdit *m_idEdit = nullptr;
    QLineEdit *m_nameEdit = nullptr;
    QLabel    *m_message = nullptr;

    QString m_id;
    QString m_savedId;
    QString m_savedName;
    bool    m_insertMode = true;
    QStringList m_takenNames;
};

#endif // DEPARTMENTEDITDIALOG_H
