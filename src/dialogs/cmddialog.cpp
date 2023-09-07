#include "cmddialog.h"

#include "pty/kshell.h"
#include "ui_cmddialog.h"

CmdDialog::CmdDialog(QWidget *parent, const QString &cmd, const QString &cwd, bool autoClose)
    : QDialog(parent),
      ui(new Ui::CmdDialog),
      m_cmd(cmd),
      m_firstBlockReceived(false),
      m_autoClose(autoClose)
{
    ui->setupUi(this);
    ui->textEdit->appendPlainText(cmd + "\n\n");
    m_pty = new Konsole::Pty(this);
    m_display = new PtyDisplay(ui->textEdit, this);
    connect(m_pty, &Konsole::Pty::receivedData, this, &CmdDialog::onReceiveBlock);
    connect(m_pty, &Konsole::Pty::finished, this, &CmdDialog::onFinished);
    m_pty->setEnv("TERM", "vt100");
    m_pty->setWindowSize(USHRT_MAX, USHRT_MAX);
    m_pty->setWorkingDirectory(cwd);
    const QStringList &args = KShell::splitArgs(cmd);
    m_pty->start(args[0], args, {}, 0, false);
}

void CmdDialog::onReceiveBlock(const char *buf, int len)
{
    m_display->onReceiveBlock(buf, len);
}

CmdDialog::~CmdDialog()
{
    delete ui;
}

void CmdDialog::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << "FINISHED:" << exitCode;
    m_display->flushContent();
    ui->textEdit->appendPlainText(QString("Finished(%1)").arg(exitCode));
    this->m_exitCode = exitCode;
    if (m_autoClose && exitCode == 0) {
        done(exitCode);
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
