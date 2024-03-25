#ifndef SWITCHMANIFESTDIALOG_H
#define SWITCHMANIFESTDIALOG_H

#include <QAbstractItemModel>
#include <QDialog>

#include "widgets/QProgressIndicator.h"

namespace Ui {
    class SwitchManifestDialog;
}

class FileEntry
{
public:
    FileEntry(QSharedPointer<FileEntry> parent = nullptr) : parent(parent)
    {
    }
    QWeakPointer<FileEntry> parent;
    QList<QSharedPointer<FileEntry>> children;
    bool isDir;
    int row = 0;
    QString name;

    QSharedPointer<FileEntry> getChild(const QString &name)
    {
        for (auto &c : children) {
            if (name == c->name) {
                return c;
            }
        }
        return nullptr;
    }
};

class FileTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit FileTreeModel(QObject *parent = nullptr);

    QModelIndex index(
        int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex index(const QString &path) const;
    QString path(const QModelIndex &index) const;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void setRootEntry(const QSharedPointer<FileEntry> &entry);
    void setCurrentPath(const QString &path);

private:
    QSharedPointer<FileEntry> m_rootEntry;
    QString m_currentPath;

    void sortChildren(const QString &path);
};

class SwitchManifestDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SwitchManifestDialog(QWidget *parent = nullptr);
    ~SwitchManifestDialog();

    enum DialogCode
    {
        OpenSync = QDialog::Accepted + 1
    };

private slots:
    void onFileDoubleClicked(const QModelIndex &index);
    void onPathClicked(const QString &path);

public slots:
    void accept() override;

private:
    struct InitData
    {
        QStringList remoteBranches;
        QString currentBranch;
    };

    Ui::SwitchManifestDialog *ui;
    QProgressIndicator *m_indicator;

    FileTreeModel *m_model;
    QString m_currentBranch;

    QSharedPointer<FileEntry> buildManifestTree(const QString &branch);
    void updateListUI(const QString &branch);
};

#endif  // SWITCHMANIFESTDIALOG_H
