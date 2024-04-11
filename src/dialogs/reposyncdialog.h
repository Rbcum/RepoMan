#ifndef REPOSYNCDIALOG_H
#define REPOSYNCDIALOG_H
#include <QDialog>

#include "repocontext.h"

namespace Ui {
    class RepoSyncDialog;
}

class RepoSyncDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RepoSyncDialog(QWidget *parent, const RepoContext &context, int currentIndex,
        const QList<Project> &projects = QList<Project>());
    ~RepoSyncDialog();

public slots:
    void accept() override;

private slots:
    void onCheckStateChanged(int state);

private:
    Ui::RepoSyncDialog *ui;
    RepoContext m_context;
    QList<Project> m_projects;
    int m_currentIndex;
};

#endif  // REPOSYNCDIALOG_H
