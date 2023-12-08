#include "fetchdialog.h"

#include <QDir>
#include <QSettings>

#include "cmddialog.h"
#include "ui_fetchdialog.h"

FetchDialog::FetchDialog(QWidget *parent, const QString &projectPath)
    : QDialog(parent), ui(new Ui::FetchDialog), m_projectPath(projectPath)
{
    ui->setupUi(this);

    QString confFile = QDir::cleanPath(m_projectPath + "/.git/config");
    QSettings gitConfig(confFile, QSettings::NativeFormat);
    for (const QString &group : gitConfig.childGroups()) {
        if (group.startsWith("remote")) {
            QString name = group.sliced(8).chopped(1);
            QString url = gitConfig.value(group + "/url").toString();
            ui->remoteComboBox->addItem(name, url);
        }
    }
}

FetchDialog::~FetchDialog()
{
    delete ui;
}

void FetchDialog::accept()
{
    QString cmd =
        QString("git fetch%1%2 %3")
            .arg(ui->pruneCB->isChecked() ? " --prune" : "",
                ui->tagsCB->isChecked() ? " --tags" : "", ui->remoteComboBox->currentText());
    int code = CmdDialog::execute(parentWidget(), cmd, m_projectPath);
    done(code == 0 ? QDialog::Accepted : QDialog::Rejected);
}
