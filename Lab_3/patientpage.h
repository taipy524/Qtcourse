#ifndef PATIENTPAGE_H
#define PATIENTPAGE_H

#include <QWidget>
#include <QSqlTableModel>
#include <QSortFilterProxyModel>

class QLabel;
class QLineEdit;
class QTableView;
class QToolBar;
class QStackedWidget;

// 只允许在表格内直接编辑“性别”列，其余字段统一用对话框编辑
class PatientTableModel : public QSqlTableModel
{
    Q_OBJECT
public:
    PatientTableModel(QObject *parent, const QSqlDatabase &db);

    Qt::ItemFlags flags(const QModelIndex &index) const override;
};

// 姓名 / 身份证 / 手机号 / ID 多列模糊过滤
class PatientFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit PatientFilterProxy(QObject *parent = nullptr);

    void setKeyword(const QString &keyword);
    void setKeyColumns(const QList<int> &columns);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QString m_keyword;
    QList<int> m_columns;
};

class PatientPage : public QWidget
{
    Q_OBJECT
public:
    explicit PatientPage(QWidget *parent = nullptr);

    void setCurrentUser(const QString &userId) { m_userId = userId; }
    void refresh();

signals:
    void backRequested();

private slots:
    void applyFilter();
    void addPatient();
    void editPatient();
    void deletePatient();
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);

private:
    void buildUi();
    void updateState();
    bool currentRecord(QSqlRecord *record) const;

    PatientTableModel  *m_model = nullptr;
    PatientFilterProxy *m_proxy = nullptr;
    QTableView         *m_view = nullptr;
    QToolBar           *m_toolbar = nullptr;
    QLineEdit          *m_searchEdit = nullptr;
    QLabel             *m_statusLabel = nullptr;
    QLabel             *m_emptyLabel = nullptr;
    QStackedWidget     *m_stack = nullptr;
    int                 m_sexColumn = -1;

    QString m_userId;
    QString m_lastSelectedId;
};

#endif // PATIENTPAGE_H
