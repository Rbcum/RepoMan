#include "reposyncdialog.h"

#include <QSettings>
#include <QThread>

#include "cmddialog.h"
#include "ui_reposyncdialog.h"

RepoSyncDialog::RepoSyncDialog(QWidget *parent, const RepoContext &context, int currentIndex,
    const QList<Project> &projects)
    : QDialog(parent),
      ui(new Ui::RepoSyncDialog),
      m_context(context),
      m_currentIndex(currentIndex),
      m_projects(projects)
{
    ui->setupUi(this);
    ui->jobsSpin->setValue(QThread::idealThreadCount() * 2);
    connect(ui->localOnyCB, &QCheckBox::stateChanged, this, &RepoSyncDialog::onCheckStateChanged);
    connect(
        ui->networkOnlyCB, &QCheckBox::stateChanged, this, &RepoSyncDialog::onCheckStateChanged);

    if (currentIndex < 0) {
        ui->syncForCombo->setItemData(0, Qt::NoItemFlags, Qt::UserRole - 1);
    }
    if (projects.size() <= 0) {
        ui->syncForCombo->setItemData(1, Qt::NoItemFlags, Qt::UserRole - 1);
    }
}

RepoSyncDialog::~RepoSyncDialog()
{
    delete ui;
}

void RepoSyncDialog::accept()
{
    QString projectsArg;
    switch (ui->syncForCombo->currentIndex()) {
        case 0:
            projectsArg += " ";
            projectsArg += m_projects[m_currentIndex].path;
            break;
        case 1:
            for (auto &p : m_projects) {
                projectsArg += " ";
                projectsArg += p.path;
            }
            break;
    }
    QString cmd =
        QString("repo sync -j%1%2%3%4%5%6%7%8%9")
            .arg(QString::number(ui->jobsSpin->value()),
                ui->forceCB->isChecked() ? " --force-sync" : "",
                ui->detachCB->isChecked() ? " -d" : "", ui->localOnyCB->isChecked() ? " -l" : "",
                ui->networkOnlyCB->isChecked() ? " -n" : "",
                ui->pruneCB->isChecked() ? " --prune" : "",
                ui->currentBranchCB->isChecked() ? " -c" : "",
                ui->noTagsCB->isChecked() ? " --no-tags" : "", projectsArg);
    int code = CmdDialog::execute(parentWidget(), cmd, m_context.repoPath());
    done(code == 0 ? QDialog::Accepted : QDialog::Rejected);
}

void RepoSyncDialog::onCheckStateChanged(int state)
{
    if (state == Qt::Checked) {
        auto uncheckCB = sender() == ui->localOnyCB ? ui->networkOnlyCB : ui->localOnyCB;
        uncheckCB->setChecked(false);
    }
}
