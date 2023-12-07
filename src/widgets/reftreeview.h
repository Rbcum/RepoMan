#ifndef REFTREEVIEW_H
#define REFTREEVIEW_H

#include <QFuture>
#include <QTreeView>

#include "pages/historytablemodel.h"
#include "reftreedelegate.h"
#include "reftreemodel.h"

class RefTreeView : public QTreeView
{
    Q_OBJECT
public:
    RefTreeView(QWidget *parent);
    ~RefTreeView();
    void setProjectPath(const QString &path);
    void refresh();

private slots:
    void onExpanded(const QModelIndex &index);
    void onDoubleClicked(const QModelIndex &index);
    void onMenuRequested(const QPoint &pos);

signals:
    void requestRefreshEvent(const HistorySelectionArg &arg = HistorySelectionArg());

private:
    RefTreeModel *m_model;
    RefTreeDelegate *m_delegate;

    QString m_projectPath;

    QFuture<QStringList> m_branchesFetcher;
    QFuture<QStringList> m_remotesFetcher;
    QFuture<QStringList> m_tagsFetcher;
    bool m_remotesLoaded;
    bool m_tagsLoaded;

    void getBranchesAsync(const QString &projectPath);
    void getRemotesAsync(const QString &projectPath);
    void getTagsAsync(const QString &projectPath);
    void deleteRef(RefTreeItem *item);
};

#endif  // REFTREEVIEW_H
