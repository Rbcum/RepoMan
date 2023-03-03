#include "reftreemodel.h"

#include <QBrush>
#include <QFont>
#include <QIcon>

RefTreeModel::RefTreeModel(QObject *parent)
    : QAbstractItemModel(parent), m_rootItem(new RefTreeItem)
{
    auto branchesItem = m_rootItem->childrens.emplaceBack(new RefTreeItem(m_rootItem));
    auto remotesItem = m_rootItem->childrens.emplaceBack(new RefTreeItem(m_rootItem));
    auto tagsItem = m_rootItem->childrens.emplaceBack(new RefTreeItem(m_rootItem));
    branchesItem->displayName = "Branches";
    branchesItem->row = RefTreeItem::Branch;
    remotesItem->displayName = "Remotes";
    remotesItem->row = RefTreeItem::Remote;
    tagsItem->displayName = "Tags";
    tagsItem->row = RefTreeItem::Tag;
    branchesItem->type = remotesItem->type = tagsItem->type = RefTreeItem::Group;
}

QModelIndex RefTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    RefTreeItem *parentItem;
    if (!parent.isValid()) {
        parentItem = m_rootItem.data();
    } else {
        parentItem = static_cast<RefTreeItem *>(parent.internalPointer());
    }
    return createIndex(row, column, parentItem->childrens[row].data());
}

QModelIndex RefTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }
    RefTreeItem *item = static_cast<RefTreeItem *>(index.internalPointer());
    if (item->type == RefTreeItem::Group) {
        return QModelIndex();
    } else {
        return createIndex(item->parent->row, 0, item->parent.data());
    }
}

int RefTreeModel::rowCount(const QModelIndex &parent) const
{
    RefTreeItem *parentItem;
    if (!parent.isValid()) {
        parentItem = m_rootItem.data();
    } else {
        parentItem = static_cast<RefTreeItem *>(parent.internalPointer());
    }
    return parentItem->childrens.size();
}

int RefTreeModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

bool RefTreeModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return true;
    }

    RefTreeItem *parentItem = static_cast<RefTreeItem *>(parent.internalPointer());
    return parentItem->type == RefTreeItem::Group || rowCount(parent) > 0;
}

QVariant RefTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();

    RefTreeItem *item = static_cast<RefTreeItem *>(index.internalPointer());
    switch (role) {
        case Qt::DisplayRole:
            return item->displayName;
        case Qt::ToolTipRole:
            if (item->type != RefTreeItem::Group) {
                return item->data;
            }
            break;
        case Qt::SizeHintRole:
            if (item->type == RefTreeItem::Group) {
                return QSize(0, 36);
            } else {
                return QSize(0, 28);
            }
            break;
        case Qt::DecorationRole:
            if (item->type == RefTreeItem::Group) {
                switch (item->row) {
                    case RefTreeItem::Branch:
                        return QIcon("://resources/icon_branch.svg");
                    case RefTreeItem::Remote:
                        return QIcon("://resources/icon_cloud.svg");
                    case RefTreeItem::Tag:
                        return QIcon("://resources/icon_tag.svg");
                    default:
                        Q_UNREACHABLE();
                }
            }
            break;
    }
    return QVariant();
}

Qt::ItemFlags RefTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) return Qt::NoItemFlags;

    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    RefTreeItem *item = static_cast<RefTreeItem *>(index.internalPointer());
    if (item->type == RefTreeItem::Group || item->type == RefTreeItem::General) {
        flags &= ~Qt::ItemIsEnabled;
    }
    return flags;
}

void RefTreeModel::addRefData(const QStringList &refsData, RefTreeItem::Type type)
{
    QSharedPointer<RefTreeItem> rootItem = m_rootItem->childrens[type];
    QModelIndex rootIndex = createIndex(rootItem->row, 0, rootItem.data());

    for (int i = 0; i < refsData.size(); ++i) {
        QSharedPointer<RefTreeItem> parent = rootItem;
        const QStringList &chunks = refsData[i].split("/");
        for (int j = 0; j < chunks.size(); ++j) {
            const QString &chunk = chunks[j];
            if (j == chunks.size() - 1) {
                // Last chunk
                auto item = parent->childrens.emplaceBack(new RefTreeItem(parent));
                item->data = refsData[i];
                item->displayName = chunk;
                item->type = type;
                item->row = parent->childrens.size() - 1;
                break;
            }
            bool parentExists = false;
            for (auto &child : parent->childrens) {
                if (child->displayName == chunk) {
                    parent = child;
                    parentExists = true;
                    break;
                }
            }
            if (parentExists) {
                continue;
            }
            auto item = parent->childrens.emplaceBack(new RefTreeItem(parent));
            item->data = chunk;
            item->displayName = chunk;
            item->type = RefTreeItem::General;
            item->row = parent->childrens.size() - 1;
            parent = item;
        }
    }

    beginInsertRows(rootIndex, 0, rootItem->childrens.size() - 1);
    endInsertRows();
}

void RefTreeModel::reset()
{
    for (auto &groupItem : m_rootItem->childrens) {
        QModelIndex groupIndex = createIndex(groupItem->row, 0, groupItem.data());
        if (groupItem->childrens.isEmpty()) continue;
        beginRemoveRows(groupIndex, 0, groupItem->childrens.size() - 1);
        groupItem->childrens.clear();
        endRemoveRows();
    }
}

RefTreeItem::RefTreeItem(QSharedPointer<RefTreeItem> parent)
{
    this->parent = parent;
}
