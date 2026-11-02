#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>

class searchDialog;
class replaceDialog;

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

private slots:
    void on_actionAbout_triggered();
    void showFindDialog();
    void showReplaceDialog();

private:
    void prefillFindText();

    Ui::MainWindow *ui;

    searchDialog *m_findDialog = nullptr;
    replaceDialog *m_replaceDialog = nullptr;

    QLabel statusCursorLabel;
    QLabel statusLabel;
};
#endif // MAINWINDOW_H
