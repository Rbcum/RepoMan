#ifndef BRANCHDIALOG_H
#define BRANCHDIALOG_H

#include <QComboBox>
#include <QDialog>

#include "global.h"

namespace Ui {
    class BranchRemoteDialog;
    class BranchCommitDialog;
}  // namespace Ui

class BranchRemoteDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BranchRemoteDialog(
        QWidget *parent, const QString &projectPath, const QString &remoteBranch);
    ~BranchRemoteDialog();

public slots:
    void accept() override;

private:
    Ui::BranchRemoteDialog *ui;
    QString m_projectPath;
    QString m_remoteBranch;
};

class BranchCommitDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BranchCommitDialog(QWidget *parent, const QString &projectPath, const Commit &commit);
    ~BranchCommitDialog();

public slots:
    void accept() override;

private:
    Ui::BranchCommitDialog *ui;
    QString m_projectPath;
    Commit m_commit;
};

#endif  // BRANCHDIALOG_H
