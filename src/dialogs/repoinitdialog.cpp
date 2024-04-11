#include "repoinitdialog.h"

#include <QAction>
#include <QDir>
#include <QFileDialog>
#include <QSettings>

#include "cmddialog.h"
#include "ui_repoinitdialog.h"

RepoInitDialog::RepoInitDialog(QWidget *parent) : QDialog(parent), ui(new Ui::RepoInitDialog)
{
    ui->setupUi(this);
    ui->urlEdit->setFocus();
    mFolderAction = ui->dirEdit->addAction(
        QIcon("://resources/action_folder.svg"), QLineEdit::TrailingPosition);
    connect(mFolderAction, &QAction::triggered, this, [this]() {
        QString path = QFileDialog::getExistingDirectory(this, "Select Init Directory");
        if (path.isEmpty()) {
            return;
        }
        ui->dirEdit->setText(path);
    });
}

RepoInitDialog::~RepoInitDialog()
{
    delete ui;
}

QString RepoInitDialog::initDir()
{
    return ui->dirEdit->text();
}

void RepoInitDialog::accept()
{
    QString dir = ui->dirEdit->text();
    if (dir.isEmpty() || !QDir(dir).exists()) {
        ui->dirEdit->setFocus();
        ui->dirEdit->selectAll();
        return;
    }

    QString cmd = QString("repo init%1%2%3%4")
                      .arg(ui->urlEdit->text().isEmpty() ? "" : " -u " + ui->urlEdit->text(),
                          ui->branchEdit->text().isEmpty() ? "" : " -b " + ui->branchEdit->text(),
                          ui->manEdit->text().isEmpty() ? "" : " -m " + ui->manEdit->text(),
                          ui->currentBranchCB->isChecked() ? " --current-branch" : "");

    int code = CmdDialog::execute(this, cmd, ui->dirEdit->text());
    done(code == 0 ? QDialog::Accepted : QDialog::Rejected);
}
