#ifndef REPOSYNCDIALOG_H
#define REPOSYNCDIALOG_H

#include <QDialog>

namespace Ui {
    class RepoSyncDialog;
}

class RepoSyncDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RepoSyncDialog(QWidget *parent = nullptr);
    ~RepoSyncDialog();

public slots:
    void accept() override;

private slots:
    void onCheckStateChanged(int state);

private:
    Ui::RepoSyncDialog *ui;
};

#endif  // REPOSYNCDIALOG_H
