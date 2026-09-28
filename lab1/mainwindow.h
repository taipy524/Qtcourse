#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    // 统一的业务核心函数：鼠标按钮与键盘事件共用
    void setupConnections();
    void inputDigit(const QString &digit);
    void inputDot();

    void showText(const QString &text);
    QString displayText() const;

    Ui::MainWindow *ui;

    // 计算器内部状态
    double m_first;        // 第一个操作数
    QString m_op;          // 当前运算符 + - * /
    bool m_waiting;        // 已按运算符，等待第二个操作数
    bool m_calculated;     // 刚完成一次计算
};
#endif // MAINWINDOW_H
