#include "doctorpage.h"
#include "doctoreditdialog.h"
#include "dbmanager.h"

#include <QAction>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlRelationalDelegate>
#include <QStackedWidget>
#include <QTableView>
#include <QToolBar>
#include <QVBoxLayout>

DoctorFilterProxy::DoctorFilterProxy(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

void DoctorFilterProxy::setKeyword(const QString &keyword)
{
    m_keyword = keyword.trimmed();
    invalidateFilter();
}

void DoctorFilterProxy::setKeyColumns(const QList<int> &columns)
{
    m_columns = columns;
}

bool DoctorFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
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

DoctorPage::DoctorPage(QWidget *parent)
    : QWidget(parent)
{
    buildUi();

    m_model = new QSqlRelationalTableModel(this, DbManager::instance().db());
    m_model->setTable(QStringLiteral("Doctor"));
    m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    m_model->setJoinMode(QSqlRelationalTableModel::LeftJoin);

    const int deptColumn = m_model->fieldIndex(QStringLiteral("DEPARTMENT_ID"));
    if (deptColumn >= 0)
        m_model->setRelation(deptColumn, QSqlRelation(QStringLiteral("Department"),
                                                      QStringLiteral("ID"),
                                                      QStringLiteral("NAME")));

    m_model->setHeaderData(m_model->fieldIndex(QStringLiteral("ID")), Qt::Horizontal,
                           QStringLiteral("ID"));
    m_model->setHeaderData(m_model->fieldIndex(QStringLiteral("EMPLOYEE_NO")), Qt::Horizontal,
                           QStringLiteral("工号"));
    m_model->setHeaderData(m_model->fieldIndex(QStringLiteral("NAME")), Qt::Horizontal,
                           QStringLiteral("姓名"));
    m_model->setHeaderData(deptColumn, Qt::Horizontal, QStringLiteral("科室"));

    m_proxy = new DoctorFilterProxy(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setDynamicSortFilter(true);

    QList<int> keyColumns;
    keyColumns << m_model->fieldIndex(QStringLiteral("EMPLOYEE_NO"))
               << m_model->fieldIndex(QStringLiteral("NAME"))
               << deptColumn;
    m_proxy->setKeyColumns(keyColumns);

    m_view->setModel(m_proxy);
    m_view->setItemDelegate(new QSqlRelationalDelegate(m_view));
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->setSortingEnabled(true);
    m_view->sortByColumn(m_model->fieldIndex(QStringLiteral("EMPLOYEE_NO")), Qt::AscendingOrder);

    connect(m_proxy, &QAbstractItemModel::rowsInserted, this, [this] { updateState(); });
    connect(m_proxy, &QAbstractItemModel::rowsRemoved, this, [this] { updateState(); });
    connect(m_proxy, &QAbstractItemModel::modelReset, this, [this] { updateState(); });
}

void DoctorPage::buildUi()
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
    m_searchEdit->setPlaceholderText(QStringLiteral("工号 / 姓名 / 科室"));
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

    m_emptyLabel = new QLabel(QStringLiteral("暂无医生数据"), this);
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

    connect(searchAction, &QAction::triggered, this, &DoctorPage::applyFilter);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &DoctorPage::applyFilter);
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (m_proxy)
            m_proxy->setKeyword(text);
        updateState();
    });
    connect(addAction, &QAction::triggered, this, &DoctorPage::addDoctor);
    connect(editAction, &QAction::triggered, this, &DoctorPage::editDoctor);
    connect(deleteAction, &QAction::triggered, this, &DoctorPage::deleteDoctor);
    connect(refreshAction, &QAction::triggered, this, &DoctorPage::refresh);
    connect(backAction, &QAction::triggered, this, &DoctorPage::backRequested);
}

void DoctorPage::refresh()
{
    if (!DbManager::instance().isOpen()) {
        m_statusLabel->setText(QStringLiteral("数据库未连接，无法加载医生信息。"));
        m_emptyLabel->setText(QStringLiteral("数据库未连接"));
        m_stack->setCurrentIndex(1);
        return;
    }

    if (!m_model->select()) {
        QMessageBox::warning(this, QStringLiteral("查询失败"),
                             DbManager::translateError(m_model->lastError()));
        return;
    }
    // 关系列的表头可能在 select 后变成别名，这里重新设置一次
    const int deptColumn = m_model->fieldIndex(QStringLiteral("DEPARTMENT_ID"));
    if (deptColumn >= 0)
        m_model->setHeaderData(deptColumn, Qt::Horizontal, QStringLiteral("科室"));

    m_proxy->invalidate();
    m_view->resizeColumnsToContents();
    updateState();
}

void DoctorPage::applyFilter()
{
    m_proxy->setKeyword(m_searchEdit->text());
    updateState();
    if (DbManager::instance().isOpen() && !m_userId.isEmpty()) {
        DbManager::instance().logEvent(
            m_userId,
            QStringLiteral("查询医生：关键字“%1”，命中 %2 条")
                .arg(m_searchEdit->text().trimmed().isEmpty() ? QStringLiteral("（全部）")
                                                              : m_searchEdit->text().trimmed())
                .arg(m_proxy->rowCount()));
    }
}

void DoctorPage::addDoctor()
{
    DoctorEditDialog dialog(this);
    dialog.setInsertMode();
    if (dialog.exec() != QDialog::Accepted)
        return;
    if (!m_userId.isEmpty())
        DbManager::instance().logEvent(
            m_userId, QStringLiteral("新增医生 %1（%2）").arg(dialog.savedId(), dialog.savedName()));
    refresh();
}

void DoctorPage::editDoctor()
{
    QSqlRecord record;
    if (!currentRecord(&record)) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先在列表中选择要修改的医生。"));
        return;
    }
    DoctorEditDialog dialog(this);
    dialog.setUpdateRecord(record);
    if (dialog.exec() != QDialog::Accepted)
        return;
    if (!m_userId.isEmpty())
        DbManager::instance().logEvent(
            m_userId, QStringLiteral("修改医生 %1（%2）").arg(dialog.savedId(), dialog.savedName()));
    refresh();
}

void DoctorPage::deleteDoctor()
{
    QSqlRecord record;
    if (!currentRecord(&record)) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先在列表中选择要删除的医生。"));
        return;
    }
    const QString id = record.value(QStringLiteral("ID")).toString();
    const QString name = record.value(QStringLiteral("NAME")).toString();

    if (QMessageBox::question(this, QStringLiteral("确认删除"),
                              QStringLiteral("确定删除医生“%1”（工号 %2）吗？")
                                  .arg(name, record.value(QStringLiteral("EMPLOYEE_NO")).toString()),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
        != QMessageBox::Yes) {
        return;
    }

    QSqlQuery query(DbManager::instance().db());
    query.prepare(QStringLiteral("DELETE FROM Doctor WHERE ID = ?"));
    query.addBindValue(id);
    if (!query.exec()) {
        QMessageBox::critical(this, QStringLiteral("删除失败"),
                              DbManager::translateError(query.lastError()));
        return;
    }
    if (!m_userId.isEmpty())
        DbManager::instance().logEvent(m_userId,
                                       QStringLiteral("删除医生 %1（%2）").arg(id, name));
    refresh();
}

void DoctorPage::updateState()
{
    if (!m_proxy || !m_stack)
        return;
    const int count = m_proxy->rowCount();
    if (count == 0) {
        m_emptyLabel->setText(m_searchEdit->text().trimmed().isEmpty()
                                  ? QStringLiteral("暂无医生数据，点击“添加”录入。")
                                  : QStringLiteral("未找到匹配“%1”的医生。")
                                        .arg(m_searchEdit->text().trimmed()));
        m_stack->setCurrentIndex(1);
    } else {
        m_stack->setCurrentIndex(0);
    }
    m_statusLabel->setText(QStringLiteral("共 %1 名医生").arg(count));
}

bool DoctorPage::currentRecord(QSqlRecord *record) const
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
