#ifndef REPOCONTEXT_H
#define REPOCONTEXT_H

#include <QSettings>
#include <QString>

struct Project
{
    Project()
    {
    }

    Project(const QString &name, const QString &path, const QString &absPath)
        : name(name), path(path), absPath(absPath)
    {
    }
    QString name;
    QString path;
    QString absPath;
};

struct Manifest
{
    QString filePath;
    QString remote;
    QString revision;
    int syncJ;
    QList<Project> projectList;
    QMap<QString, Project> projectMap;  // key:path
};

class RepoContext
{
public:
    RepoContext(const QString &repoPath);
    QSettings settings();
    bool isValid();
    void loadManifest();
    QString repoPath() const
    {
        return m_repoPath;
    }
    const Manifest &manifest() const
    {
        return m_manifest;
    }

private:
    QString m_repoPath;
    Manifest m_manifest;

    void parseManifest(const QString &manPath);
};

#endif  // REPOCONTEXT_H
