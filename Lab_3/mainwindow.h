#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QLabel;
class QStackedWidget;
class LoginPage;
class WelcomePage;
class PatientPage;
class DoctorPage;
class DepartmentPage;
class HistoryPage;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void tryOpenDatabase();
    void onLoginSucceeded(const QString &userId, const QString &fullName);
    void onRegisterRequested();
    void onLogout();
    void showWelcome();
    void showPatientPage();
    void showDoctorPage();
    void showDepartmentPage();
    void showHistoryPage();

private:
    void buildUi();
    void ensurePages();
    void applyUserToPages();

    QStackedWidget *m_stack = nullptr;
    LoginPage      *m_login = nullptr;
    WelcomePage    *m_welcome = nullptr;
    PatientPage    *m_patient = nullptr;
    DoctorPage     *m_doctor = nullptr;
    DepartmentPage *m_department = nullptr;
    HistoryPage    *m_history = nullptr;

    QLabel *m_statusDb = nullptr;
    QLabel *m_statusUser = nullptr;
    QLabel *m_statusStudent = nullptr;

    QString m_userId;
    QString m_userName;
};

#endif // MAINWINDOW_H
