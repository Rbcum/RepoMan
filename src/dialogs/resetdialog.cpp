#include "resetdialog.h"

#include <QtConcurrent>

#include "cmddialog.h"
#include "ui_resetdialog.h"

ResetDialog::ResetDialog(QWidget *parent, const QString &projectPath, const Commit &commit)
    : QDialog(parent), ui(new Ui::ResetDialog), m_projectPath(projectPath), m_commit(commit)
{
    ui->setupUi(this);
    m_indicator = new QProgressIndicator(this);

    setEnabled(false);
    m_indicator->startHint();
    QtConcurrent::run([projectPath, commit]() {
        QPair<QString, Commit> data;  // branch,commit
        QStringList lines = global::getCmdResult("git branch", projectPath).split('\n');
        for (const QString &line : lines) {
            QString trLine = line.trimmed();
            if (trLine.size() > 0) {
                if (trLine.contains(" -> ")) {
                    trLine = trLine.split(" -> ").first();
                }
                if (trLine.startsWith("*")) {
                    data.first = trLine.mid(2);
                    break;
                }
            }
        }

        if (commit.hash.isEmpty()) {
            lines = global::getCmdResult("git log -n1 --format=%H¿%h¿%s", projectPath)
                        .trimmed()
                        .split("¿");
            Commit c;
            c.hash = lines.at(0);
            c.shortHash = lines.at(1);
            c.subject = lines.at(2);
            data.second = c;
        } else {
            data.second = commit;
        }
        return data;
    }).then(this, [this](QPair<QString, Commit> data) {
        m_commit = data.second;
        ui->branchLabel->setText(data.first);
        ui->commitLabel->setText("<b>" + m_commit.shortHash + "</b> " + m_commit.subject);
        setEnabled(true);
        m_indicator->stopHint();
    });
}

ResetDialog::~ResetDialog()
{
    delete ui;
}

void ResetDialog::accept()
{
    QString modeArg;
    if (ui->buttonGroup->checkedButton() == ui->softRB) {
        modeArg = "--soft";
    } else if (ui->buttonGroup->checkedButton() == ui->mixRB) {
        modeArg = "--mixed";
    } else {
        modeArg = "--hard";
    }
    QString cmd = QString("git reset %1 %2").arg(modeArg, m_commit.hash);
    int code = CmdDialog::execute(parentWidget(), cmd, m_projectPath, true);
    done(code == 0 ? QDialog::Accepted : QDialog::Rejected);
}
