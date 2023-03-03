#ifndef REFTREEDELEGATE_H
#define REFTREEDELEGATE_H

#include <QStyledItemDelegate>
#include <QTreeView>

class RefTreeDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    RefTreeDelegate(QObject *parent, QTreeView *treeView);
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
        const QModelIndex &index) const override;

    void setCurrentBranch(const QString &branch);
    void setBranchesLoading(bool loading);
    void setRemotesLoading(bool loading);
    void setTagsLoading(bool loading);

private:
    QTreeView *m_treeView;

    QString m_currentBranch;
    int m_branchesLoading;
    int m_remotesLoading;
    int m_tagsLoading;
};

#endif  // REFTREEDELEGATE_H
