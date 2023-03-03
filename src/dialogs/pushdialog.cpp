#include "pushdialog.h"

#include <QAbstractItemView>
#include <QDir>
#include <QProcess>
#include <QtConcurrent>

#include "cmddialog.h"
#include "global.h"
#include "ui_pushdialog.h"

PushDialog::PushDialog(QWidget *parent, const QString &projectPath)
    : QDialog(parent), ui(new Ui::PushDialog), m_projectPath(projectPath)
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

    ui->localBranchBox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->targetBranchBox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    connect(ui->remoteBox, &QComboBox::currentIndexChanged, this, &PushDialog::updateUrlUI);
    connect(
        ui->remoteBox, &QComboBox::currentIndexChanged, this, &PushDialog::updateTargetBranchUI);
    connect(ui->replaceHostCB, &QCheckBox::stateChanged, this, [&](int state) {
        QSettings().setValue(global::getRepoSettingsKey("replaceHostChecked"), state == Qt::Checked);
        updateUrlUI();
    });
    connect(ui->replaceHostEdit, &QLineEdit::textChanged, this, [&](const QString &text) {
        QSettings().setValue(global::getRepoSettingsKey("replaceHost"), text);
        updateUrlUI();
    });
    connect(ui->localBranchBox, &QComboBox::currentIndexChanged, this,
        &PushDialog::updateTargetBranchUI);

    updateUrlUI();
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
                if (!trLine.startsWith("remotes/")) {
                    data.localBranches << trLine;
                } else {
                    trLine = trLine.mid(QString("remotes/").size());
                    int slashIdx = trLine.indexOf("/");
                    QStringList &branches = data.remoteBranches[trLine.first(slashIdx)];
                    if (branches.isEmpty()) {
                        branches << "";
                    }
                    branches << trLine.mid(slashIdx + 1);
                }
            }
        }
        return data;
    }).then(this, [this](InitData data) {
        ui->localBranchBox->addItems(data.localBranches);
        ui->localBranchBox->setCurrentText(data.head);
        m_remoteBranches = data.remoteBranches;
        updateTargetBranchUI();
        setEnabled(true);
        m_indicator->stopHint();
    });
}

PushDialog::~PushDialog()
{
    delete ui;
}

void PushDialog::updateUrlUI()
{
    QSettings settings;
    bool useEditHost = settings.value(global::getRepoSettingsKey("replaceHostChecked"), false).toBool();
    QString editHost = settings.value(global::getRepoSettingsKey("replaceHost")).toString();
    ui->replaceHostCB->setChecked(useEditHost);
    ui->replaceHostEdit->setText(editHost);
    ui->replaceHostEdit->setEnabled(useEditHost);

    ui->urlTextEdit->clear();
    QString url = ui->remoteBox->currentData().toString();
    if (useEditHost) {
        QUrl qUrl(url);
        QString prefix = qUrl.scheme() + "://" + qUrl.authority();
        QString newUrl = QString(url).replace(0, prefix.length(), editHost);
        ui->urlTextEdit->insertPlainText(newUrl);
    } else {
        ui->urlTextEdit->insertPlainText(url);
    }
}

void PushDialog::updateTargetBranchUI()
{
    ui->targetBranchBox->clear();
    ui->targetBranchBox->addItems(m_remoteBranches[ui->remoteBox->currentText()]);
    ui->targetBranchBox->setCurrentText(ui->localBranchBox->currentText());
}

void PushDialog::accept()
{
    QString targetBranch = ui->targetBranchBox->currentText().isEmpty()
                               ? ui->localBranchBox->currentText()
                               : ui->targetBranchBox->currentText();
    QString cmd = QString("git push %1 HEAD:%1%2")
                      .arg(ui->refForCheckBox->isChecked() ? "refs/for/" : "",
                          ui->urlTextEdit->toPlainText(), targetBranch);
    CmdDialog dialog(parentWidget(), cmd, m_projectPath);
    done(dialog.exec() == 0 ? QDialog::Accepted : QDialog::Rejected);
}
