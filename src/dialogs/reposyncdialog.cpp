#include "reposyncdialog.h"

#include <QSettings>
#include <QThread>

#include "cmddialog.h"
#include "ui_reposyncdialog.h"

RepoSyncDialog::RepoSyncDialog(QWidget *parent) : QDialog(parent), ui(new Ui::RepoSyncDialog)
{
    ui->setupUi(this);
    ui->jobsSpin->setValue(QThread::idealThreadCount() * 2);
    connect(ui->localOnyCB, &QCheckBox::stateChanged, this, &RepoSyncDialog::onCheckStateChanged);
    connect(
        ui->networkOnlyCB, &QCheckBox::stateChanged, this, &RepoSyncDialog::onCheckStateChanged);
}

RepoSyncDialog::~RepoSyncDialog()
{
    delete ui;
}

void RepoSyncDialog::accept()
{
    QString cmd =
        QString("repo sync -j%1%2%3%4%5%6%7%8")
            .arg(QString::number(ui->jobsSpin->value()),
                ui->forceCB->isChecked() ? " --force-sync" : "",
                ui->detachCB->isChecked() ? " -d" : "", ui->localOnyCB->isChecked() ? " -l" : "",
                ui->networkOnlyCB->isChecked() ? " -n" : "",
                ui->pruneCB->isChecked() ? " --prune" : "",
                ui->currentBranchCB->isChecked() ? " -c" : "",
                ui->noTagsCB->isChecked() ? " --no-tags" : "");
    QString cwd = QSettings().value("cwd").toString();
    int code = CmdDialog::execute(parentWidget(), cmd, cwd);
    done(code == 0 ? QDialog::Accepted : QDialog::Rejected);
}

void RepoSyncDialog::onCheckStateChanged(int state)
{
    if (state == Qt::Checked) {
        auto uncheckCB = sender() == ui->localOnyCB ? ui->networkOnlyCB : ui->localOnyCB;
        uncheckCB->setChecked(false);
    }
}
