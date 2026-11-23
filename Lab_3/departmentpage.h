#ifndef DEPARTMENTPAGE_H
#define DEPARTMENTPAGE_H

#include <QWidget>
#include <QSqlTableModel>
#include <QSortFilterProxyModel>

class QLabel;
class QLineEdit;
class QTableView;
class QStackedWidget;

class DepartmentFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit DepartmentFilterProxy(QObject *parent = nullptr);

    void setKeyword(const QString &keyword);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QString m_keyword;
};

class DepartmentPage : public QWidget
{
    Q_OBJECT
public:
    explicit DepartmentPage(QWidget *parent = nullptr);

    void setCurrentUser(const QString &userId) { m_userId = userId; }
    void refresh();

signals:
    void backRequested();

private slots:
    void applyFilter();
    void addDepartment();
    void editDepartment();
    void deleteDepartment();

private:
    void buildUi();
    void updateState();
    bool currentRecord(QSqlRecord *record) const;
    int doctorCount(const QString &departmentId);

    QSqlTableModel         *m_model = nullptr;
    DepartmentFilterProxy  *m_proxy = nullptr;
    QTableView             *m_view = nullptr;
    QLineEdit              *m_searchEdit = nullptr;
    QLabel                 *m_statusLabel = nullptr;
    QLabel                 *m_emptyLabel = nullptr;
    QStackedWidget         *m_stack = nullptr;

    QString m_userId;
};

#endif // DEPARTMENTPAGE_H
