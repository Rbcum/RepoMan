#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QComboBox>
#include <QLabel>
#include <QMainWindow>
#include <QThreadPool>

#include "pages/pagehost.h"
#include "repocontext.h"
#include "widgets/QProgressIndicator.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QString repoPath = "");
    ~MainWindow();
    bool eventFilter(QObject *watched, QEvent *event) override;

    struct TabData
    {
        Project project;
        QWidget *page;
        bool isNewTab = false;
    };

private slots:
    void onAction();
    void closeTab(int index);
    void openProject(const Project &project);

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
    QAction *m_actionRepoSwitchManifest;
    QAction *m_actionRepoSync;
    QProgressIndicator *m_indicator;
    QLabel *m_statusLabel;
    QComboBox *m_projectCombo;

    RepoContext m_context;

    void onActionOpen();
    void onActionRepoStart();
    void onActionRepoSync();
    void onProjectAction(void (PageHost::*func)());

    void updateUI();
    int addTab(const Project &project, bool isNewTab = false, int index = -1);
    void saveTabs();
    void restoreTabs();
    void closeAllTabs();
    void openRepo(const QString &path);

protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif  // MAINWINDOW_H
