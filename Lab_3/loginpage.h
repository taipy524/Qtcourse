#ifndef LOGINPAGE_H
#define LOGINPAGE_H

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;

class LoginPage : public QWidget
{
    Q_OBJECT
public:
    explicit LoginPage(QWidget *parent = nullptr);

    void setDatabaseState(bool ready, const QString &message);
    void showMessage(const QString &text, bool error = true);
    void reset();
    void setUserName(const QString &username);

signals:
    void loginSucceeded(const QString &userId, const QString &fullName);
    void registerRequested();
    void retryDatabaseRequested();

private slots:
    void doLogin();

private:
    QLineEdit    *m_userEdit = nullptr;
    QLineEdit    *m_pwdEdit = nullptr;
    QPushButton  *m_loginBtn = nullptr;
    QPushButton  *m_registerBtn = nullptr;
    QPushButton  *m_retryBtn = nullptr;
    QLabel       *m_message = nullptr;
    QLabel       *m_dbState = nullptr;
    bool          m_dbReady = false;
};

#endif // LOGINPAGE_H
