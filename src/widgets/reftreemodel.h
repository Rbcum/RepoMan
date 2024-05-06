#ifndef REVSTREEMODEL_H
#define REVSTREEMODEL_H

#include <QAbstractItemModel>
#include <QSharedPointer>

class RefTreeItem
{
public:
    enum Type
    {
        // Order and value matters here
        General = -2,
        Group = -1,
        Branch = 0,
        Remote,
        Tag,
    };
    explicit RefTreeItem(QSharedPointer<RefTreeItem> parent = nullptr);
    QString data;
    QString displayName;
    Type type;
    int row;
    QWeakPointer<RefTreeItem> parent;
    QList<QSharedPointer<RefTreeItem>> childrens;
};

class RefTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit RefTreeModel(QObject *parent = nullptr);

    // Basic functionality:
    QModelIndex index(
        int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void addRefData(const QStringList &refsData, RefTreeItem::Type type);
    void reset();

private:
    QSharedPointer<RefTreeItem> m_rootItem;
};

#endif  // REVSTREEMODEL_H
