#include "checkoutdialog.h"

#include "cmddialog.h"
#include "ui_checkoutdialog.h"

CheckoutDialog::CheckoutDialog(QWidget *parent, const QString &projectPath, const Commit &commit)
    : QDialog(parent), ui(new Ui::CheckoutDialog), m_projectPath(projectPath), m_commit(commit)
{
    ui->setupUi(this);
    ui->commitLabel->setText("<b>" + commit.shortHash + "</b> " + commit.subject);
}

CheckoutDialog::~CheckoutDialog()
{
    delete ui;
}

void CheckoutDialog::accept()
{
    QString cmd = QString("git checkout %1").arg(m_commit.hash);
    CmdDialog dialog(parentWidget(), cmd, m_projectPath, true);
    done(dialog.exec() == 0 ? QDialog::Accepted : QDialog::Rejected);
}
