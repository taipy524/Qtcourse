#include "mainwindow.h"
#include "config.h"
#include "dbmanager.h"
#include "loginpage.h"
#include "registerdialog.h"
#include "welcomepage.h"
#include "patientpage.h"
#include "doctorpage.h"
#include "departmentpage.h"
#include "historypage.h"

#include <QCloseEvent>
#include <QLabel>
#include <QMessageBox>
#include <QStackedWidget>
#include <QStatusBar>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    buildUi();
    tryOpenDatabase();
}

MainWindow::~MainWindow() = default;

void MainWindow::buildUi()
{
    setWindowTitle(AppInfo::kAppTitle);
    resize(1120, 720);
    setMinimumSize(960, 620);

    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);

    m_stack = new QStackedWidget(central);
    layout->addWidget(m_stack);
    setCentralWidget(central);

    m_login = new LoginPage(this);
    m_welcome = new WelcomePage(this);
    m_stack->addWidget(m_login);
    m_stack->addWidget(m_welcome);
    m_stack->setCurrentWidget(m_login);

    connect(m_login, &LoginPage::loginSucceeded, this, &MainWindow::onLoginSucceeded);
    connect(m_login, &LoginPage::registerRequested, this, &MainWindow::onRegisterRequested);
    connect(m_login, &LoginPage::retryDatabaseRequested, this, &MainWindow::tryOpenDatabase);

    connect(m_welcome, &WelcomePage::patientsRequested, this, &MainWindow::showPatientPage);
    connect(m_welcome, &WelcomePage::doctorsRequested, this, &MainWindow::showDoctorPage);
    connect(m_welcome, &WelcomePage::departmentsRequested, this, &MainWindow::showDepartmentPage);
    connect(m_welcome, &WelcomePage::historyRequested, this, &MainWindow::showHistoryPage);
    connect(m_welcome, &WelcomePage::logoutRequested, this, &MainWindow::onLogout);

    m_statusDb = new QLabel(QStringLiteral("数据库：未连接"), this);
    m_statusUser = new QLabel(QStringLiteral("当前用户：-"), this);
    m_statusStudent = new QLabel(QStringLiteral("学号 %1  |  %2")
                                     .arg(AppInfo::kStudentId, AppInfo::kStudentName), this);
    statusBar()->addWidget(m_statusDb, 1);
    statusBar()->addPermanentWidget(m_statusUser);
    statusBar()->addPermanentWidget(m_statusStudent);
}

void MainWindow::ensurePages()
{
    if (m_patient)
        return;

    m_patient = new PatientPage(this);
    m_doctor = new DoctorPage(this);
    m_department = new DepartmentPage(this);
    m_history = new HistoryPage(this);

    m_stack->addWidget(m_patient);
    m_stack->addWidget(m_doctor);
    m_stack->addWidget(m_department);
    m_stack->addWidget(m_history);

    connect(m_patient, &PatientPage::backRequested, this, &MainWindow::showWelcome);
    connect(m_doctor, &DoctorPage::backRequested, this, &MainWindow::showWelcome);
    connect(m_department, &DepartmentPage::backRequested, this, &MainWindow::showWelcome);
    connect(m_history, &HistoryPage::backRequested, this, &MainWindow::showWelcome);

    applyUserToPages();
}

void MainWindow::applyUserToPages()
{
    if (!m_patient)
        return;
    m_patient->setCurrentUser(m_userId);
    m_doctor->setCurrentUser(m_userId);
    m_department->setCurrentUser(m_userId);
    m_history->setCurrentUser(m_userId);
}

void MainWindow::tryOpenDatabase()
{
    QString error;
    const bool ok = DbManager::instance().open(&error);

    if (ok) {
        ensurePages();
        m_statusDb->setText(QStringLiteral("数据库：%1").arg(DbManager::instance().databaseInfo()));
        m_statusDb->setStyleSheet(QStringLiteral("color:#2e7d32;"));
        m_login->setDatabaseState(true, QStringLiteral("数据库连接成功，可以登录。"));
        statusBar()->showMessage(QStringLiteral("数据库连接成功"), 3000);

        if (m_patient) {
            m_patient->refresh();
            m_doctor->refresh();
            m_department->refresh();
        }
    } else {
        m_statusDb->setText(QStringLiteral("数据库：未连接"));
        m_statusDb->setStyleSheet(QStringLiteral("color:#c62828;"));
        m_login->setDatabaseState(false, error);
        statusBar()->showMessage(QStringLiteral("数据库连接失败"), 5000);
    }
}

void MainWindow::onLoginSucceeded(const QString &userId, const QString &fullName)
{
    m_userId = userId;
    m_userName = fullName;

    ensurePages();
    applyUserToPages();

    m_welcome->setCurrentUser(fullName);
    m_statusUser->setText(QStringLiteral("当前用户：%1").arg(fullName));
    m_stack->setCurrentWidget(m_welcome);
    statusBar()->showMessage(QStringLiteral("欢迎 %1，请选择要管理的模块。").arg(fullName), 5000);
}

void MainWindow::onRegisterRequested()
{
    RegisterDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    m_login->showMessage(
        QStringLiteral("注册成功，请使用账号“%1”登录。").arg(dialog.createdLoginName()), false);
    m_login->setUserName(dialog.createdLoginName());
}

void MainWindow::onLogout()
{
    if (!m_userId.isEmpty())
        DbManager::instance().logEvent(m_userId, QStringLiteral("退出登录"));
    m_userId.clear();
    m_userName.clear();
    applyUserToPages();

    m_statusUser->setText(QStringLiteral("当前用户：-"));
    m_welcome->setCurrentUser(QStringLiteral("-"));
    m_login->reset();
    m_stack->setCurrentWidget(m_login);
}

void MainWindow::showWelcome()
{
    m_stack->setCurrentWidget(m_welcome);
    statusBar()->showMessage(QStringLiteral("已返回欢迎页"), 2000);
}

void MainWindow::showPatientPage()
{
    if (!m_patient)
        return;
    m_patient->refresh();
    m_stack->setCurrentWidget(m_patient);
    statusBar()->showMessage(QStringLiteral("患者管理：支持查找、添加、修改、删除"), 3000);
}

void MainWindow::showDoctorPage()
{
    if (!m_doctor)
        return;
    m_doctor->refresh();
    m_stack->setCurrentWidget(m_doctor);
    statusBar()->showMessage(QStringLiteral("医生管理：科室信息通过外键关联显示"), 3000);
}

void MainWindow::showDepartmentPage()
{
    if (!m_department)
        return;
    m_department->refresh();
    m_stack->setCurrentWidget(m_department);
    statusBar()->showMessage(QStringLiteral("科室管理：被医生引用的科室不可删除"), 3000);
}

void MainWindow::showHistoryPage()
{
    if (!m_history)
        return;
    m_history->refresh();
    m_stack->setCurrentWidget(m_history);
    statusBar()->showMessage(QStringLiteral("操作日志：记录登录与增删改查"), 3000);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!m_userId.isEmpty())
        DbManager::instance().logEvent(m_userId, QStringLiteral("关闭程序"));
    DbManager::instance().close();
    QMainWindow::closeEvent(event);
}
