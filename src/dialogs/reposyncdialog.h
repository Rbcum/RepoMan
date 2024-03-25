#ifndef REPOSYNCDIALOG_H
#define REPOSYNCDIALOG_H
#include <QDialog>

#include "global.h"

namespace Ui {
    class RepoSyncDialog;
}

class RepoSyncDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RepoSyncDialog(QWidget *parent, int currentIndex,
        const QList<RepoProject> &projects = QList<RepoProject>());
    ~RepoSyncDialog();

public slots:
    void accept() override;

private slots:
    void onCheckStateChanged(int state);

private:
    Ui::RepoSyncDialog *ui;
    QList<RepoProject> m_projects;
    int m_currentIndex;
};

#endif  // REPOSYNCDIALOG_H
