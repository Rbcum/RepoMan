#ifndef CMDDIALOG_H
#define CMDDIALOG_H

#include <QDialog>

#include "pty/Pty.h"
#include "pty/ptydisplay.h"

namespace Ui {
    class CmdDialog;
}

class CmdDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CmdDialog(
        QWidget *parent, const QStringList &cmdList, const QString &cwd, bool autoClose = false);
    ~CmdDialog();
    static int execute(
        QWidget *parent, const QString &cmd, const QString &cwd, bool autoClose = false);
    static int execute(
        QWidget *parent, const QStringList &cmdList, const QString &cwd, bool autoClose = false);

private:
    Ui::CmdDialog *ui;
    Konsole::Pty *m_pty;
    PtyDisplay *m_display;
    QStringList m_cmdList;
    int m_nextCmdIndex;
    bool m_autoClose;
    int m_exitCode;

    void processNextCmd();
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus);

    // QDialog interface
public slots:
    void reject() override;
    void onReceiveBlock(const char *buf, int len);
};

#endif  // CMDDIALOG_H
