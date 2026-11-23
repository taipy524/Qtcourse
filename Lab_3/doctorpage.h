#ifndef DOCTORPAGE_H
#define DOCTORPAGE_H

#include <QWidget>
#include <QSqlRelationalTableModel>
#include <QSortFilterProxyModel>

class QLabel;
class QLineEdit;
class QTableView;
class QToolBar;
class QStackedWidget;

class DoctorFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit DoctorFilterProxy(QObject *parent = nullptr);

    void setKeyword(const QString &keyword);
    void setKeyColumns(const QList<int> &columns);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QString m_keyword;
    QList<int> m_columns;
};

class DoctorPage : public QWidget
{
    Q_OBJECT
public:
    explicit DoctorPage(QWidget *parent = nullptr);

    void setCurrentUser(const QString &userId) { m_userId = userId; }
    void refresh();

signals:
    void backRequested();

private slots:
    void applyFilter();
    void addDoctor();
    void editDoctor();
    void deleteDoctor();

private:
    void buildUi();
    void updateState();
    bool currentRecord(QSqlRecord *record) const;

    QSqlRelationalTableModel *m_model = nullptr;
    DoctorFilterProxy        *m_proxy = nullptr;
    QTableView               *m_view = nullptr;
    QLineEdit                *m_searchEdit = nullptr;
    QLabel                   *m_statusLabel = nullptr;
    QLabel                   *m_emptyLabel = nullptr;
    QStackedWidget           *m_stack = nullptr;

    QString m_userId;
};

#endif // DOCTORPAGE_H
