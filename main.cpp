#include "mainwindow.h"
#include <QApplication>
#include <QFile>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 设置全局标识，这样 QSettings 才知道去注册表哪个位置读写
    QCoreApplication::setOrganizationName("Luxuryend_Studio");
    QCoreApplication::setApplicationName("MusicPlayer");

    QFile file(":/style.qss");
    if (file.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(file.readAll());
        a.setStyleSheet(styleSheet);
        file.close();
    }

    MainWindow w;
    w.show();
    return a.exec();
}
