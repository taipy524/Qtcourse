#include "mainwindow.h"
#include "ui_mainwindow.h"
#include<QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->btnNum0, SIGNAL(clicked()), this, SLOT(btnNumClicked()));
    connect(ui->btnNum1, SIGNAL(clicked()), this, SLOT(btnNumClicked()));
    connect(ui->btnNum2, SIGNAL(clicked()), this, SLOT(btnNumClicked()));
    connect(ui->btnNum3, SIGNAL(clicked()), this, SLOT(btnNumClicked()));
    connect(ui->btnNum4, SIGNAL(clicked()), this, SLOT(btnNumClicked()));
    connect(ui->btnNum5, SIGNAL(clicked()), this, SLOT(btnNumClicked()));
    connect(ui->btnNum6, SIGNAL(clicked()), this, SLOT(btnNumClicked()));
    connect(ui->btnNum7, SIGNAL(clicked()), this, SLOT(btnNumClicked()));
    connect(ui->btnNum8, SIGNAL(clicked()), this, SLOT(btnNumClicked()));
    connect(ui->btnNum9, SIGNAL(clicked()), this, SLOT(btnNumClicked()));

    connect(ui->binPlus, SIGNAL(clicked()), this, SLOT(btnBinaryOperatorClicked()));
    connect(ui->binMinus, SIGNAL(clicked()), this, SLOT(btnBinaryOperatorClicked()));
    connect(ui->binMultiple, SIGNAL(clicked()), this, SLOT(btnBinaryOperatorClicked()));
    connect(ui->binDivide, SIGNAL(clicked()), this, SLOT(btnBinaryOperatorClicked()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

QString MainWindow::calculation(bool *ok)
{
    double result=0;
    if(operands.size()==2 &&opcodes.size()>0){
        //取操作数
        double operand1=operands.front().toDouble();
        operands.pop_front();
        double operand2=operands.front().toDouble();
        operands.pop_front();

        //取操作符
        QString op=opcodes.front();
        opcodes.pop_front();


        if(op=="+"){
            result=operand1+operand2;
        }
        else if(op=="-"){
             result=operand1-operand2;
        }
        else if(op=="×"){
            result=operand1*operand2;
        }
        else if(op=="÷"){
            result=operand1/operand2;
        }

        ui->statusbar->showMessage(QString("calcuation is in progress : operands is %1,opcodes is %2").arg(operands.size()).arg(opcodes.size()));
    }
    else{
          ui->statusbar->showMessage(QString("operands is %1,opcodes is %2")
                                       .arg(operands.size()).arg(opcodes.size()));
    }
    return QString::number(result);
}

void MainWindow::btnNumClicked()
{
    QString digit=qobject_cast<QPushButton *>(sender())->text();

    if(digit=="0"&&operand=="0")
        digit="";

    if(operand=="0"&& digit!="0")
        operand="";

    operand +=digit;


    ui->lineEdit->setText(operand);
    //ui->statusbar->showMessage(qobject_cast<QPushButton *>(sender())->text()+"btn clicked");

}


void MainWindow::on_pushButton_17_clicked()//小数点
{
    if(!operand.contains("."))
        operand +=qobject_cast<QPushButton *>(sender())->text();
     ui->lineEdit->setText(operand);
}


void MainWindow::on_pushButton_4_clicked()//退格
{
    operand=operand.left(operand.length()-1);
    ui->lineEdit->setText(operand);
}


void MainWindow::on_pushButton_3_clicked()
{
    operand.clear();
    ui->lineEdit->setText(operand);
}


void MainWindow::btnBinaryOperatorClicked()
{
    ui->statusbar->showMessage("last operand"+ operand);
    QString opcode=qobject_cast<QPushButton *>(sender())->text();
    qDebug()<<opcode;

    if(operand !=""){
        operands.push_back(operand);
        operand="";

        opcodes.push_back(opcode);


        QString result=calculation();

        ui->lineEdit->setText(result);
    }
}

void MainWindow::on_btnEqual_clicked()
{
    if(operand !=""){
        operands.push_back(operand);
        operand="";
    }

    QString result=calculation();
    ui->lineEdit->setText(result);
}

