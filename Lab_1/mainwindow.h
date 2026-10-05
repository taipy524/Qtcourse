#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include<QStack>

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

    QString operand;
    QString opcode;
    QStack<QString>operands;
    QStack<QString>opcodes;

    QString calculation(bool *ok=NULL);

private slots:
    void btnNumClicked();
    void btnBinaryOperatorClicked();
    void btnUnaryOperatorClicked();

    void on_pushButton_17_clicked();

    void on_pushButton_4_clicked();

    void on_pushButton_3_clicked();

    void on_btnEqual_clicked();

private:
    Ui::MainWindow *ui;

};
#endif // MAINWINDOW_H
