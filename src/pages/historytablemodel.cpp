#include "historytablemodel.h"

#include <QPainter>
#include <QTableView>

#include "widgets/qhistorytableview.h"

HistoryTableModel::HistoryTableModel(QObject *parent)
    : QAbstractTableModel{parent}, m_canFetchMoreFlag(false)
{
}

int HistoryTableModel::rowCount(const QModelIndex &parent) const
{
    return m_commitList.size();
}

int HistoryTableModel::columnCount(const QModelIndex &parent) const
{
    return QHistoryTableView::HEADERS.size();
}

QVariant HistoryTableModel::data(const QModelIndex &index, int role) const
{
    Commit commit = m_commitList.at(index.row());
    switch (role) {
        case Qt::DisplayRole:
            {
                switch (index.column()) {
                    case 0:
                        return "";
                    case 1:
                        return commit.subject;
                    case 2:
                        return commit.commitDate.chopped(9);
                    case 3:
                        return commit.author + " <" + commit.authorEmail + ">";
                    case 4:
                        return commit.shortHash;
                    default:
                        return "";
                }
            }
        case CommitRole:
            {
                return QVariant::fromValue(commit);
            }
        default:
            {
                return QVariant();
            }
    }
}

QVariant HistoryTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        return QHistoryTableView::HEADERS[section];
    }
    return QVariant();
}

void HistoryTableModel::fetchMore(const QModelIndex &parent)
{
    qDebug() << "fetchMore";
    emit fetchMoreEvt(m_commitList.size());
}

void HistoryTableModel::fetchMore(const QModelIndex &parent, const HistorySelectionArg &arg)
{
    qDebug() << "fetchMore arg:" << arg;
    emit fetchMoreEvt(m_commitList.size(), arg);
}

bool HistoryTableModel::canFetchMore(const QModelIndex &parent) const
{
    qDebug() << "canFetchMore:" << parent << m_canFetchMoreFlag;
    return m_canFetchMoreFlag;
}

void HistoryTableModel::reset()
{
    beginResetModel();
    this->m_commitList.clear();
    this->m_canFetchMoreFlag = false;
    endResetModel();
}

void HistoryTableModel::addCommits(const QList<Commit> &commits)
{
    beginInsertRows(QModelIndex(), m_commitList.size(), m_commitList.size() + commits.size() - 1);
    this->m_commitList.append(commits);
    this->m_canFetchMoreFlag = commits.size() >= global::commitPageSize;
    endInsertRows();
}

QModelIndex HistoryTableModel::searchCommit(const HistorySelectionArg &arg)
{
    for (int i = 0; i < m_commitList.size(); ++i) {
        const Commit &c = m_commitList[i];
        if (arg.match(c)) {
            return createIndex(i, 0);
        }
    }
    return QModelIndex();
}
