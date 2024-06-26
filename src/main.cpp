#include <themes/theme.h>
#include <themes/theme_p.h>

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include "mainwindow.h"
#include "themes/repomanstyle.h"

using namespace global;
using namespace utils;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QSettings settings;

    a.setStyle(new RepoManStyle());
    a.setStyleSheet(" ");  // For TabBarEx setStyle propagation, see QWidget::setStyle

    QString themeFile =
        QString(":/themes/%1.theme").arg(settings.value("theme", "light").toString());
    // themeFile = ":/themes/light.theme";
    QSettings themeSettings(themeFile, QSettings::IniFormat);
    Theme *theme = new Theme("default");
    theme->readSettings(themeSettings);
    Theme::setInitialPalette(theme);  // Initialize palette before setting it
    setCreatorTheme(theme);

    auto cwd = settings.value("cwd").toString();
    MainWindow w(cwd);
    w.show();

    return a.exec();
}
