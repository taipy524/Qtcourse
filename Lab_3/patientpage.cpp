#include "patientpage.h"
#include "patienteditdialog.h"
#include "dbmanager.h"
#include "delegates/comboboxdelegate.h"

#include <QAction>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStackedWidget>
#include <QTableView>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

// ---------------------------------------------------------------------------
// PatientTableModel
// ---------------------------------------------------------------------------
PatientTableModel::PatientTableModel(QObject *parent, const QSqlDatabase &db)
    : QSqlTableModel(parent, db)
{
}

Qt::ItemFlags PatientTableModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags result = QSqlTableModel::flags(index);
    if (index.isValid() && index.column() != fieldIndex(QStringLiteral("SEX")))
        result &= ~Qt::ItemIsEditable;
    return result;
}

// ---------------------------------------------------------------------------
// PatientFilterProxy
// ---------------------------------------------------------------------------
PatientFilterProxy::PatientFilterProxy(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

void PatientFilterProxy::setKeyword(const QString &keyword)
{
    m_keyword = keyword.trimmed();
    invalidateFilter();
}

void PatientFilterProxy::setKeyColumns(const QList<int> &columns)
{
    m_columns = columns;
}

bool PatientFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (m_keyword.isEmpty())
        return true;

    const QAbstractItemModel *model = sourceModel();
    if (!model)
        return true;

    for (int column : m_columns) {
        const QModelIndex index = model->index(sourceRow, column, sourceParent);
        if (index.data(Qt::DisplayRole).toString().contains(m_keyword, Qt::CaseInsensitive))
            return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// PatientPage
// ---------------------------------------------------------------------------
PatientPage::PatientPage(QWidget *parent)
    : QWidget(parent)
{
    buildUi();

    m_model = new PatientTableModel(this, DbManager::instance().db());
    m_model->setTable(QStringLiteral("Patient"));
    m_model->setEditStrategy(QSqlTableModel::OnRowChange);

    const QList<QPair<QString, QString>> headers = {
        {QStringLiteral("ID"), QStringLiteral("ID")},
        {QStringLiteral("NAME"), QStringLiteral("姓名")},
        {QStringLiteral("ID_CARD"), QStringLiteral("身份证")},
        {QStringLiteral("SEX"), QStringLiteral("性别")},
        {QStringLiteral("DOB"), QStringLiteral("出生日期")},
        {QStringLiteral("AGE"), QStringLiteral("年龄")},
        {QStringLiteral("HEIGHT"), QStringLiteral("身高(cm)")},
        {QStringLiteral("WEIGHT"), QStringLiteral("体重(kg)")},
        {QStringLiteral("MOBILEPHONE"), QStringLiteral("手机号")},
        {QStringLiteral("CREATEDTIMESTAMP"), QStringLiteral("创建时间")},
    };
    for (const auto &pair : headers) {
        const int column = m_model->fieldIndex(pair.first);
        if (column >= 0)
            m_model->setHeaderData(column, Qt::Horizontal, pair.second);
    }

    m_sexColumn = m_model->fieldIndex(QStringLiteral("SEX"));

    m_proxy = new PatientFilterProxy(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setDynamicSortFilter(true);

    QList<int> keyColumns;
    keyColumns << m_model->fieldIndex(QStringLiteral("ID"))
               << m_model->fieldIndex(QStringLiteral("ID_CARD"))
               << m_model->fieldIndex(QStringLiteral("NAME"))
               << m_model->fieldIndex(QStringLiteral("MOBILEPHONE"));
    m_proxy->setKeyColumns(keyColumns);

    m_view->setModel(m_proxy);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->setEditTriggers(QAbstractItemView::DoubleClicked
                            | QAbstractItemView::SelectedClicked
                            | QAbstractItemView::EditKeyPressed);
    m_view->setSortingEnabled(true);
    if (m_model->fieldIndex(QStringLiteral("CREATEDTIMESTAMP")) >= 0)
        m_view->sortByColumn(m_model->fieldIndex(QStringLiteral("CREATEDTIMESTAMP")), Qt::DescendingOrder);

    auto *sexDelegate = new ComboBoxDelegate(this);
    sexDelegate->setItems(QVariantList() << 0 << 1 << 2,
                          QStringList() << QStringLiteral("未知")
                                        << QStringLiteral("男")
                                        << QStringLiteral("女"));
    if (m_sexColumn >= 0)
        m_view->setItemDelegateForColumn(m_sexColumn, sexDelegate);

    connect(m_view, &QTableView::doubleClicked, this, [this](const QModelIndex &index) {
        if (m_sexColumn >= 0 && index.column() == m_sexColumn)
            return;   // 性别列用代理就地编辑
        if (index.isValid())
            editPatient();
    });

    connect(m_model, &QAbstractItemModel::dataChanged, this, &PatientPage::onDataChanged);
    m_editLogTimer = new QTimer(this);
    m_editLogTimer->setSingleShot(true);
    m_editLogTimer->setInterval(400);
    connect(m_editLogTimer, &QTimer::timeout, this, &PatientPage::logInlineEdit);
    connect(m_proxy, &QAbstractItemModel::rowsInserted, this, [this] { updateState(); });
    connect(m_proxy, &QAbstractItemModel::rowsRemoved, this, [this] { updateState(); });
    connect(m_proxy, &QAbstractItemModel::modelReset, this, [this] { updateState(); });
}

void PatientPage::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(12, 10, 12, 8);
    root->setSpacing(8);

    m_toolbar = new QToolBar(this);
    m_toolbar->setMovable(false);
    m_toolbar->setFloatable(false);
    m_toolbar->setIconSize(QSize(20, 20));
    m_toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(QStringLiteral("姓名 / 身份证 / 手机号 / ID"));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setMinimumWidth(220);

    m_toolbar->addWidget(new QLabel(QStringLiteral(" 查找："), this));
    m_toolbar->addWidget(m_searchEdit);

    QAction *searchAction = m_toolbar->addAction(QIcon(QStringLiteral(":/icons/search.png")),
                                                 QStringLiteral("查找"));
    QAction *addAction = m_toolbar->addAction(QIcon(QStringLiteral(":/icons/add.png")),
                                              QStringLiteral("添加"));
    QAction *editAction = m_toolbar->addAction(QIcon(QStringLiteral(":/icons/edit.png")),
                                               QStringLiteral("修改"));
    QAction *deleteAction = m_toolbar->addAction(QIcon(QStringLiteral(":/icons/delete.png")),
                                                 QStringLiteral("删除"));
    QAction *refreshAction = m_toolbar->addAction(QIcon(QStringLiteral(":/icons/refresh.png")),
                                                  QStringLiteral("刷新"));
    m_toolbar->addSeparator();
    QAction *backAction = m_toolbar->addAction(QIcon(QStringLiteral(":/icons/back.png")),
                                               QStringLiteral("返回"));

    root->addWidget(m_toolbar);

    m_view = new QTableView(this);
    m_view->setAlternatingRowColors(true);
    m_view->verticalHeader()->setVisible(false);
    m_view->horizontalHeader()->setStretchLastSection(true);
    m_view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

    m_emptyLabel = new QLabel(QStringLiteral("暂无患者数据"), this);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setStyleSheet(QStringLiteral("color:#888;font-size:16px;background:#fafafa;"));

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(m_view);
    m_stack->addWidget(m_emptyLabel);
    m_stack->setCurrentIndex(0);

    m_statusLabel = new QLabel(QStringLiteral("未加载"), this);
    m_statusLabel->setStyleSheet(QStringLiteral("color:#555;"));

    root->addWidget(m_stack, 1);
    root->addWidget(m_statusLabel);

    connect(searchAction, &QAction::triggered, this, &PatientPage::applyFilter);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &PatientPage::applyFilter);
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (m_proxy)
            m_proxy->setKeyword(text);
        updateState();
    });
    connect(addAction, &QAction::triggered, this, &PatientPage::addPatient);
    connect(editAction, &QAction::triggered, this, &PatientPage::editPatient);
    connect(deleteAction, &QAction::triggered, this, &PatientPage::deletePatient);
    connect(refreshAction, &QAction::triggered, this, &PatientPage::refresh);
    connect(backAction, &QAction::triggered, this, &PatientPage::backRequested);
}

void PatientPage::refresh()
{
    if (!DbManager::instance().isOpen()) {
        m_statusLabel->setText(QStringLiteral("数据库未连接，无法加载患者信息。"));
        m_emptyLabel->setText(QStringLiteral("数据库未连接"));
        m_stack->setCurrentIndex(1);
        return;
    }

    m_lastSelectedId.clear();
    QSqlRecord current;
    if (currentRecord(&current))
        m_lastSelectedId = current.value(QStringLiteral("ID")).toString();

    if (!m_model->select()) {
        QMessageBox::warning(this, QStringLiteral("查询失败"),
                             DbManager::translateError(m_model->lastError()));
        return;
    }

    const int createdColumn = m_model->fieldIndex(QStringLiteral("CREATEDTIMESTAMP"));
    if (createdColumn >= 0)
        m_model->sort(createdColumn, Qt::DescendingOrder);

    m_proxy->invalidate();
    m_view->resizeColumnsToContents();
    updateState();

    if (!m_lastSelectedId.isEmpty()) {
        for (int row = 0; row < m_proxy->rowCount(); ++row) {
            const QModelIndex index = m_proxy->index(row, 0);
            if (index.data().toString() == m_lastSelectedId) {
                m_view->selectRow(row);
                m_view->scrollTo(index);
                break;
            }
        }
    }
}

void PatientPage::applyFilter()
{
    if (!m_proxy)
        return;
    m_proxy->setKeyword(m_searchEdit->text());
    updateState();

    if (DbManager::instance().isOpen() && !m_userId.isEmpty()) {
        const QString keyword = m_searchEdit->text().trimmed();
        DbManager::instance().logEvent(
            m_userId,
            QStringLiteral("查询患者：关键字“%1”，命中 %2 条")
                .arg(keyword.isEmpty() ? QStringLiteral("（全部）") : keyword)
                .arg(m_proxy->rowCount()));
    }
}

void PatientPage::addPatient()
{
    if (!DbManager::instance().isOpen()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("数据库未连接。"));
        return;
    }

    PatientEditDialog dialog(this);
    dialog.setInsertMode();
    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString id = dialog.savedId();
    const QString name = dialog.savedName();
    if (!m_userId.isEmpty())
        DbManager::instance().logEvent(m_userId,
                                       QStringLiteral("新增患者 %1（%2）").arg(id, name));
    refresh();
}

void PatientPage::editPatient()
{
    QSqlRecord record;
    if (!currentRecord(&record)) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先在列表中选择要修改的患者。"));
        return;
    }

    PatientEditDialog dialog(this);
    dialog.setUpdateRecord(record);
    if (dialog.exec() != QDialog::Accepted)
        return;

    if (!m_userId.isEmpty())
        DbManager::instance().logEvent(
            m_userId,
            QStringLiteral("修改患者 %1（%2）").arg(dialog.savedId(), dialog.savedName()));
    refresh();
}

void PatientPage::deletePatient()
{
    QSqlRecord record;
    if (!currentRecord(&record)) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先在列表中选择要删除的患者。"));
        return;
    }

    const QString id = record.value(QStringLiteral("ID")).toString();
    const QString name = record.value(QStringLiteral("NAME")).toString();

    const QMessageBox::StandardButton answer = QMessageBox::question(
        this, QStringLiteral("确认删除"),
        QStringLiteral("确定要删除患者“%1”（ID：%2）吗？\n删除后不可恢复。").arg(name, id),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;

    QSqlQuery query(DbManager::instance().db());
    query.prepare(QStringLiteral("DELETE FROM Patient WHERE ID = ?"));
    query.addBindValue(id);
    if (!query.exec()) {
        QMessageBox::critical(this, QStringLiteral("删除失败"),
                              DbManager::translateError(query.lastError()));
        return;
    }
    if (query.numRowsAffected() <= 0) {
        QMessageBox::warning(this, QStringLiteral("删除失败"),
                             QStringLiteral("数据库中未找到该记录，可能已被其他用户删除。"));
        refresh();
        return;
    }

    if (!m_userId.isEmpty())
        DbManager::instance().logEvent(m_userId,
                                       QStringLiteral("删除患者 %1（%2）").arg(id, name));
    refresh();
}

void PatientPage::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    Q_UNUSED(bottomRight)

    // QSqlTableModel 的 setData + submit 可能连续触发两次 dataChanged，
    // 用 400ms 防抖只写一条日志。
    if (!topLeft.isValid())
        return;
    m_pendingEditIndex = QPersistentModelIndex(topLeft);
    if (m_editLogTimer)
        m_editLogTimer->start();
}

void PatientPage::logInlineEdit()
{
    if (!m_pendingEditIndex.isValid())
        return;

    const QSqlRecord record = m_model->record(m_pendingEditIndex.row());
    const QString id = record.value(QStringLiteral("ID")).toString();
    if (id.isEmpty())
        return;

    if (!m_userId.isEmpty()) {
        DbManager::instance().logEvent(
            m_userId, QStringLiteral("表格内修改患者 %1 的性别").arg(id));
    }
    m_statusLabel->setText(QStringLiteral("共 %1 条记录（已同步数据库，最后修改：%2）")
                               .arg(m_proxy->rowCount())
                               .arg(id));
}

void PatientPage::updateState()
{
    if (!m_proxy || !m_stack)
        return;

    const int count = m_proxy->rowCount();
    if (count == 0) {
        if (m_searchEdit->text().trimmed().isEmpty())
            m_emptyLabel->setText(QStringLiteral("暂无患者数据，点击“添加”录入第一条记录。"));
        else
            m_emptyLabel->setText(QStringLiteral("未找到匹配“%1”的患者记录。\n请更换关键字或点击“刷新”。")
                                      .arg(m_searchEdit->text().trimmed()));
        m_stack->setCurrentIndex(1);
    } else {
        m_stack->setCurrentIndex(0);
    }
    m_statusLabel->setText(QStringLiteral("共 %1 条记录%2")
                               .arg(count)
                               .arg(m_searchEdit->text().trimmed().isEmpty()
                                        ? QString()
                                        : QStringLiteral("（已按关键字过滤）")));
}

bool PatientPage::currentRecord(QSqlRecord *record) const
{
    if (!m_view || !m_proxy)
        return false;
    const QModelIndex index = m_view->currentIndex();
    if (!index.isValid())
        return false;
    const QModelIndex source = m_proxy->mapToSource(index);
    if (!source.isValid())
        return false;
    *record = m_model->record(source.row());
    return true;
}
