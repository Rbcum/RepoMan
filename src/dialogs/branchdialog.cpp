#include "branchdialog.h"

#include <QLineEdit>

#include "cmddialog.h"
#include "global.h"
#include "ui_branchcommitdialog.h"
#include "ui_branchremotedialog.h"

//    QString cmd;
//    if (ui->typeGroup->checkedButton() == ui->existingBrRB) {
//        cmd = QString("git checkout %1").arg(m_selectBox->currentText());
//    } else {
//        cmd = QString("git checkout -b %1 --track origin/%2")
//                  .arg(m_newBranchEdit->text(), m_trackBranchBox->currentText());
//    }
//    CmdDialog dialog(parentWidget(), cmd, m_projectPath);
//    dialog.exec();

BranchRemoteDialog::BranchRemoteDialog(
    QWidget *parent, const QString &projectPath, const QString &remoteBranch)
    : QDialog(parent),
      ui(new Ui::BranchRemoteDialog),
      m_projectPath(projectPath),
      m_remoteBranch(remoteBranch)
{
    ui->setupUi(this);
    ui->remoteBranchLabel->setText(remoteBranch);
    ui->branchNameEdit->setText(remoteBranch.split("/").last());
    ui->branchNameEdit->selectAll();
}

BranchRemoteDialog::~BranchRemoteDialog()
{
    delete ui;
}

void BranchRemoteDialog::accept()
{
    QString cmd =
        QString("git checkout -b %1 --track %2").arg(ui->branchNameEdit->text(), m_remoteBranch);
    int code = CmdDialog::execute(parentWidget(), cmd, m_projectPath, true);
    done(code == 0 ? QDialog::Accepted : QDialog::Rejected);
}

BranchCommitDialog::BranchCommitDialog(
    QWidget *parent, const QString &projectPath, const Commit &commit)
    : QDialog(parent), ui(new Ui::BranchCommitDialog), m_projectPath(projectPath), m_commit(commit)
{
    ui->setupUi(this);
    ui->commitLabel->setText("<b>" + commit.shortHash + "</b> " + commit.subject);
}

BranchCommitDialog::~BranchCommitDialog()
{
    delete ui;
}

void BranchCommitDialog::accept()
{
    QStringList cmds;
    cmds << QString("git branch %1 %2").arg(ui->branchNameEdit->text(), m_commit.hash);
    if (ui->checkoutBox->isChecked()) {
        cmds << "git checkout " + ui->branchNameEdit->text();
    }
    int code = CmdDialog::execute(parentWidget(), cmds, m_projectPath, true);
    done(code == 0 ? QDialog::Accepted : QDialog::Rejected);
}
