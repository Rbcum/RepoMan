#include "cleandialog.h"

#include <QScrollBar>
#include <QtConcurrent>

#include "cmddialog.h"
#include "global.h"
#include "ui_cleandialog.h"
#include "warningfilelistdialog.h"

CleanDialog::CleanDialog(QWidget *parent, const QString &projectPath)
    : QDialog(parent), ui(new Ui::CleanDialog), m_projectPath(projectPath)
{
    ui->setupUi(this);
    m_indicator = new QProgressIndicator(this);
}

CleanDialog::~CleanDialog()
{
    delete ui;
}

void CleanDialog::accept()
{
    QString projectPath = m_projectPath;
    QString cmd = QString("git clean -f%1%2")
                      .arg(ui->dirCheckBox->isChecked() ? "d" : "",
                          ui->ignorecheckBox->isChecked() ? "x" : "");
    setEnabled(false);
    m_indicator->startHint();

    QtConcurrent::run([projectPath, cmd]() {
        QStringList fileList;
        QStringList lines = global::getCmdResult(cmd + "n", projectPath).split('\n');
        for (const QString &line : lines) {
            QString trLine = line.trimmed();
            if (trLine.size() > 0) {
                fileList << trLine.mid(QLatin1String("Would remove ").size());
            }
        }
        return fileList;
    }).then(this, [this, cmd](QStringList fileList) {
        setEnabled(true);
        m_indicator->stopHint();
        if (WarningFileListDialog::confirm(this, "Confirm Remove",
                "The following files will be removed permernantly!!!", fileList)) {
            int code = CmdDialog::execute(parentWidget(), cmd, m_projectPath, true);
            done(code == 0 ? QDialog::Accepted : QDialog::Rejected);
        }
    });
}
