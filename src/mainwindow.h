#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QLabel>
#include <QMainWindow>
#include <QThreadPool>

#include "global.h"
#include "pages/pagehost.h"
#include "widgets/QProgressIndicator.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    bool eventFilter(QObject *watched, QEvent *event) override;

    struct TabData
    {
        RepoProject project;
        QWidget *page;
        bool isNewTab = false;
    };

private slots:
    void onAction();
    void closeTab(int index);
    void openProject(const RepoProject &project);

private:
    Ui::MainWindow *ui;
    QAction *m_actionPush;
    QAction *m_actionPull;
    QAction *m_actionFetch;
    QAction *m_actionReset;
    QAction *m_actionBranch;
    QAction *m_actionClean;
    QAction *m_actionTerm;
    QAction *m_actionFolder;
    QProgressIndicator *m_indicator;
    QLabel *m_statusLabel;

    void onActionSwitchManifest();
    void onActionOpen();
    void onActionRepoStart();
    void onProjectAction(void (PageHost::*func)());

    void openRepo();
    void parseManifest(const QString &manPath);
    void updateUI();
    int addTab(const RepoProject &project, bool isNewTab = false, int index = -1);
    void saveTabs();
    void restoreTabs();
};

#endif  // MAINWINDOW_H
