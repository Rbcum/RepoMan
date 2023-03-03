#include "deleterefdialog.h"

#include "dialogs/cmddialog.h"
#include "ui_deletelocalbranchdialog.h"
#include "ui_deletetagdialog.h"
#include "ui_deletremotebranchdialog.h"

DeleteLocalBranchDialog::DeleteLocalBranchDialog(
    QWidget *parent, const QString &projectPath, const QString &branch)
    : QDialog(parent),
      ui(new Ui::DeleteLocalBranchDialog),
      m_projectPath(projectPath),
      m_branch(branch)
{
    ui->setupUi(this);
    ui->branchLabel->setText(branch);
}

DeleteLocalBranchDialog::~DeleteLocalBranchDialog()
{
    delete ui;
}

void DeleteLocalBranchDialog::accept()
{
    QString cmd =
        QString("git branch %1 %2").arg(ui->forceCheckBox->isChecked() ? "-D" : "-d", m_branch);
    CmdDialog dialog(parentWidget(), cmd, m_projectPath, true);
    done(dialog.exec() == 0 ? QDialog::Accepted : QDialog::Rejected);
}

DeleteRemoteBranchDialog::DeleteRemoteBranchDialog(
    QWidget *parent, const QString &projectPath, const QString &branch)
    : QDialog(parent),
      ui(new Ui::DeleteRemoteBranchDialog),
      m_projectPath(projectPath),
      m_branch(branch)
{
    ui->setupUi(this);
    ui->branchLabel->setText(branch);
}

DeleteRemoteBranchDialog::~DeleteRemoteBranchDialog()
{
    delete ui;
}

void DeleteRemoteBranchDialog::accept()
{
    QString cmd = QString("git push origin :%1").arg(m_branch);
    CmdDialog dialog(parentWidget(), cmd, m_projectPath, true);
    done(dialog.exec() == 0 ? QDialog::Accepted : QDialog::Rejected);
}

DeleteTagDialog::DeleteTagDialog(QWidget *parent, const QString &projectPath, const QString &tag)
    : QDialog(parent), ui(new Ui::DeleteTagDialog), m_projectPath(projectPath), m_tag(tag)
{
    ui->setupUi(this);
    ui->tagLabel->setText(tag);
}

DeleteTagDialog::~DeleteTagDialog()
{
    delete ui;
}

void DeleteTagDialog::accept()
{
    QString cmd = QString("git tag -d %1").arg(m_tag);
    CmdDialog dialog(parentWidget(), cmd, m_projectPath, true);
    done(dialog.exec() == 0 ? QDialog::Accepted : QDialog::Rejected);
}
