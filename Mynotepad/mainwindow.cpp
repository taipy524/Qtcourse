#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "aboutdialog.h"
#include "codeeditor.h"
#include "replacedialog.h"
#include "searchdialog.h"

#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QColorDialog>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QPalette>
#include <QTextCodec>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextStream>
#include <QTime>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_findDialog = new searchDialog(this);
    m_findDialog->setTextEdit(ui->textEdit);

    m_replaceDialog = new replaceDialog(this);
    m_replaceDialog->setTextEdit(ui->textEdit);

    // 查找 / 替换
    connect(ui->actionFind, &QAction::triggered, this, &MainWindow::showFindDialog);
    connect(ui->actionReplace, &QAction::triggered, this, &MainWindow::showReplaceDialog);

    // 文件操作
    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::newFile);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveFile);
    connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::saveFileAs);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);

    // 编辑操作
    connect(ui->actionUndo, &QAction::triggered, ui->textEdit, &QPlainTextEdit::undo);
    connect(ui->actionRedo, &QAction::triggered, ui->textEdit, &QPlainTextEdit::redo);
    connect(ui->actionCut, &QAction::triggered, ui->textEdit, &QPlainTextEdit::cut);
    connect(ui->actionCopy, &QAction::triggered, ui->textEdit, &QPlainTextEdit::copy);
    connect(ui->actionPaste, &QAction::triggered, ui->textEdit, &QPlainTextEdit::paste);
    connect(ui->actionSelectAll, &QAction::triggered, ui->textEdit, &QPlainTextEdit::selectAll);

    // 格式与视图
    connect(ui->actionFont, &QAction::triggered, this, &MainWindow::chooseFont);
    connect(ui->actionTextColor, &QAction::triggered, this, &MainWindow::chooseTextColor);
    connect(ui->actionBackgroundColor, &QAction::triggered, this, &MainWindow::chooseBackgroundColor);
    connect(ui->actionWrapLine, &QAction::toggled, this, &MainWindow::setWordWrap);
    connect(ui->actionLineNumber, &QAction::toggled, this, &MainWindow::setLineNumbersVisible);
    connect(ui->actionToolBar, &QAction::toggled, ui->toolBar, &QWidget::setVisible);
    connect(ui->actionStatusBar, &QAction::toggled, ui->statusbar, &QWidget::setVisible);

    // 编辑状态与 Action 使能
    connect(ui->textEdit, &QPlainTextEdit::textChanged, this, &MainWindow::documentModified);
    connect(ui->textEdit, &QPlainTextEdit::copyAvailable, this, &MainWindow::updateEditActions);
    connect(ui->textEdit, &QPlainTextEdit::undoAvailable, this, &MainWindow::updateEditActions);
    connect(ui->textEdit, &QPlainTextEdit::redoAvailable, this, &MainWindow::updateEditActions);
    connect(ui->textEdit, &QPlainTextEdit::selectionChanged, this, &MainWindow::updateEditActions);
    connect(ui->textEdit, &QPlainTextEdit::cursorPositionChanged, this, &MainWindow::updateCursorPosition);
    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &MainWindow::updateEditActions);

    // 定时自动保存
    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setInterval(10 * 1000);
    connect(m_autoSaveTimer, &QTimer::timeout, this, &MainWindow::autoSave);
    connect(ui->actionAutoSave, &QAction::toggled, m_autoSaveTimer, [this](bool enabled) {
        if (enabled)
            m_autoSaveTimer->start();
        else
            m_autoSaveTimer->stop();
    });
    if (ui->actionAutoSave->isChecked())
        m_autoSaveTimer->start();

    statusLabel.setMaximumWidth(150);
    ui->statusbar->addPermanentWidget(&statusLabel);

    statusCursorLabel.setMaximumWidth(150);
    ui->statusbar->addPermanentWidget(&statusCursorLabel);

    QLabel *author = new QLabel(ui->statusbar);
    author->setText(tr("雷语"));
    ui->statusbar->addPermanentWidget(author);

    // 与 UI 中 Action 的默认勾选状态保持一致
    ui->textEdit->setLineWrapMode(ui->actionWrapLine->isChecked()
                                      ? QPlainTextEdit::WidgetWidth
                                      : QPlainTextEdit::NoWrap);
    ui->textEdit->setLineNumbersVisible(ui->actionLineNumber->isChecked());

    updateWindowTitle();
    updateStatusInfo();
    updateEditActions();
    updateCursorPosition();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave())
        event->accept();
    else
        event->ignore();
}

bool MainWindow::maybeSave()
{
    if (!m_isModified)
        return true;

    const QString name = m_filePath.isEmpty() ? tr("无标题") : QFileInfo(m_filePath).fileName();
    const QMessageBox::StandardButton ret =
        QMessageBox::warning(this, tr("文本编辑器"),
                             tr("“%1”已修改，是否保存？").arg(name),
                             QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                             QMessageBox::Save);

    if (ret == QMessageBox::Save)
        return saveFile();
    if (ret == QMessageBox::Cancel)
        return false;
    return true;
}

bool MainWindow::writeFile(const QString &path, bool showError)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (showError) {
            QMessageBox::warning(this, tr("保存文件"),
                                 tr("无法写入文件：\n%1\n\n%2").arg(path, file.errorString()));
        }
        return false;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << ui->textEdit->toPlainText();

    if (out.status() != QTextStream::Ok) {
        if (showError) {
            QMessageBox::warning(this, tr("保存文件"),
                                 tr("写入文件失败：\n%1").arg(path));
        }
        file.close();
        return false;
    }

    file.close();
    ui->textEdit->document()->setModified(false);
    m_isModified = false;
    updateStatusInfo();
    updateEditActions();
    return true;
}

void MainWindow::newFile()
{
    if (!maybeSave())
        return;

    ui->textEdit->clear();
    m_filePath.clear();
    ui->textEdit->document()->setModified(false);
    m_isModified = false;

    updateWindowTitle();
    updateStatusInfo();
    updateEditActions();
    updateCursorPosition();
    ui->textEdit->setFocus();
}

// 自动识别文本编码：优先按 BOM，其次严格校验 UTF-8，最后回退到系统本地编码
static QString decodeText(const QByteArray &data)
{
    if (data.startsWith("\xEF\xBB\xBF")) {
        return QString::fromUtf8(data.constData() + 3, data.size() - 3);
    }
    if (data.startsWith("\xFF\xFE")) {
        QTextCodec *codec = QTextCodec::codecForName("UTF-16LE");
        return codec->toUnicode(data.constData() + 2, data.size() - 2);
    }
    if (data.startsWith("\xFE\xFF")) {
        QTextCodec *codec = QTextCodec::codecForName("UTF-16BE");
        return codec->toUnicode(data.constData() + 2, data.size() - 2);
    }

    // 无 BOM：先按 UTF-8 严格解码，包含非法字节则说明不是 UTF-8
    QTextCodec *utf8 = QTextCodec::codecForName("UTF-8");
    QTextCodec::ConverterState state;
    const QString utf8Text = utf8->toUnicode(data.constData(), data.size(), &state);
    if (state.invalidChars == 0 && state.remainingChars == 0)
        return utf8Text;

    // 回退到本地编码（中文 Windows 下为 GBK/GB18030）
    QTextCodec *local = QTextCodec::codecForLocale();
    if (local && local != utf8)
        return local->toUnicode(data);

    return utf8Text;
}

void MainWindow::openFile()
{
    if (!maybeSave())
        return;

    const QString path = QFileDialog::getOpenFileName(
        this, tr("打开文件"), QString(),
        tr("文本文件 (*.txt);;所有文件 (*.*)"));
    if (path.isEmpty())
        return;

    const QFileInfo info(path);
    if (!info.exists() || !info.isFile()) {
        QMessageBox::warning(this, tr("打开文件"),
                             tr("文件不存在或不是普通文件：\n%1").arg(path));
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("打开文件"),
                             tr("无法打开文件：\n%1\n\n%2").arg(path, file.errorString()));
        return;
    }

    const QByteArray data = file.readAll();
    if (file.error() != QFile::NoError) {
        QMessageBox::warning(this, tr("打开文件"),
                             tr("读取文件失败：\n%1\n\n%2").arg(path, file.errorString()));
        file.close();
        return;
    }
    file.close();

    QString content = decodeText(data);
    content.replace("\r\n", "\n");
    content.replace('\r', '\n');

    ui->textEdit->setPlainText(content);
    m_filePath = path;
    ui->textEdit->document()->setModified(false);
    m_isModified = false;

    updateWindowTitle();
    updateStatusInfo();
    updateEditActions();
    updateCursorPosition();
}

bool MainWindow::saveFile()
{
    if (m_filePath.isEmpty())
        return saveFileAs();

    if (!writeFile(m_filePath))
        return false;

    updateWindowTitle();
    return true;
}

bool MainWindow::saveFileAs()
{
    QString path = QFileDialog::getSaveFileName(
        this, tr("另存为"), m_filePath,
        tr("文本文件 (*.txt);;所有文件 (*.*)"));
    if (path.isEmpty())
        return false;

    if (QFileInfo(path).suffix().isEmpty())
        path += ".txt";

    if (!writeFile(path))
        return false;

    m_filePath = path;
    updateWindowTitle();
    return true;
}

void MainWindow::autoSave()
{
    // 未命名的新文件没有保存路径，等用户手动保存
    if (!m_isModified || m_filePath.isEmpty())
        return;

    if (writeFile(m_filePath, false)) {
        updateWindowTitle();
        ui->statusbar->showMessage(
            tr("已自动保存 %1").arg(QTime::currentTime().toString("HH:mm:ss")), 3000);
    } else {
        // 写盘失败时停止自动保存并只提示一次，避免每分钟弹窗
        m_autoSaveTimer->stop();
        ui->actionAutoSave->setChecked(false);
        QMessageBox::warning(this, tr("自动保存"),
                             tr("自动保存失败，已停止自动保存功能。\n"
                                "请检查文件是否可写：\n%1").arg(m_filePath));
    }
}

void MainWindow::documentModified()
{
    m_isModified = ui->textEdit->document()->isModified();
    updateWindowTitle();
    updateStatusInfo();
    updateEditActions();
}

void MainWindow::updateWindowTitle()
{
    const QString name = m_filePath.isEmpty() ? tr("无标题") : QFileInfo(m_filePath).fileName();
    setWindowTitle(QString("%1%2 - 小九文本编辑器")
                       .arg(m_isModified ? "*" : "")
                       .arg(name));
}

void MainWindow::updateStatusInfo()
{
    statusLabel.setText(tr("length: %1   lines: %2")
                            .arg(ui->textEdit->toPlainText().length())
                            .arg(ui->textEdit->document()->blockCount()));
}

void MainWindow::updateCursorPosition()
{
    const QTextCursor cursor = ui->textEdit->textCursor();
    statusCursorLabel.setText(tr("Ln: %1   Col: %2")
                                  .arg(cursor.blockNumber() + 1)
                                  .arg(cursor.positionInBlock() + 1));
}

void MainWindow::updateEditActions()
{
    const bool hasSelection = ui->textEdit->textCursor().hasSelection();

    ui->actionCut->setEnabled(hasSelection);
    ui->actionCopy->setEnabled(hasSelection);
    ui->actionPaste->setEnabled(ui->textEdit->canPaste());
    ui->actionUndo->setEnabled(ui->textEdit->document()->isUndoAvailable());
    ui->actionRedo->setEnabled(ui->textEdit->document()->isRedoAvailable());
    ui->actionSelectAll->setEnabled(!ui->textEdit->document()->isEmpty());
}

void MainWindow::chooseFont()
{
    bool ok = false;
    const QFont font = QFontDialog::getFont(&ok, ui->textEdit->font(), this, tr("选择字体"));
    if (ok)
        ui->textEdit->setFont(font);
}

void MainWindow::chooseTextColor()
{
    const QColor current = ui->textEdit->palette().color(QPalette::Text);
    const QColor color = QColorDialog::getColor(current, this, tr("选择字体颜色"));
    if (!color.isValid())
        return;

    QPalette palette = ui->textEdit->palette();
    palette.setColor(QPalette::Text, color);
    ui->textEdit->setPalette(palette);
}

void MainWindow::chooseBackgroundColor()
{
    const QColor current = ui->textEdit->palette().color(QPalette::Base);
    const QColor color = QColorDialog::getColor(current, this, tr("选择编辑器背景色"));
    if (!color.isValid())
        return;

    QPalette palette = ui->textEdit->palette();
    palette.setColor(QPalette::Base, color);
    ui->textEdit->setPalette(palette);
}

void MainWindow::setWordWrap(bool enabled)
{
    ui->textEdit->setLineWrapMode(enabled ? QPlainTextEdit::WidgetWidth
                                          : QPlainTextEdit::NoWrap);
}

void MainWindow::setLineNumbersVisible(bool visible)
{
    ui->textEdit->setLineNumbersVisible(visible);
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
