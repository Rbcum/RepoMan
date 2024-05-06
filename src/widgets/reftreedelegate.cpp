#include "reftreedelegate.h"

#include <QPainter>
#include <QTreeView>

#include "reftreemodel.h"
#include "themes/icon.h"
#include "themes/theme.h"

using namespace utils;

RefTreeDelegate::RefTreeDelegate(QObject *parent, QTreeView *treeView)
    : QStyledItemDelegate{parent},
      m_treeView(treeView),
      m_branchesLoading(0),
      m_remotesLoading(0),
      m_tagsLoading(0)
{
}

void RefTreeDelegate::paint(
    QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    RefTreeItem *item = static_cast<RefTreeItem *>(index.internalPointer());
    QStyleOptionViewItem opt = option;
    opt.state &= ~QStyle::State_HasFocus;
    auto textColor = creatorTheme()->color(Theme::PaletteText);
    opt.palette.setColor(QPalette::All, QPalette::Text, textColor);
    opt.palette.setColor(QPalette::All, QPalette::HighlightedText, textColor);
    if (item->type != RefTreeItem::Group) {
        opt.font.setPointSize(10);
        if (item->type == RefTreeItem::Branch) {
            if (item->data == m_currentBranch) {
                opt.font.setBold(true);
            }
        }
    }
    QStyledItemDelegate::paint(painter, opt, index);

    if (item->type == RefTreeItem::Group) {
        QIcon refreshIcon = Icon({{":/icons/refresh.png", Theme::IconsBaseColor}}).icon();
        QRect iconRect = opt.rect.adjusted(0, 8, -5, -8);
        switch (item->row) {
            case RefTreeItem::Branch:
                if (m_branchesLoading) {
                    refreshIcon.paint(painter, iconRect, Qt::AlignVCenter | Qt::AlignRight);
                }
                break;
            case RefTreeItem::Remote:
                if (m_remotesLoading) {
                    refreshIcon.paint(painter, iconRect, Qt::AlignVCenter | Qt::AlignRight);
                }
                break;
            case RefTreeItem::Tag:
                if (m_tagsLoading) {
                    refreshIcon.paint(painter, iconRect, Qt::AlignVCenter | Qt::AlignRight);
                }
                break;
            default:
                break;
        }
    }
}

void RefTreeDelegate::setCurrentBranch(const QString &branch)
{
    m_currentBranch = branch;
}

void RefTreeDelegate::setBranchesLoading(bool loading)
{
    if (loading) {
        m_branchesLoading++;
    } else {
        m_branchesLoading--;
    }
    m_treeView->viewport()->update();
}

void RefTreeDelegate::setRemotesLoading(bool loading)
{
    if (loading) {
        m_remotesLoading++;
    } else {
        m_remotesLoading--;
    }
    m_treeView->viewport()->update();
}

void RefTreeDelegate::setTagsLoading(bool loading)
{
    if (loading) {
        m_tagsLoading++;
    } else {
        m_tagsLoading--;
    }
    m_treeView->viewport()->update();
}
