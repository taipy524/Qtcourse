#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_first(0.0)
    , m_waiting(false)
    , m_calculated(false)
{
    ui->setupUi(this);

    m_op.clear();
    setupConnections();
    showText(QStringLiteral("0"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

/**
 * @brief 把所有输入控件统一接到核心业务函数上
 *
 * 鼠标点击按钮 → 核心函数；后面实现的键盘事件 → 同一批核心函数。
 * 槽函数里不写任何业务逻辑，避免两套计算代码。
 */
void MainWindow::setupConnections()
{
    struct DigitBinding {
        QPushButton *button;
        const char *digit;
    };
    const DigitBinding digits[] = {
        {ui->btn0, "0"}, {ui->btn1, "1"}, {ui->btn2, "2"},
        {ui->btn3, "3"}, {ui->btn4, "4"}, {ui->btn5, "5"},
        {ui->btn6, "6"}, {ui->btn7, "7"}, {ui->btn8, "8"},
        {ui->btn9, "9"},
    };

    for (const DigitBinding &item : digits) {
        const QString digit = QString::fromLatin1(item.digit);
        connect(item.button, &QPushButton::clicked, this, [this, digit]() {
            inputDigit(digit);
        });
    }

    connect(ui->btnDot, &QPushButton::clicked, this, &MainWindow::inputDot);
}

void MainWindow::showText(const QString &text)
{
    ui->display->setText(text);
}

QString MainWindow::displayText() const
{
    return ui->display->text();
}

/**
 * @brief 数字输入：0~9 依次拼接
 *
 * 显示 0 时输入 5 得到 5（不是 05）；
 * 刚按过运算符或刚开始新输入时，重新起一个操作数。
 */
void MainWindow::inputDigit(const QString &digit)
{
    QString text = displayText();

    if (m_waiting || m_calculated) {   // 开始输入新的操作数
        text = QStringLiteral("0");
        m_waiting = false;
        m_calculated = false;
    }

    if (text == QStringLiteral("0"))
        text.clear();

    text += digit;
    showText(text);
}

/**
 * @brief 小数点输入：同一个操作数最多一个点，1.2.3 的第二个点被忽略
 */
void MainWindow::inputDot()
{
    QString text = displayText();

    if (m_waiting || m_calculated) {   // 新操作数以 0. 起头
        text = QStringLiteral("0");
        m_waiting = false;
        m_calculated = false;
    }

    if (text.contains(QLatin1Char('.')))
        return;                        // 已有小数点，直接忽略

    if (text.isEmpty())
        text = QStringLiteral("0");

    text += QLatin1Char('.');
    showText(text);
}
