#include "mainwindow.h"

#include <QApplication>
#include <QFont>

int main(int argc, char *argv[])
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("HospitalLab3"));
    app.setOrganizationName(QStringLiteral("DGUT"));

    // Windows 下统一使用微软雅黑，避免默认字体导致中文界面观感不一致
    QFont appFont(QStringLiteral("Microsoft YaHei UI"));
    appFont.setPointSize(10);
    app.setFont(appFont);

    MainWindow window;
    window.show();

    return app.exec();
}
