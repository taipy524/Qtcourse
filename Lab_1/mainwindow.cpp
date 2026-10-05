#include "mainwindow.h"
#include "ui_mainwindow.h"

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
}

MainWindow::~MainWindow()
{
    delete ui;
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

