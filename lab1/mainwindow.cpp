#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_first(0.0)
    , m_waiting(false)
    , m_calculated(false)
    , m_error(false)
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

    struct OperatorBinding {
        QPushButton *button;
        const char *op;
    };
    const OperatorBinding operators[] = {
        {ui->btnAdd, "+"}, {ui->btnSub, "-"},
        {ui->btnMul, "*"}, {ui->btnDiv, "/"},
    };
    for (const OperatorBinding &item : operators) {
        const QString op = QString::fromLatin1(item.op);
        connect(item.button, &QPushButton::clicked, this, [this, op]() {
            inputOperator(op);
        });
    }

    connect(ui->btnEqual, &QPushButton::clicked, this, &MainWindow::calculateResult);
    connect(ui->btnClear, &QPushButton::clicked, this, &MainWindow::clearCalculator);
    connect(ui->btnBackspace, &QPushButton::clicked, this, &MainWindow::backspace);
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
    if (m_error)
        clearCalculator();                 // 上次出错，重新开始

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
    if (m_error)
        clearCalculator();

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

/**
 * @brief 显示一个计算结果，去掉无意义的尾随零（2.000000 -> 2）
 */
void MainWindow::showValue(double value)
{
    if (value == 0.0)
        value = 0.0;                       // 消除 -0 显示
    showText(QString::number(value, 'g', 15));
}

/**
 * @brief 取当前显示框里的操作数；非法内容按 0 处理
 */
double MainWindow::currentOperand() const
{
    bool ok = false;
    const double value = displayText().toDouble(&ok);
    return ok ? value : 0.0;
}

/**
 * @brief 运算符输入
 *
 * 处于“等待第二个操作数”时再按运算符，只替换运算符，
 * 这样连续按多个运算符不会产生非法状态。
 */
void MainWindow::inputOperator(const QString &op)
{
    if (m_error)
        clearCalculator();

    if (m_waiting) {                         // 还没输入第二个操作数：只换运算符
        m_op = op;
        return;
    }

    if (!m_op.isEmpty()) {                   // 已有挂起的运算，先把上一步算出来
        if (!performCalculation())
            return;                          // 出错（如除以 0）后停止本次输入
    }

    m_first = currentOperand();              // 记下第一个操作数
    m_op = op;
    m_waiting = true;
    m_calculated = false;
}

/**
 * @brief 等号：完成一次计算
 *
 * 没有运算符或还没输入第二个操作数时按等号，直接忽略，不崩溃。
 */
void MainWindow::calculateResult()
{
    if (m_error)
        return;

    if (m_op.isEmpty() || m_waiting)
        return;

    performCalculation();
}

/**
 * @brief 真正执行一次二元运算，返回是否成功
 *
 * 连续计算（1 + 2 + 3 =）与等号共用这一份代码。
 */
bool MainWindow::performCalculation()
{
    const double a = m_first;
    const double b = currentOperand();

    if (m_op == QStringLiteral("/") && b == 0.0) {
        showError();                         // 除以 0：提示并复位
        return false;
    }

    double result = 0.0;

    if (m_op == QStringLiteral("+"))
        result = a + b;
    else if (m_op == QStringLiteral("-"))
        result = a - b;
    else if (m_op == QStringLiteral("*"))
        result = a * b;
    else if (m_op == QStringLiteral("/"))
        result = a / b;
    else
        return false;

    showValue(result);

    m_first = result;
    m_op.clear();
    m_waiting = false;
    m_calculated = true;
    return true;
}

/**
 * @brief 清除 C：界面与内部计算状态一起复位
 */
void MainWindow::clearCalculator()
{
    m_first = 0.0;
    m_op.clear();
    m_waiting = false;
    m_calculated = false;
    m_error = false;
    showText(QStringLiteral("0"));
}

/**
 * @brief 退格：删除最后一位；删空后回落到 0，不会出现空字符串
 */
void MainWindow::backspace()
{
    if (m_error) {
        clearCalculator();
        return;
    }

    QString text = displayText();
    text.chop(1);
    if (text.isEmpty())
        text = QStringLiteral("0");

    showText(text);
    m_calculated = false;                    // 退格后按“正在输入”处理
}

/**
 * @brief 错误提示（除以 0），随后复位到可继续操作的状态
 */
void MainWindow::showError()
{
    showText(QStringLiteral("不能除以0"));

    m_first = 0.0;
    m_op.clear();
    m_waiting = false;
    m_calculated = false;
    m_error = true;
}
