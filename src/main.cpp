#include <QApplication>

#include "mainwindow.h"

using namespace global;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    cwd = getGlobalSettings().value("cwd").toString();

    MainWindow w;
    w.show();
    return a.exec();
}
