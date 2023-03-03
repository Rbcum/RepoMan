#include "cmddialog.h"

#include "ui_cmddialog.h"

CmdDialog::CmdDialog(QWidget *parent, const QString &cmd, const QString &cwd, bool autoClose)
    : QDialog(parent),
      ui(new Ui::CmdDialog),
      m_cmd(cmd),
      m_firstBlockReceived(false),
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
    m_pty->start("/bin/bash", {""}, {}, 0, false);
}

void CmdDialog::onReceiveBlock(const char *buf, int len)
{
    if (!m_firstBlockReceived) {
        QString cmd2 = m_cmd + "    ;RET=$?;echo;exit $RET\r";
        m_pty->sendData(cmd2.toLatin1(), cmd2.length());
        m_firstBlockReceived = true;
    }

    m_display->onReceiveBlock(buf, len);
}

CmdDialog::~CmdDialog()
{
    delete ui;
}

void CmdDialog::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << "FINISHED:" << exitCode;
    this->m_exitCode = exitCode;
    if (m_autoClose && exitCode == 0) {
        done(exitCode);
    }
}

void CmdDialog::reject()
{
    if (m_pty->isRunning()) {
        // Ctrl+C
        m_pty->sendData("\x03", 1);
    } else {
        done(m_exitCode);
    }
}
