#include "cmddialog.h"

#include "pty/kshell.h"
#include "ui_cmddialog.h"

CmdDialog::CmdDialog(
    QWidget *parent, const QStringList &cmdList, const QString &cwd, bool autoClose)
    : QDialog(parent),
      ui(new Ui::CmdDialog),
      m_cmdList(cmdList),
      m_nextCmdIndex(0),
      m_autoClose(autoClose)
{
    ui->setupUi(this);
    m_pty = new Konsole::Pty(this);
    m_display = new PtyDisplay(ui->textEdit, this);
    connect(m_pty, &Konsole::Pty::receivedData, this, &CmdDialog::onReceiveBlock);
    connect(m_pty, &Konsole::Pty::finished, this, &CmdDialog::onFinished);
    m_pty->setEnv("TERM", "vt100");
    m_pty->setWindowSize(USHRT_MAX, USHRT_MAX);
    m_pty->setWorkingDirectory(cwd);

    processNextCmd();
}

CmdDialog::~CmdDialog()
{
    delete ui;
}

void CmdDialog::processNextCmd()
{
    const QString &cmd = m_cmdList.at(m_nextCmdIndex++);
    ui->textEdit->appendPlainText(cmd + "\n\n");
    const QStringList &args = KShell::splitArgs(cmd);
    m_pty->start(args[0], args, {}, 0, false);
    qDebug() << "CmdDialog:" << cmd;
}

int CmdDialog::execute(QWidget *parent, const QString &cmd, const QString &cwd, bool autoClose)
{
    CmdDialog dialog(parent, {cmd}, cwd, autoClose);
    return dialog.exec();
}

int CmdDialog::execute(
    QWidget *parent, const QStringList &cmdList, const QString &cwd, bool autoClose)
{
    CmdDialog dialog(parent, cmdList, cwd, autoClose);
    return dialog.exec();
}

void CmdDialog::onReceiveBlock(const char *buf, int len)
{
    m_display->onReceiveBlock(buf, len);
}

void CmdDialog::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << "FINISHED:" << exitCode;
    m_display->flushContent();
    m_exitCode = exitCode;

    bool isLastCmd = m_nextCmdIndex > m_cmdList.size() - 1;

    if (exitCode != 0) {
        ui->textEdit->appendPlainText(QString("FAILED(%1)").arg(exitCode));
        if (!isLastCmd) {
            ui->textEdit->appendPlainText("Remaining commands:");
            for (int i = m_nextCmdIndex + 1; i < m_cmdList.size() - 1; ++i) {
                ui->textEdit->appendPlainText(QString("=> %1").arg(m_cmdList.at(i)));
            }
        }
        return;
    }

    if (!isLastCmd) {
        processNextCmd();
    } else if (m_autoClose) {
        done(exitCode);
    } else {
        ui->textEdit->appendPlainText(QString("FINISHED(%1)").arg(exitCode));
    }
}

void CmdDialog::reject()
{
    if (m_pty->isRunning()) {
        m_pty->terminate();
    } else {
        done(m_exitCode);
    }
}
