#ifndef GLOBAL_H
#define GLOBAL_H

#include <QList>
#include <QMap>
#include <QProcess>
#include <QString>

struct Commit
{
    Commit()
    {
    }

    QString hash;
    QString shortHash;
    QString subject;
    QString author;
    QString authorDate;
    QString authorEmail;
    QString committer;
    QString commitDate;
    QString committerEmail;

    QStringList parents;
    bool isHEAD;
    QStringList heads;
    QStringList tags;
    QStringList remotes;
};

struct GitFile
{
    GitFile()
    {
    }

    GitFile(const QString &path, const QString &mode) : path(path), mode(mode)
    {
    }
    QString path;
    QString mode;

    friend bool operator==(const GitFile &f1, const GitFile &f2) noexcept
    {
        return f1.mode == f2.mode && f1.path == f2.path;
    }
};

namespace global {
    extern int commitPageSize;

    extern int getCmdCode(const QString &cmd, const QString &dir);
    extern QString getCmdResult(const QString &cmd, const QString &dir);
}  // namespace global

#endif  // GLOBAL_H
