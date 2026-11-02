#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QString>

class QTimer;
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

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void on_actionAbout_triggered();
    void showFindDialog();
    void showReplaceDialog();
    void newFile();
    void openFile();
    bool saveFile();
    bool saveFileAs();
    void documentModified();
    void updateEditActions();
    void updateCursorPosition();
    void chooseFont();
    void chooseTextColor();
    void chooseBackgroundColor();
    void setWordWrap(bool enabled);
    void setLineNumbersVisible(bool visible);
    void autoSave();

private:
    void prefillFindText();
    bool maybeSave();
    bool writeFile(const QString &path, bool showError = true);
    void updateWindowTitle();
    void updateStatusInfo();

    Ui::MainWindow *ui;

    searchDialog *m_findDialog = nullptr;
    replaceDialog *m_replaceDialog = nullptr;
    QTimer *m_autoSaveTimer = nullptr;

    QString m_filePath;
    bool m_isModified = false;

    QLabel statusCursorLabel;
    QLabel statusLabel;
};
#endif // MAINWINDOW_H
