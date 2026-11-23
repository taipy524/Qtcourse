#include "welcomepage.h"
#include "config.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

WelcomePage::WelcomePage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 24, 40, 20);
    layout->setSpacing(18);

    auto *topBar = new QHBoxLayout;
    auto *title = new QLabel(QStringLiteral("医院诊疗系统"), this);
    QFont titleFont = title->font();
    titleFont.setPointSize(22);
    titleFont.setBold(true);
    title->setFont(titleFont);

    m_userLabel = new QLabel(QStringLiteral("未登录"), this);
    m_userLabel->setStyleSheet(QStringLiteral("color:#555;"));

    auto *logoutBtn = new QPushButton(QStringLiteral("退出登录"), this);
    connect(logoutBtn, &QPushButton::clicked, this, &WelcomePage::logoutRequested);

    topBar->addWidget(title);
    topBar->addStretch(1);
    topBar->addWidget(m_userLabel);
    topBar->addSpacing(8);
    topBar->addWidget(logoutBtn);
    layout->addLayout(topBar);

    auto *hint = new QLabel(QStringLiteral("请选择要管理的模块"), this);
    hint->setStyleSheet(QStringLiteral("color:#666;"));
    layout->addWidget(hint);

    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(22);
    grid->setVerticalSpacing(18);

    auto *patientBtn = createEntryButton(QStringLiteral("患者管理"),
                                         QStringLiteral("信息查询 / 新增 / 修改 / 删除"),
                                         QStringLiteral("#1e88e5"),
                                         QStringLiteral(":/icons/patient.png"));
    auto *doctorBtn = createEntryButton(QStringLiteral("医生管理"),
                                        QStringLiteral("医生档案与所属科室"),
                                        QStringLiteral("#43a047"),
                                        QStringLiteral(":/icons/doctor.png"));
    auto *deptBtn = createEntryButton(QStringLiteral("科室管理"),
                                      QStringLiteral("科室维护（被引用不可删）"),
                                      QStringLiteral("#fb8c00"),
                                      QStringLiteral(":/icons/department.png"));
    auto *historyBtn = createEntryButton(QStringLiteral("操作日志"),
                                         QStringLiteral("登录与增删改查审计"),
                                         QStringLiteral("#8e24aa"),
                                         QStringLiteral(":/icons/history.png"));

    grid->addWidget(patientBtn, 0, 0);
    grid->addWidget(doctorBtn, 0, 1);
    grid->addWidget(deptBtn, 1, 0);
    grid->addWidget(historyBtn, 1, 1);
    layout->addLayout(grid);
    layout->addStretch(1);

    auto *footer = new QLabel(QStringLiteral("开发者：学号 %1    姓名 %2")
                                  .arg(AppInfo::kStudentId, AppInfo::kStudentName), this);
    footer->setAlignment(Qt::AlignCenter);
    footer->setStyleSheet(QStringLiteral("color:#777;padding-top:6px;"));
    layout->addWidget(footer);

    connect(patientBtn, &QPushButton::clicked, this, &WelcomePage::patientsRequested);
    connect(doctorBtn, &QPushButton::clicked, this, &WelcomePage::doctorsRequested);
    connect(deptBtn, &QPushButton::clicked, this, &WelcomePage::departmentsRequested);
    connect(historyBtn, &QPushButton::clicked, this, &WelcomePage::historyRequested);
}

QPushButton *WelcomePage::createEntryButton(const QString &text, const QString &description,
                                            const QString &color, const QString &iconPath)
{
    auto *button = new QPushButton(this);
    button->setMinimumSize(230, 130);
    button->setCursor(Qt::PointingHandCursor);
    button->setIcon(QIcon(iconPath));
    button->setIconSize(QSize(40, 40));
    button->setText(QStringLiteral("%1\n%2").arg(text, description));
    button->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background:%1; color:white; border:none; border-radius:10px;"
        "  font-size:17px; font-weight:bold; padding:14px;"
        "}"
        "QPushButton:hover { background:%2; }"
        "QPushButton:pressed { background:%3; }")
        .arg(color, QColor(color).lighter(112).name(), QColor(color).darker(112).name()));
    return button;
}

void WelcomePage::setCurrentUser(const QString &fullName)
{
    m_userLabel->setText(QStringLiteral("当前用户：%1").arg(fullName));
}
