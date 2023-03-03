#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QProgressBar>
#include <QSortFilterProxyModel>

#include "pages/changespage.h"
#include "pages/historypage.h"

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
    //    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void onAction();
    void onRefClicked(const QModelIndex &index);
    void onChangeMode();
    void onProjectSelected();
    void refresh(const HistorySelectionArg &arg = HistorySelectionArg());

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
    QComboBox *m_projectListBox;
    QProgressIndicator *m_indicator;
    QLabel *m_statusLabel;

    ChangesPage *m_changesPage;
    HistoryPage *m_historyPage;

    void onActionSwitchManifest();
    void onActionOpen();
    void onActionRepoStart();

    void openRepo();
    void parseManifest(const QString &manPath);
    void updateUI();
    QString getProjectFullPath(const RepoProject &project);

    QThreadPool *m_threadPool;
};

#endif  // MAINWINDOW_H
