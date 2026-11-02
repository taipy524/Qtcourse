#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <QDialog>

class QPlainTextEdit;

namespace Ui {
class searchDialog;
}

class searchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit searchDialog(QWidget *parent = nullptr);
    ~searchDialog();

    void setTextEdit(QPlainTextEdit *edit);
    void setFindText(const QString &text);

private slots:
    void findNext();

private:
    Ui::searchDialog *ui;
    QPlainTextEdit *textEdit = nullptr;
};

#endif // SEARCHDIALOG_H
