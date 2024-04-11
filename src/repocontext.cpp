#include "repocontext.h"

#include <QDir>
#include <QDomDocument>

RepoContext::RepoContext(const QString &repoPath) : m_repoPath(repoPath)
{
    loadManifest();
}

QSettings RepoContext::settings()
{
    const QString &confFile = QDir::cleanPath(m_repoPath + "/repoman.conf");
    return QSettings(confFile, QSettings::NativeFormat);
}

bool RepoContext::isValid()
{
    return !m_manifest.filePath.isEmpty();
}

void RepoContext::loadManifest()
{
    if (m_repoPath.isEmpty()) return;
    QString manPath = QDir::cleanPath(m_repoPath + "/.repo/manifest.xml");
    parseManifest(manPath);
}

void RepoContext::parseManifest(const QString &manPath)
{
    QFile manFile(manPath);
    QDomDocument doc("");
    if (!manFile.open(QIODevice::ReadOnly)) return;
    if (!doc.setContent(&manFile)) {
        manFile.close();
        return;
    }
    manFile.close();

    m_manifest = {};
    m_manifest.filePath = manPath;

    QDomNode n = doc.documentElement().firstChild();
    while (!n.isNull()) {
        QDomElement e = n.toElement();
        if (e.nodeName() == "include") {
            QString manPath2 =
                QDir::cleanPath(m_repoPath + "/.repo/manifests/" + e.attribute("name"));
            parseManifest(manPath2);
        } else if (e.nodeName() == "default") {
            m_manifest.remote = e.attribute("remote");
            m_manifest.revision = e.attribute("revision");
            m_manifest.syncJ = e.attribute("sync-j").toInt();
        } else if (e.nodeName() == "project") {
            QString name = e.attribute("name");
            QString path = e.attribute("path");
            if (path.isEmpty()) path = name;
            QString absPath = QDir::cleanPath(m_repoPath + "/" + path);
            m_manifest.projectList.emplace_back(name, path, absPath);
            m_manifest.projectMap.insert(path, Project(name, path, absPath));
        }
        n = n.nextSibling();
    }
    std::sort(m_manifest.projectList.begin(), m_manifest.projectList.end(),
        [](const Project &p1, const Project &p2) {
            return p1.path < p2.path;
        });
}
