#ifndef REPLACEDIALOG_H
#define REPLACEDIALOG_H

#include <QDialog>

class QPlainTextEdit;

namespace Ui {
class replaceDialog;
}

class replaceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit replaceDialog(QWidget *parent = nullptr);
    ~replaceDialog();

    void setTextEdit(QPlainTextEdit *edit);
    void setFindText(const QString &text);

private slots:
    void findNext();
    void replace();
    void replaceAll();

private:
    bool checkFindText();

    Ui::replaceDialog *ui;
    QPlainTextEdit *textEdit = nullptr;
};

#endif // REPLACEDIALOG_H
