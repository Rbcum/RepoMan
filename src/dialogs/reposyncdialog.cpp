#include "reposyncdialog.h"

#include <QSettings>

#include "cmddialog.h"
#include "global.h"
#include "ui_reposyncdialog.h"

RepoSyncDialog::RepoSyncDialog(QWidget *parent) : QDialog(parent), ui(new Ui::RepoSyncDialog)
{
    ui->setupUi(this);
    ui->jobsSpin->setValue(global::manifest.syncJ);
}

RepoSyncDialog::~RepoSyncDialog()
{
    delete ui;
}

void RepoSyncDialog::accept()
{
    QString cmd =
        QString("repo sync -f -j%1%2%3%4%5%6%7")
            .arg(QString::number(ui->jobsSpin->value()),
                ui->forceCB->isChecked() ? " --force-sync" : "",
                ui->detachCB->isChecked() ? " -d" : "", ui->localOnyCB->isChecked() ? " -l" : "",
                ui->networkOnlyCB->isChecked() ? " -n" : "",
                ui->pruneCB->isChecked() ? " --prune" : "",
                ui->verboseBox->isChecked() ? " -v" : "");
    QString cwd = QSettings().value("cwd").toString();
    CmdDialog dialog(parentWidget(), cmd, cwd);
    done(dialog.exec() == 0 ? QDialog::Accepted : QDialog::Rejected);
}
