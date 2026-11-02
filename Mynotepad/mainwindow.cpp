#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "aboutdialog.h"
#include "replacedialog.h"
#include "searchdialog.h"

#include <QTextCursor>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_findDialog = new searchDialog(this);
    m_findDialog->setTextEdit(ui->textEdit);

    m_replaceDialog = new replaceDialog(this);
    m_replaceDialog->setTextEdit(ui->textEdit);

    connect(ui->actionFind, &QAction::triggered, this, &MainWindow::showFindDialog);
    connect(ui->actionReplace, &QAction::triggered, this, &MainWindow::showReplaceDialog);

    ui->statusbar;
    statusLabel.setMaximumWidth(150);
    statusLabel.setText("length: " + QString::number(0) + "   lines: " + QString::number(1));
    ui->statusbar->addPermanentWidget(&statusLabel);

    statusCursorLabel.setMaximumWidth(150);
    statusCursorLabel.setText("Ln: " + QString::number(0) + "   Col: " + QString::number(1));
    ui->statusbar->addPermanentWidget(&statusCursorLabel);

    QLabel *author = new QLabel(ui->statusbar);
    author->setText(tr("雷语"));
    ui->statusbar->addPermanentWidget(author);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::prefillFindText()
{
    const QTextCursor cursor = ui->textEdit->textCursor();
    if (!cursor.hasSelection())
        return;

    const QString selected = cursor.selectedText();
    if (selected.isEmpty() || selected.contains(QChar::ParagraphSeparator))
        return;

    m_findDialog->setFindText(selected);
    m_replaceDialog->setFindText(selected);
}

void MainWindow::showFindDialog()
{
    prefillFindText();
    m_findDialog->exec();
}

void MainWindow::showReplaceDialog()
{
    prefillFindText();
    m_replaceDialog->exec();
}

void MainWindow::on_actionAbout_triggered()
{
    aboutDialog dlg(this);
    dlg.exec();
}
