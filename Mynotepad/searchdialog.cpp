#include "searchdialog.h"
#include "ui_searchdialog.h"

#include <QMessageBox>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QTextDocument>

searchDialog::searchDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::searchDialog)
{
    ui->setupUi(this);
    ui->gridLayout->setColumnStretch(1, 1);

    connect(ui->btnFindNext, &QPushButton::clicked, this, &searchDialog::findNext);
    connect(ui->btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->lineEditFind, &QLineEdit::returnPressed, this, &searchDialog::findNext);

    ui->radioDown->setChecked(true);
}

searchDialog::~searchDialog()
{
    delete ui;
}

void searchDialog::setTextEdit(QPlainTextEdit *edit)
{
    textEdit = edit;
}

void searchDialog::setFindText(const QString &text)
{
    ui->lineEditFind->setText(text);
}

void searchDialog::findNext()
{
    if (!textEdit)
        return;

    const QString text = ui->lineEditFind->text();
    if (text.isEmpty()) {
        QMessageBox::warning(this, tr("查找"), tr("请输入要查找的内容。"));
        ui->lineEditFind->setFocus();
        return;
    }

    QTextDocument::FindFlags flags;
    if (ui->checkCase->isChecked())
        flags |= QTextDocument::FindCaseSensitively;

    QTextDocument *doc = textEdit->document();
    const QTextCursor current = textEdit->textCursor();
    QTextCursor found;

    if (ui->radioDown->isChecked()) {
        // 向下：从当前选区末尾开始找；没有选区则从光标处开始
        const int start = current.hasSelection() ? current.selectionEnd() : current.position();
        found = doc->find(text, start, flags);
        if (found.isNull())
            found = doc->find(text, 0, flags); // 回绕到文首
    } else {
        // 向上：从当前选区开头向前找；没有选区则从光标处向前找
        const int start = current.hasSelection() ? current.selectionStart() : current.position();
        found = doc->find(text, start, flags | QTextDocument::FindBackward);
        if (found.isNull())
            found = doc->find(text, doc->characterCount(), flags | QTextDocument::FindBackward); // 回绕到文末
    }

    if (found.isNull()) {
        QMessageBox::information(this, tr("查找"),
                                 tr("未找到查找目标：%1").arg(text));
        ui->lineEditFind->setFocus();
        ui->lineEditFind->selectAll();
        return;
    }

    textEdit->setTextCursor(found);
    textEdit->ensureCursorVisible();
}
