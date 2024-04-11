#ifndef REPOINITDIALOG_H
#define REPOINITDIALOG_H

#include <QDialog>

namespace Ui {
    class RepoInitDialog;
}

class RepoInitDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RepoInitDialog(QWidget *parent = nullptr);
    ~RepoInitDialog();
    QString initDir();

public slots:
    void accept() override;

private:
    Ui::RepoInitDialog *ui;

    QAction *mFolderAction;
};

#endif  // REPOINITDIALOG_H
