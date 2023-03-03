#include "cleandialog.h"

#include <QScrollBar>
#include <QtConcurrent>

#include "cmddialog.h"
#include "global.h"
#include "ui_cleanconfirmdialog.h"
#include "ui_cleandialog.h"

CleanDialog::CleanDialog(QWidget *parent, const QString &projectPath)
    : QDialog(parent), ui(new Ui::CleanDialog), m_projectPath(projectPath)
{
    ui->setupUi(this);
}

CleanDialog::~CleanDialog()
{
    delete ui;
}

void CleanDialog::accept()
{
    QString cmd = QString("git clean -f%1%2")
                      .arg(ui->dirCheckBox->isChecked() ? "d" : "",
                          ui->ignorecheckBox->isChecked() ? "x" : "");
    CleanConfirmDialog confirmDialog(this, m_projectPath, cmd);
    if (confirmDialog.exec()) {
        CmdDialog dialog(parentWidget(), cmd, m_projectPath);
        done(dialog.exec() == 0 ? QDialog::Accepted : QDialog::Rejected);
    }
}

CleanConfirmDialog::CleanConfirmDialog(
    QWidget *parent, const QString &projectPath, const QString &cmd)
    : QDialog(parent), ui(new Ui::CleanConfirmDialog), m_projectPath(projectPath)
{
    ui->setupUi(this);
    m_indicator = new QProgressIndicator(this);

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
    }).then(this, [this](QStringList fileList) {
        for (const QString &f : fileList) {
            ui->filesTextEdit->appendPlainText(f);
        }
        ui->filesTextEdit->verticalScrollBar()->setValue(0);
        setEnabled(true);
        m_indicator->stopHint();
    });
}

CleanConfirmDialog::~CleanConfirmDialog()
{
    delete ui;
}
