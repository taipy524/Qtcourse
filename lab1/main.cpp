#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QTextStream>

namespace {

// ---------- 自测用的辅助：全部走真实控件与真实事件 ----------

QString displayText(MainWindow *w)
{
    QLineEdit *edit = w->findChild<QLineEdit *>(QStringLiteral("display"));
    return edit ? edit->text() : QStringLiteral("<no display>");
}

// 模拟鼠标：真正的 QPushButton::click()，触发与鼠标完全相同的信号槽
bool clickButton(MainWindow *w, const char *objectName)
{
    QPushButton *button = w->findChild<QPushButton *>(QString::fromLatin1(objectName));
    if (!button)
        return false;
    button->click();
    return true;
}

// 模拟键盘：把 QKeyEvent 直接送进主窗口的 keyPressEvent
void pressKey(MainWindow *w, int key, const QString &text = QString())
{
    QKeyEvent event(QEvent::KeyPress, key, Qt::NoModifier, text);
    QApplication::sendEvent(w, &event);
}

// 鼠标序列 "1+2=" 中的符号 -> 按钮 objectName
const char *buttonFor(QChar c)
{
    switch (c.toLatin1()) {
    case '0': return "btn0";
    case '1': return "btn1";
    case '2': return "btn2";
    case '3': return "btn3";
    case '4': return "btn4";
    case '5': return "btn5";
    case '6': return "btn6";
    case '7': return "btn7";
    case '8': return "btn8";
    case '9': return "btn9";
    case '.': return "btnDot";
    case '+': return "btnAdd";
    case '-': return "btnSub";
    case '*': return "btnMul";
    case '/': return "btnDiv";
    case '=': return "btnEqual";
    case 'C': return "btnClear";
    case '<': return "btnBackspace";
    default:  return nullptr;
    }
}

void playMouse(MainWindow *w, const QString &sequence)
{
    for (const QChar c : sequence) {
        const char *name = buttonFor(c);
        if (name)
            clickButton(w, name);
    }
}

// 键盘序列：0-9 . + - * / = 直接对应按键，R=Enter，B=Backspace，E=Esc
void playKeys(MainWindow *w, const QString &sequence)
{
    for (const QChar c : sequence) {
        const char ch = c.toLatin1();
        if (ch >= '0' && ch <= '9') {
            pressKey(w, Qt::Key_0 + (ch - '0'), QString(ch));
        } else {
            switch (ch) {
            case '.': pressKey(w, Qt::Key_Period,   QStringLiteral(".")); break;
            case '+': pressKey(w, Qt::Key_Plus,     QStringLiteral("+")); break;
            case '-': pressKey(w, Qt::Key_Minus,    QStringLiteral("-")); break;
            case '*': pressKey(w, Qt::Key_Asterisk, QStringLiteral("*")); break;
            case '/': pressKey(w, Qt::Key_Slash,    QStringLiteral("/")); break;
            case '=': pressKey(w, Qt::Key_Equal,    QStringLiteral("=")); break;
            case 'R': pressKey(w, Qt::Key_Return);                       break;
            case 'B': pressKey(w, Qt::Key_Backspace);                    break;
            case 'E': pressKey(w, Qt::Key_Escape);                       break;
            default: break;
            }
        }
    }
}

struct Case {
    const char *id;
    const char *name;
    bool useMouse;
    const char *input;
    const char *expected;
};

const Case kCases[] = {
    {"T01",  "加法",             true,  "1+2=",              "3"},
    {"T02",  "减法",             true,  "10-3=",             "7"},
    {"T03",  "乘法",             true,  "5*6=",              "30"},
    {"T04",  "除法",             true,  "20/4=",             "5"},
    {"T05",  "小数运算",         true,  "1.5+2.5=",          "4"},
    {"T06",  "重复小数点",       true,  "1.2.3",             "1.23"},
    {"T07",  "除零",             true,  "10/0=",             "不能除以0"},
    {"T07b", "除零后可恢复",     true,  "10/0=12",           "12"},
    {"T08",  "退格",             true,  "12345<<",           "123"},
    {"T09",  "清除",             true,  "12345C",            "0"},
    {"T10",  "连续计算",         true,  "10+5=+3=",          "18"},
    {"T10b", "连续计算继续",     true,  "10+5=+3=*2=",       "36"},
    {"T11",  "计算后重新输入",   true,  "10+5=2",            "2"},
    {"T12",  "键盘运算",         false, "12+5=",              "17"},
    {"T13",  "键盘退格",         false, "12345BB",            "123"},
    {"T14",  "键盘清除",         false, "12345E",             "0"},
    {"T15",  "键盘小数与回车",   false, "1.5+2.5R",           "4"},
    {"T16",  "键盘乘法",         false, "5*6R",               "30"},
    {"T17",  "键盘除零",         false, "10/0=",              "不能除以0"},
    {"T18",  "键盘连续计算",     false, "10+5R+3R",           "18"},
};

/**
 * @brief 命令行自测：lab1.exe --selftest
 *
 * 直接驱动真实窗口里的按钮 click 信号与 keyPressEvent，
 * 结果写入 selftest_results.txt，返回 0 表示全部通过。
 */
int runSelfTest(MainWindow *w, const QString &outPath)
{
    int failed = 0;
    int passed = 0;

    QFile file(outPath);
    const bool opened = file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
    if (!opened)
        return 2;                       // 结果文件无法写入
    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << QStringLiteral("ID\t名称\t输入方式\t输入\t预期\t实际\t状态\n");

    for (const Case &c : kCases) {
        // 每个用例前先用 Esc 复位，保证互不影响
        pressKey(w, Qt::Key_Escape);
        QApplication::processEvents();

        const QString input = QString::fromUtf8(c.input);
        if (c.useMouse)
            playMouse(w, input);
        else
            playKeys(w, input);

        const QString actual = displayText(w);
        const QString expected = QString::fromUtf8(c.expected);
        const bool ok = (actual == expected);
        if (ok)
            ++passed;
        else
            ++failed;

        out << QString::fromUtf8(c.id) << '\t'
            << QString::fromUtf8(c.name) << '\t'
            << (c.useMouse ? QStringLiteral("鼠标") : QStringLiteral("键盘")) << '\t'
            << input << '\t'
            << expected << '\t'
            << actual << '\t'
            << (ok ? QStringLiteral("PASS") : QStringLiteral("FAIL")) << '\n';
    }

    out << QStringLiteral("TOTAL\t\t\t\t\t") << passed << QStringLiteral(" / ")
        << (passed + failed) << QStringLiteral(" PASS\n");
    file.close();
    return failed == 0 ? 0 : 1;
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    w.show();
    QApplication::processEvents();

    if (a.arguments().contains(QStringLiteral("--selftest")))
        return runSelfTest(&w, QStringLiteral("selftest_results.txt"));

    return a.exec();
}
