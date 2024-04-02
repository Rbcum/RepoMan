#include "global.h"

#include <QDir>

namespace global {
    Manifest manifest;
    int commitPageSize = 100;
    QString cwd;

    QSettings getRepoSettings()
    {
        const QString &confFile = QDir::cleanPath(cwd + "/repoman.conf");
        return QSettings(confFile, QSettings::NativeFormat);
    }

    QSettings getGlobalSettings()
    {
        return QSettings();
    }

    int getCmdCode(const QString &cmd, const QString &dir)
    {
        //        qDebug() << "Cmd:" << cmd;
        QProcess process;
        process.setWorkingDirectory(dir);
        process.startCommand(cmd, QIODeviceBase::ReadOnly);
        if (!process.waitForFinished(-1) || process.error() == QProcess::FailedToStart) return -2;
        return process.exitStatus() == QProcess::NormalExit ? process.exitCode() : -1;
    }

    QString getCmdResult(const QString &cmd, const QString &dir)
    {
        qDebug() << "Cmd:" << cmd;
        QProcess process;
        process.setWorkingDirectory(dir);
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.startCommand(cmd, QIODeviceBase::ReadOnly);
        if (!process.waitForFinished(-1) || process.error() == QProcess::FailedToStart) return "";
        return process.readAll();
    }

}  // namespace global
