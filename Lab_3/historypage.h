#ifndef HISTORYPAGE_H
#define HISTORYPAGE_H

#include <QWidget>
#include <QSqlQueryModel>
#include <QSortFilterProxyModel>

class QComboBox;
class QLabel;
class QLineEdit;
class QTableView;
class QStackedWidget;

class HistoryFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit HistoryFilterProxy(QObject *parent = nullptr);

    void setKeyword(const QString &keyword);
    void setUserName(const QString &name);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QString m_keyword;
    QString m_userName;
};

class HistoryPage : public QWidget
{
    Q_OBJECT
public:
    explicit HistoryPage(QWidget *parent = nullptr);

    void setCurrentUser(const QString &userId) { m_userId = userId; }
    void refresh();

signals:
    void backRequested();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void applyFilter();

private:
    void buildUi();
    void updateState();
    void reloadUsers();

    QSqlQueryModel      *m_model = nullptr;
    HistoryFilterProxy  *m_proxy = nullptr;
    QTableView          *m_view = nullptr;
    QLineEdit           *m_searchEdit = nullptr;
    QComboBox           *m_userCombo = nullptr;
    QLabel              *m_statusLabel = nullptr;
    QLabel              *m_emptyLabel = nullptr;
    QStackedWidget      *m_stack = nullptr;

    QString m_userId;
};

#endif // HISTORYPAGE_H
