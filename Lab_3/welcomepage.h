#ifndef WELCOMEPAGE_H
#define WELCOMEPAGE_H

#include <QWidget>

class QLabel;
class QPushButton;

class WelcomePage : public QWidget
{
    Q_OBJECT
public:
    explicit WelcomePage(QWidget *parent = nullptr);

    void setCurrentUser(const QString &fullName);

signals:
    void patientsRequested();
    void doctorsRequested();
    void departmentsRequested();
    void historyRequested();
    void logoutRequested();

private:
    QPushButton *createEntryButton(const QString &text, const QString &description,
                                   const QString &color, const QString &iconPath);

    QLabel *m_userLabel = nullptr;
};

#endif // WELCOMEPAGE_H
