#include "pulldialog.h"

#include <QDir>
#include <QSettings>
#include <QtConcurrent>

#include "cmddialog.h"
#include "global.h"
#include "ui_pulldialog.h"

PullDialog::PullDialog(QWidget *parent, const QString &projectPath)
    : QDialog(parent), ui(new Ui::PullDialog), m_projectPath(projectPath)
{
    ui->setupUi(this);
    m_indicator = new QProgressIndicator(this);

    QString confFile = QDir::cleanPath(m_projectPath + "/.git/config");
    QSettings gitConfig(confFile, QSettings::NativeFormat);
    for (const QString &group : gitConfig.childGroups()) {
        if (group.startsWith("remote")) {
            QString name = group.sliced(8).chopped(1);
            QString url = gitConfig.value(group + "/url").toString();
            ui->remoteBox->addItem(name, url);
        }
    }
    connect(ui->remoteBox, &QComboBox::currentIndexChanged, this, &PullDialog::updateBranchUI);

    setEnabled(false);
    m_indicator->startHint();
    QtConcurrent::run([projectPath]() {
        InitData data;
        QStringList lines = global::getCmdResult("git branch -a", projectPath).split('\n');
        for (const QString &line : lines) {
            QString trLine = line.trimmed();
            if (trLine.size() > 0) {
                if (trLine.contains(" -> ")) {
                    trLine = trLine.split(" -> ").first();
                }
                if (trLine.startsWith("*")) {
                    trLine = trLine.mid(2);
                    data.head = trLine;
                }
                if (trLine.startsWith("remotes/")) {
                    trLine = trLine.mid(QString("remotes/").size());
                    int slashIdx = trLine.indexOf("/");
                    QStringList &branches = data.remoteBranches[trLine.first(slashIdx)];
                    branches << trLine.mid(slashIdx + 1);
                }
            }
        }
        return data;
    }).then(this, [this](InitData data) {
        ui->localLabel->setText(data.head);
        m_remoteBranches = data.remoteBranches;
        updateBranchUI();
        setEnabled(true);
        m_indicator->stopHint();
    });
}

PullDialog::~PullDialog()
{
    delete ui;
}

void PullDialog::updateBranchUI()
{
    ui->branchBox->clear();
    ui->branchBox->addItems(m_remoteBranches[ui->remoteBox->currentText()]);
    ui->branchBox->setCurrentText(ui->localLabel->text());
}

void PullDialog::accept()
{
    QString cmd = QString("git pull%1 %2 %3")
                      .arg(ui->rebaseCB->isChecked() ? " --rebase" : "",
                          ui->remoteBox->currentText(), ui->branchBox->currentText());
    CmdDialog dialog(parentWidget(), cmd, m_projectPath);
    done(dialog.exec() == 0 ? QDialog::Accepted : QDialog::Rejected);
}
