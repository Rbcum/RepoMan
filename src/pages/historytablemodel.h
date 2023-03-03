#ifndef HISTORYTABLEMODEL_H
#define HISTORYTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStyledItemDelegate>

#include "global.h"

class HistorySelectionArg
{
public:
    enum Type
    {
        Null,
        First,
        Hash,
        Tag,
        Head,
        Remote,
    };

    HistorySelectionArg() : type(Null)
    {
    }
    HistorySelectionArg(Type type, const QString &data = "") : type(type), data(data)
    {
    }

    Type type;
    QString data;

    bool isSearchable() const
    {
        return type == Hash || type == Tag || type == Head || type == Remote;
    }

    bool match(const Commit &c) const
    {
        switch (type) {
            case Hash:
                return c.hash == data;
            case Tag:
                return c.tags.contains(data);
            case Head:
                return c.heads.contains(data);
            case Remote:
                return c.remotes.contains(data);
            default:
                return false;
        }
    }

    //        friend bool operator==(const SelectionArg &a1, const SelectionArg &a2)
    //        {
    //            return a1.hash == a2.hash && a1.tag == a2.tag && a1.head == a2.head &&
    //                   a1.remote == a2.remote;
    //        }

    friend QDebug operator<<(QDebug dbg, const HistorySelectionArg &a)
    {
        QDebugStateSaver saver(dbg);
        dbg.nospace() << "type: " << a.type << ", data: " << a.data;
        return dbg;
    }
};

class HistoryTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum CustomRoles
    {
        CommitRole = Qt::UserRole,
    };

    explicit HistoryTableModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    void fetchMore(const QModelIndex &parent) override;
    void fetchMore(const QModelIndex &parent, const HistorySelectionArg &arg);
    bool canFetchMore(const QModelIndex &parent) const override;

    void reset();
    void addCommits(const QList<Commit> &commits);
    QModelIndex searchCommit(const HistorySelectionArg &arg);

private:
    QList<Commit> m_commitList;
    bool m_canFetchMoreFlag;

signals:
    void fetchMoreEvt(int skip, const HistorySelectionArg &arg = HistorySelectionArg());
};

#endif  // HISTORYTABLEMODEL_H
