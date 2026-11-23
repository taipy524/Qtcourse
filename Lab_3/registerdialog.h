#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>

class QLineEdit;
class QLabel;

class RegisterDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RegisterDialog(QWidget *parent = nullptr);

    QString createdUserId() const { return m_createdUserId; }
    QString createdUserName() const { return m_createdUserName; }
    QString createdLoginName() const { return m_createdLoginName; }

private slots:
    void doRegister();

private:
    QLineEdit *m_username = nullptr;
    QLineEdit *m_fullname = nullptr;
    QLineEdit *m_password = nullptr;
    QLineEdit *m_confirm = nullptr;
    QLabel    *m_message = nullptr;

    QString m_createdUserId;
    QString m_createdUserName;
    QString m_createdLoginName;
};

#endif // REGISTERDIALOG_H
