#include "switchmanifestdialog.h"

#include <QFileSystemModel>
#include <QScrollBar>
#include <QSettings>
#include <QTimer>
#include <QtConcurrent>

#include "cmddialog.h"
#include "global.h"
#include "ui_switchmanifestdialog.h"

using namespace global;

SwitchManifestDialog::SwitchManifestDialog(QWidget *parent, const RepoContext &context)
    : QDialog(parent), ui(new Ui::SwitchManifestDialog), m_context(context)
{
    ui->setupUi(this);
    m_indicator = new QProgressIndicator(this);
    m_model = new FileTreeModel(this);

    ui->listView->setModel(m_model);
    ui->pathFinder->setTitle("Manifests:");

    connect(
        ui->listView, &QListView::doubleClicked, this, &SwitchManifestDialog::onFileDoubleClicked);
    connect(
        ui->pathFinder, &PathFinderView::pathClicked, this, &SwitchManifestDialog::onPathClicked);
    connect(
        ui->branchCombo, &QComboBox::currentTextChanged, this, &SwitchManifestDialog::updateListUI);

    setEnabled(false);
    m_indicator->startHint();
    QtConcurrent::run([=]() {
        InitData data;
        QString repoPath = m_context.repoPath();
        global::getCmdCode("git fetch", repoPath + "/.repo/manifests");
        data.currentBranch = global::getCmdResult(
            "git rev-parse --abbrev-ref --symbolic-full-name @{u}", repoPath + "/.repo/manifests")
                                 .trimmed()
                                 .mid(QString("origin/").size());
        QStringList lines =
            global::getCmdResult("git branch -r", repoPath + "/.repo/manifests").split('\n');
        for (const QString &line : lines) {
            QString trLine = line.trimmed();
            if (trLine.size() > 0) {
                if (trLine.startsWith("origin/")) {
                    data.remoteBranches << trLine.mid(QString("origin/").size());
                }
            }
        }
        return data;
    }).then(this, [=](InitData data) {
        setEnabled(true);
        m_indicator->stopHint();
        m_currentBranch = data.currentBranch;
        {
            const QSignalBlocker blocker(ui->branchCombo);
            ui->branchCombo->addItems(data.remoteBranches);
        }
        ui->branchCombo->setCurrentText(data.currentBranch);
    });
}

SwitchManifestDialog::~SwitchManifestDialog()
{
    delete ui;
}

bool SwitchManifestDialog::syncAfterSwitch()
{
    return ui->syncSwitch->isChecked();
}

void SwitchManifestDialog::onFileDoubleClicked(const QModelIndex &index)
{
    auto entry = static_cast<FileEntry *>(index.internalPointer());
    QString path = m_model->data(index, QFileSystemModel::FilePathRole).toString();

    if (entry->isDir) {
        m_model->setCurrentPath(path);
        ui->pathFinder->setPath(path);
        ui->listView->setRootIndex(m_model->index(path));
    } else {
        accept();
    }
}

void SwitchManifestDialog::onPathClicked(const QString &path)
{
    m_model->setCurrentPath(path);
    ui->listView->setRootIndex(m_model->index(path));
}

void SwitchManifestDialog::accept()
{
    if (!ui->listView->selectionModel()->hasSelection()) return;
    QString path = ui->listView->selectionModel()
                       ->selectedRows()
                       .first()
                       .data(QFileSystemModel::FilePathRole)
                       .toString();
    if (!path.endsWith(".xml")) return;

    QString cmd = QString("repo init -m %1 -b %2").arg(path, ui->branchCombo->currentText());
    int code = CmdDialog::execute(this, cmd, m_context.repoPath());
    done(code == 0 ? QDialog::Accepted : QDialog::Rejected);
}

QSharedPointer<FileEntry> SwitchManifestDialog::buildManifestTree(const QString &branch)
{
    const QString &manDir = QDir::cleanPath(m_context.repoPath() + "/.repo/manifests");
    const QString &cmdResult =
        global::getCmdResult("git ls-tree -r --name-only origin/" + branch, manDir);
    QSharedPointer<FileEntry> rootEntry(new FileEntry());
    if (cmdResult.isEmpty()) {
        return rootEntry;
    }
    QStringList lines = cmdResult.trimmed().split('\n');
    for (int i = 0; i < lines.size(); ++i) {
        auto parentEntry = rootEntry;
        QStringList segments = lines[i].split("/");
        for (int j = 0; j < segments.size(); ++j) {
            auto segEntry = parentEntry->getChild(segments[j]);
            if (!segEntry) {
                segEntry = parentEntry->children.emplaceBack(new FileEntry(parentEntry));
                segEntry->name = segments[j];
                segEntry->row = parentEntry->children.size() - 1;
                segEntry->isDir = j != segments.size() - 1;
            }
            parentEntry = segEntry;
        }
    }
    return rootEntry;
}

void SwitchManifestDialog::updateListUI(const QString &branch)
{
    qDebug() << "updateListUI";
    setEnabled(false);
    m_indicator->startHint();
    QtConcurrent::run([=]() {
        return buildManifestTree(branch);
    }).then(this, [=](QSharedPointer<FileEntry> rootEntry) {
        setEnabled(true);
        m_indicator->stopHint();
        m_model->setRootEntry(rootEntry);
        if (branch == m_currentBranch) {
            QString manPath = QFileInfo(m_context.manifest().filePath).symLinkTarget();
            QString manDir = QFileInfo(manPath).absoluteDir().path();
            QString innerManDir =
                manDir.sliced(QDir::cleanPath(m_context.repoPath() + "/.repo/manifests/").size());
            QString innerManPath =
                manPath.sliced(QDir::cleanPath(m_context.repoPath() + "/.repo/manifests/").size());
            // qDebug() << cwd << manPath << manDir << innerManDir;

            m_model->setCurrentPath(innerManDir);
            ui->pathFinder->setPath(innerManDir);
            ui->listView->setRootIndex(m_model->index(innerManDir));
            QTimer::singleShot(0, this, [=]() {
                ui->listView->scrollTo(
                    m_model->index(innerManPath), QAbstractItemView::PositionAtCenter);
                ui->listView->setCurrentIndex(m_model->index(innerManPath));
            });
        } else {
            m_model->setCurrentPath("");
            ui->pathFinder->setPath("");
            ui->listView->setRootIndex(QModelIndex());
            ui->listView->scrollToTop();
        }
    });
}

FileTreeModel::FileTreeModel(QObject *parent)
    : QAbstractItemModel(parent), m_rootEntry(new FileEntry())
{
}

QModelIndex FileTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    FileEntry *parentEntry;
    if (!parent.isValid()) {
        parentEntry = m_rootEntry.data();
    } else {
        parentEntry = static_cast<FileEntry *>(parent.internalPointer());
    }
    return createIndex(row, column, parentEntry->children[row].data());
}

QModelIndex FileTreeModel::index(const QString &path) const
{
    QStringList segments = path.split("/");
    auto parentEntry = m_rootEntry;
    for (int i = 0; i < segments.size(); ++i) {
        const QString &seg = segments[i];
        if (seg.isEmpty()) {
            continue;
        }
        auto entry = parentEntry->getChild(seg);
        if (!entry) {
            break;
        } else if (i == segments.size() - 1) {
            return createIndex(entry->row, 0, entry.data());
        }
        parentEntry = entry;
    }
    return QModelIndex();
}

QString FileTreeModel::path(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QString();
    }
    QStringList path;
    QModelIndex idx = index;
    while (idx.isValid()) {
        auto entry = static_cast<FileEntry *>(idx.internalPointer());
        path.prepend(entry->name);
        idx = idx.parent();
    }
    QString fullPath = QDir::fromNativeSeparators(path.join(QDir::separator()));
    return fullPath;
}

QModelIndex FileTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }
    FileEntry *entry = static_cast<FileEntry *>(index.internalPointer());
    if (entry->parent == nullptr || entry->parent == m_rootEntry) {
        return QModelIndex();
    } else {
        const QSharedPointer<FileEntry> parentEntry = entry->parent.toStrongRef();
        Q_ASSERT(parentEntry);
        return createIndex(parentEntry->row, 0, parentEntry.data());
    }
}

int FileTreeModel::rowCount(const QModelIndex &parent) const
{
    FileEntry *parentEntry;
    if (!parent.isValid()) {
        parentEntry = m_rootEntry.data();
    } else {
        parentEntry = static_cast<FileEntry *>(parent.internalPointer());
    }
    return parentEntry->children.size();
}

int FileTreeModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

bool FileTreeModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return true;
    }

    FileEntry *parentEntry = static_cast<FileEntry *>(parent.internalPointer());
    return parentEntry->children.size() > 0;
}

QVariant FileTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    FileEntry *entry = static_cast<FileEntry *>(index.internalPointer());
    switch (role) {
        case Qt::DisplayRole:
            return entry->name;
        case Qt::SizeHintRole:
            return QSize(0, 20);
        case Qt::DecorationRole:
            if (entry->name.endsWith(".xml")) {
                return QIcon::fromTheme("text-xml");
            } else {
                return QIcon::fromTheme("folder");
            }
        case QFileSystemModel::FilePathRole:
            return path(index);
    }
    return QVariant();
}

class FileTreeModelSorter
{
public:
    inline FileTreeModelSorter()
    {
        naturalCompare.setNumericMode(true);
        naturalCompare.setCaseSensitivity(Qt::CaseInsensitive);
    }

    bool operator()(const QSharedPointer<FileEntry> &l, const QSharedPointer<FileEntry> &r) const
    {
        // place directories before files
        bool left = l->isDir;
        bool right = r->isDir;
        if (left ^ right) return left;

        return naturalCompare.compare(l->name, r->name) < 0;
    }

private:
    QCollator naturalCompare;
};

void FileTreeModel::sortChildren(const QString &path)
{
    QStringList segments = path.split("/");
    auto parentEntry = m_rootEntry;
    for (int i = 0; i < segments.size(); ++i) {
        const QString &seg = segments[i];
        if (seg.isEmpty()) {
            continue;
        }
        auto entry = parentEntry->getChild(seg);
        Q_ASSERT(entry);
        parentEntry = entry;
    }

    auto &children = parentEntry->children;
    FileTreeModelSorter sorter;
    std::sort(children.begin(), children.end(), sorter);
    for (int i = 0; i < children.size(); ++i) {
        children[i]->row = i;
    }
}

void FileTreeModel::sort(int column, Qt::SortOrder order)
{
    emit layoutAboutToBeChanged();
    QModelIndexList oldList = persistentIndexList();

    sortChildren(m_currentPath);

    QModelIndexList newList;
    const int size = oldList.size();
    newList.reserve(size);
    for (int i = 0; i < size; ++i) {
        FileEntry *entry = static_cast<FileEntry *>(oldList[i].internalPointer());
        newList.append(createIndex(entry->row, 0, entry));
    }
    changePersistentIndexList(oldList, newList);
    emit layoutChanged();
}

Qt::ItemFlags FileTreeModel::flags(const QModelIndex &index) const
{
    return QAbstractItemModel::flags(index);
}

void FileTreeModel::setRootEntry(const QSharedPointer<FileEntry> &entry)
{
    beginResetModel();
    m_rootEntry = entry;
    endResetModel();
}

void FileTreeModel::setCurrentPath(const QString &path)
{
    m_currentPath = path;

    QTimer::singleShot(0, this, [&]() {
        sort(0, Qt::AscendingOrder);
    });
}
