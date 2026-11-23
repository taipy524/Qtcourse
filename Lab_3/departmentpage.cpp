#include "departmentpage.h"
#include "departmenteditdialog.h"
#include "dbmanager.h"

#include <QAction>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStackedWidget>
#include <QTableView>
#include <QToolBar>
#include <QVBoxLayout>

DepartmentFilterProxy::DepartmentFilterProxy(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

void DepartmentFilterProxy::setKeyword(const QString &keyword)
{
    m_keyword = keyword.trimmed();
    invalidateFilter();
}

bool DepartmentFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (m_keyword.isEmpty())
        return true;
    const QAbstractItemModel *model = sourceModel();
    if (!model)
        return true;
    for (int column = 0; column < model->columnCount(); ++column) {
        const QModelIndex index = model->index(sourceRow, column, sourceParent);
        if (index.data(Qt::DisplayRole).toString().contains(m_keyword, Qt::CaseInsensitive))
            return true;
    }
    return false;
}

DepartmentPage::DepartmentPage(QWidget *parent)
    : QWidget(parent)
{
    buildUi();

    m_model = new QSqlTableModel(this, DbManager::instance().db());
    m_model->setTable(QStringLiteral("Department"));
    m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    m_model->setHeaderData(m_model->fieldIndex(QStringLiteral("ID")), Qt::Horizontal,
                           QStringLiteral("科室编号"));
    m_model->setHeaderData(m_model->fieldIndex(QStringLiteral("NAME")), Qt::Horizontal,
                           QStringLiteral("科室名称"));

    m_proxy = new DepartmentFilterProxy(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setDynamicSortFilter(true);

    m_view->setModel(m_proxy);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->setSortingEnabled(true);
    m_view->sortByColumn(m_model->fieldIndex(QStringLiteral("ID")), Qt::AscendingOrder);

    connect(m_proxy, &QAbstractItemModel::rowsInserted, this, [this] { updateState(); });
    connect(m_proxy, &QAbstractItemModel::rowsRemoved, this, [this] { updateState(); });
    connect(m_proxy, &QAbstractItemModel::modelReset, this, [this] { updateState(); });
}

void DepartmentPage::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(12, 10, 12, 8);
    root->setSpacing(8);

    auto *toolbar = new QToolBar(this);
    toolbar->setMovable(false);
    toolbar->setFloatable(false);
    toolbar->setIconSize(QSize(20, 20));
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(QStringLiteral("科室编号 / 名称"));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setMinimumWidth(220);

    toolbar->addWidget(new QLabel(QStringLiteral(" 查找："), this));
    toolbar->addWidget(m_searchEdit);

    QAction *searchAction = toolbar->addAction(QIcon(QStringLiteral(":/icons/search.png")),
                                               QStringLiteral("查找"));
    QAction *addAction = toolbar->addAction(QIcon(QStringLiteral(":/icons/add.png")),
                                            QStringLiteral("添加"));
    QAction *editAction = toolbar->addAction(QIcon(QStringLiteral(":/icons/edit.png")),
                                             QStringLiteral("修改"));
    QAction *deleteAction = toolbar->addAction(QIcon(QStringLiteral(":/icons/delete.png")),
                                               QStringLiteral("删除"));
    QAction *refreshAction = toolbar->addAction(QIcon(QStringLiteral(":/icons/refresh.png")),
                                                QStringLiteral("刷新"));
    toolbar->addSeparator();
    QAction *backAction = toolbar->addAction(QIcon(QStringLiteral(":/icons/back.png")),
                                             QStringLiteral("返回"));

    root->addWidget(toolbar);

    m_view = new QTableView(this);
    m_view->setAlternatingRowColors(true);
    m_view->verticalHeader()->setVisible(false);
    m_view->horizontalHeader()->setStretchLastSection(true);

    m_emptyLabel = new QLabel(QStringLiteral("暂无科室数据"), this);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setStyleSheet(QStringLiteral("color:#888;font-size:16px;background:#fafafa;"));

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(m_view);
    m_stack->addWidget(m_emptyLabel);

    m_statusLabel = new QLabel(QStringLiteral("未加载"), this);
    m_statusLabel->setStyleSheet(QStringLiteral("color:#555;"));

    root->addWidget(m_stack, 1);
    root->addWidget(m_statusLabel);

    connect(searchAction, &QAction::triggered, this, &DepartmentPage::applyFilter);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &DepartmentPage::applyFilter);
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (m_proxy)
            m_proxy->setKeyword(text);
        updateState();
    });
    connect(addAction, &QAction::triggered, this, &DepartmentPage::addDepartment);
    connect(editAction, &QAction::triggered, this, &DepartmentPage::editDepartment);
    connect(deleteAction, &QAction::triggered, this, &DepartmentPage::deleteDepartment);
    connect(refreshAction, &QAction::triggered, this, &DepartmentPage::refresh);
    connect(backAction, &QAction::triggered, this, &DepartmentPage::backRequested);
}

int DepartmentPage::doctorCount(const QString &departmentId)
{
    QSqlQuery query(DbManager::instance().db());
    query.prepare(QStringLiteral("SELECT COUNT(*) FROM Doctor WHERE DEPARTMENT_ID = ?"));
    query.addBindValue(departmentId);
    if (query.exec() && query.next())
        return query.value(0).toInt();
    return 0;
}

void DepartmentPage::refresh()
{
    if (!DbManager::instance().isOpen()) {
        m_statusLabel->setText(QStringLiteral("数据库未连接，无法加载科室信息。"));
        m_emptyLabel->setText(QStringLiteral("数据库未连接"));
        m_stack->setCurrentIndex(1);
        return;
    }

    if (!m_model->select()) {
        QMessageBox::warning(this, QStringLiteral("查询失败"),
                             DbManager::translateError(m_model->lastError()));
        return;
    }
    m_proxy->invalidate();
    m_view->resizeColumnsToContents();
    updateState();
}

void DepartmentPage::applyFilter()
{
    m_proxy->setKeyword(m_searchEdit->text());
    updateState();
    if (DbManager::instance().isOpen() && !m_userId.isEmpty()) {
        DbManager::instance().logEvent(
            m_userId,
            QStringLiteral("查询科室：关键字“%1”，命中 %2 条")
                .arg(m_searchEdit->text().trimmed().isEmpty() ? QStringLiteral("（全部）")
                                                              : m_searchEdit->text().trimmed())
                .arg(m_proxy->rowCount()));
    }
}

void DepartmentPage::addDepartment()
{
    QStringList names;
    for (int row = 0; row < m_model->rowCount(); ++row)
        names << m_model->record(row).value(QStringLiteral("NAME")).toString();

    DepartmentEditDialog dialog(this);
    dialog.setInsertMode();
    dialog.setTakenNames(names);
    if (dialog.exec() != QDialog::Accepted)
        return;
    if (!m_userId.isEmpty())
        DbManager::instance().logEvent(
            m_userId,
            QStringLiteral("新增科室 %1（%2）").arg(dialog.savedId(), dialog.savedName()));
    refresh();
}

void DepartmentPage::editDepartment()
{
    QSqlRecord record;
    if (!currentRecord(&record)) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先在列表中选择要修改的科室。"));
        return;
    }

    QStringList names;
    for (int row = 0; row < m_model->rowCount(); ++row) {
        const QString name = m_model->record(row).value(QStringLiteral("NAME")).toString();
        if (name != record.value(QStringLiteral("NAME")).toString())
            names << name;
    }

    DepartmentEditDialog dialog(this);
    dialog.setUpdateRecord(record.value(QStringLiteral("ID")).toString(),
                           record.value(QStringLiteral("NAME")).toString());
    dialog.setTakenNames(names);
    if (dialog.exec() != QDialog::Accepted)
        return;
    if (!m_userId.isEmpty())
        DbManager::instance().logEvent(
            m_userId,
            QStringLiteral("修改科室 %1（%2）").arg(dialog.savedId(), dialog.savedName()));
    refresh();
}

void DepartmentPage::deleteDepartment()
{
    QSqlRecord record;
    if (!currentRecord(&record)) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先在列表中选择要删除的科室。"));
        return;
    }

    const QString id = record.value(QStringLiteral("ID")).toString();
    const QString name = record.value(QStringLiteral("NAME")).toString();

    const int count = doctorCount(id);
    if (count > 0) {
        QMessageBox::information(
            this, QStringLiteral("无法删除"),
            QStringLiteral("科室“%1”下仍有 %2 名医生，不能删除。\n"
                           "请先在“医生管理”中把这些医生调整到其他科室。")
                .arg(name).arg(count));
        return;
    }

    if (QMessageBox::question(this, QStringLiteral("确认删除"),
                              QStringLiteral("确定删除科室“%1”吗？").arg(name),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
        != QMessageBox::Yes) {
        return;
    }

    QSqlQuery query(DbManager::instance().db());
    query.prepare(QStringLiteral("DELETE FROM Department WHERE ID = ?"));
    query.addBindValue(id);
    if (!query.exec()) {
        // 并发场景下仍可能被外键拦下
        QMessageBox::critical(this, QStringLiteral("删除失败"),
                              DbManager::translateError(query.lastError()));
        return;
    }
    if (!m_userId.isEmpty())
        DbManager::instance().logEvent(m_userId,
                                       QStringLiteral("删除科室 %1（%2）").arg(id, name));
    refresh();
}

void DepartmentPage::updateState()
{
    if (!m_proxy || !m_stack)
        return;
    const int count = m_proxy->rowCount();
    if (count == 0) {
        m_emptyLabel->setText(m_searchEdit->text().trimmed().isEmpty()
                                  ? QStringLiteral("暂无科室数据，点击“添加”录入。")
                                  : QStringLiteral("未找到匹配“%1”的科室。")
                                        .arg(m_searchEdit->text().trimmed()));
        m_stack->setCurrentIndex(1);
    } else {
        m_stack->setCurrentIndex(0);
    }
    m_statusLabel->setText(QStringLiteral("共 %1 个科室").arg(count));
}

bool DepartmentPage::currentRecord(QSqlRecord *record) const
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
