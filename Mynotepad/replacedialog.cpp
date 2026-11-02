#include "replacedialog.h"
#include "ui_replacedialog.h"

#include <QMessageBox>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QTextDocument>

replaceDialog::replaceDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::replaceDialog)
{
    ui->setupUi(this);
    ui->gridLayout->setColumnStretch(1, 1);

    connect(ui->btnFindNext, &QPushButton::clicked, this, &replaceDialog::findNext);
    connect(ui->btnReplace, &QPushButton::clicked, this, &replaceDialog::replace);
    connect(ui->btnReplaceAll, &QPushButton::clicked, this, &replaceDialog::replaceAll);
    connect(ui->btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->lineEditFind, &QLineEdit::returnPressed, this, &replaceDialog::findNext);

    ui->radioDown->setChecked(true);
}

replaceDialog::~replaceDialog()
{
    delete ui;
}

void replaceDialog::setTextEdit(QPlainTextEdit *edit)
{
    textEdit = edit;
}

void replaceDialog::setFindText(const QString &text)
{
    ui->lineEditFind->setText(text);
}

bool replaceDialog::checkFindText()
{
    if (ui->lineEditFind->text().isEmpty()) {
        QMessageBox::warning(this, tr("替换"), tr("请输入要查找的内容。"));
        ui->lineEditFind->setFocus();
        return false;
    }
    return true;
}

void replaceDialog::findNext()
{
    if (!textEdit || !checkFindText())
        return;

    const QString text = ui->lineEditFind->text();

    QTextDocument::FindFlags flags;
    if (ui->checkCase->isChecked())
        flags |= QTextDocument::FindCaseSensitively;

    QTextDocument *doc = textEdit->document();
    const QTextCursor current = textEdit->textCursor();
    QTextCursor found;

    if (ui->radioDown->isChecked()) {
        const int start = current.hasSelection() ? current.selectionEnd() : current.position();
        found = doc->find(text, start, flags);
        if (found.isNull())
            found = doc->find(text, 0, flags); // 回绕到文首
    } else {
        const int start = current.hasSelection() ? current.selectionStart() : current.position();
        found = doc->find(text, start, flags | QTextDocument::FindBackward);
        if (found.isNull())
            found = doc->find(text, doc->characterCount(), flags | QTextDocument::FindBackward); // 回绕到文末
    }

    if (found.isNull()) {
        QMessageBox::information(this, tr("替换"),
                                 tr("未找到查找目标：%1").arg(text));
        ui->lineEditFind->setFocus();
        ui->lineEditFind->selectAll();
        return;
    }

    textEdit->setTextCursor(found);
    textEdit->ensureCursorVisible();
}

void replaceDialog::replace()
{
    if (!textEdit || !checkFindText())
        return;

    const QString findText = ui->lineEditFind->text();
    const QString replaceText = ui->lineEditReplace->text();

    const Qt::CaseSensitivity cs = ui->checkCase->isChecked()
                                       ? Qt::CaseSensitive
                                       : Qt::CaseInsensitive;

    QTextCursor cursor = textEdit->textCursor();
    if (cursor.hasSelection()
        && QString::compare(cursor.selectedText(), findText, cs) == 0) {
        // 当前选中的正是查找目标，直接替换
        cursor.insertText(replaceText);
        textEdit->setTextCursor(cursor);
    }

    // 替换后自动定位到下一个匹配项
    findNext();
}

void replaceDialog::replaceAll()
{
    if (!textEdit || !checkFindText())
        return;

    const QString findText = ui->lineEditFind->text();
    const QString replaceText = ui->lineEditReplace->text();

    QTextDocument::FindFlags flags;
    if (ui->checkCase->isChecked())
        flags |= QTextDocument::FindCaseSensitively;

    QTextDocument *doc = textEdit->document();
    int count = 0;

    QTextCursor editCursor(doc);
    editCursor.beginEditBlock();

    QTextCursor found = doc->find(findText, 0, flags);
    while (!found.isNull()) {
        found.insertText(replaceText);
        ++count;
        // 从插入内容之后继续查找，避免替换文本包含查找文本时死循环
        found = doc->find(findText, found.position(), flags);
    }

    editCursor.endEditBlock();

    if (count == 0) {
        QMessageBox::information(this, tr("替换"),
                                 tr("未找到查找目标：%1").arg(findText));
    } else {
        QMessageBox::information(this, tr("替换"),
                                 tr("全部替换完成，共替换 %1 处。").arg(count));
    }
}
