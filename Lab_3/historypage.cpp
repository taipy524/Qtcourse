#include "historypage.h"
#include "dbmanager.h"

#include <QAction>
#include <QComboBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QShowEvent>
#include <QSqlError>
#include <QSqlQuery>
#include <QStackedWidget>
#include <QTableView>
#include <QToolBar>
#include <QVBoxLayout>

HistoryFilterProxy::HistoryFilterProxy(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

void HistoryFilterProxy::setKeyword(const QString &keyword)
{
    m_keyword = keyword.trimmed();
    invalidateFilter();
}

void HistoryFilterProxy::setUserName(const QString &name)
{
    m_userName = name;
    invalidateFilter();
}

bool HistoryFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const QAbstractItemModel *model = sourceModel();
    if (!model)
        return true;

    const QString rowUser = model->index(sourceRow, 0, sourceParent).data().toString();
    if (!m_userName.isEmpty() && rowUser != m_userName)
        return false;

    if (m_keyword.isEmpty())
        return true;

    for (int column = 0; column < model->columnCount(); ++column) {
        const QModelIndex index = model->index(sourceRow, column, sourceParent);
        if (index.data(Qt::DisplayRole).toString().contains(m_keyword, Qt::CaseInsensitive))
            return true;
    }
    return false;
}

HistoryPage::HistoryPage(QWidget *parent)
    : QWidget(parent)
{
    buildUi();

    m_model = new QSqlQueryModel(this);
    m_proxy = new HistoryFilterProxy(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setDynamicSortFilter(true);

    m_view->setModel(m_proxy);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->setSortingEnabled(true);
    m_view->horizontalHeader()->setStretchLastSection(true);

    connect(m_proxy, &QAbstractItemModel::modelReset, this, [this] { updateState(); });
}

void HistoryPage::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(12, 10, 12, 8);
    root->setSpacing(8);

    auto *toolbar = new QToolBar(this);
    toolbar->setMovable(false);
    toolbar->setFloatable(false);
    toolbar->setIconSize(QSize(20, 20));
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    m_userCombo = new QComboBox(this);
    m_userCombo->setMinimumWidth(140);
    m_userCombo->addItem(QStringLiteral("全部用户"), QString());

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(QStringLiteral("操作内容 / 操作人"));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setMinimumWidth(220);

    toolbar->addWidget(new QLabel(QStringLiteral(" 用户："), this));
    toolbar->addWidget(m_userCombo);
    toolbar->addWidget(new QLabel(QStringLiteral(" 关键字："), this));
    toolbar->addWidget(m_searchEdit);

    QAction *searchAction = toolbar->addAction(QIcon(QStringLiteral(":/icons/search.png")),
                                               QStringLiteral("查找"));
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

    m_emptyLabel = new QLabel(QStringLiteral("暂无操作日志"), this);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setStyleSheet(QStringLiteral("color:#888;font-size:16px;background:#fafafa;"));

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(m_view);
    m_stack->addWidget(m_emptyLabel);

    m_statusLabel = new QLabel(QStringLiteral("未加载"), this);
    m_statusLabel->setStyleSheet(QStringLiteral("color:#555;"));

    root->addWidget(m_stack, 1);
    root->addWidget(m_statusLabel);

    connect(searchAction, &QAction::triggered, this, &HistoryPage::applyFilter);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &HistoryPage::applyFilter);
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (m_proxy)
            m_proxy->setKeyword(text);
        updateState();
    });
    connect(m_userCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        if (m_proxy)
            m_proxy->setUserName(m_userCombo->currentData().toString());
        updateState();
    });
    connect(refreshAction, &QAction::triggered, this, &HistoryPage::refresh);
    connect(backAction, &QAction::triggered, this, &HistoryPage::backRequested);
}

void HistoryPage::reloadUsers()
{
    const QString current = m_userCombo->currentData().toString();
    m_userCombo->blockSignals(true);
    m_userCombo->clear();
    m_userCombo->addItem(QStringLiteral("全部用户"), QString());

    QSqlQuery query(DbManager::instance().db());
    if (query.exec(QStringLiteral("SELECT DISTINCT USER_NAME FROM v_history "
                                  "WHERE USER_NAME IS NOT NULL ORDER BY USER_NAME"))) {
        while (query.next())
            m_userCombo->addItem(query.value(0).toString(), query.value(0).toString());
    }
    const int index = m_userCombo->findData(current);
    m_userCombo->setCurrentIndex(index >= 0 ? index : 0);
    m_userCombo->blockSignals(false);
}

void HistoryPage::refresh()
{
    if (!DbManager::instance().isOpen()) {
        m_statusLabel->setText(QStringLiteral("数据库未连接，无法加载操作日志。"));
        m_emptyLabel->setText(QStringLiteral("数据库未连接"));
        m_stack->setCurrentIndex(1);
        return;
    }

    m_model->setQuery(QStringLiteral(
        "SELECT USER_NAME AS `操作人`, EVENT AS `操作内容`, `TIMESTAMP` AS `操作时间` "
        "FROM v_history ORDER BY `TIMESTAMP` DESC"),
        DbManager::instance().db());

    if (m_model->lastError().isValid()) {
        QMessageBox::warning(this, QStringLiteral("查询失败"),
                             DbManager::translateError(m_model->lastError()));
        return;
    }

    reloadUsers();
    m_proxy->invalidate();
    m_view->resizeColumnsToContents();
    m_view->horizontalHeader()->setStretchLastSection(true);
    updateState();
}

void HistoryPage::applyFilter()
{
    m_proxy->setKeyword(m_searchEdit->text());
    m_proxy->setUserName(m_userCombo->currentData().toString());
    updateState();
    if (!m_userId.isEmpty()) {
        DbManager::instance().logEvent(
            m_userId,
            QStringLiteral("查询操作日志：用户“%1”，关键字“%2”，命中 %3 条")
                .arg(m_userCombo->currentText(),
                     m_searchEdit->text().trimmed().isEmpty() ? QStringLiteral("（全部）")
                                                              : m_searchEdit->text().trimmed())
                .arg(m_proxy->rowCount()));
    }
}

void HistoryPage::updateState()
{
    if (!m_proxy || !m_stack)
        return;
    const int count = m_proxy->rowCount();
    if (count == 0) {
        m_emptyLabel->setText(QStringLiteral("未找到符合条件的操作日志。"));
        m_stack->setCurrentIndex(1);
    } else {
        m_stack->setCurrentIndex(0);
    }
    m_statusLabel->setText(QStringLiteral("共 %1 条日志（按时间倒序）").arg(count));
}

void HistoryPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (DbManager::instance().isOpen())
        refresh();
}
