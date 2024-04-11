#include <QApplication>

#include "mainwindow.h"

using namespace global;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    auto cwd = QSettings().value("cwd").toString();
    MainWindow w(cwd);
    w.show();

    return a.exec();
}
