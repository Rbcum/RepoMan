#include "historygraphdelegate.h"

#include <QApplication>
#include <QPainter>
#include <QPainterPath>

#include "historytablemodel.h"
#include "themes/theme.h"

// #define DEBUG_DRAW

HistoryGraphDelegate::HistoryGraphDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
}

void HistoryGraphDelegate::paint(
    QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const int row = index.row();
    const int column = index.column();

    QStyleOptionViewItem opt = option;
    opt.state &= ~QStyle::State_HasFocus;

    painter->save();
    if (index.column() == 0) {
        QStyledItemDelegate::paint(painter, opt, index);

        if (row < m_graphTable.size()) {
            const QRect &rect = option.rect;

            painter->setClipRect(rect);
            painter->setRenderHint(QPainter::Antialiasing);

            const GraphTableRow &rowLanes = m_graphTable[row];
            QSharedPointer<GraphLane> commitLane;
            for (const QSharedPointer<GraphLane> &lane : rowLanes) {
                painter->save();
                lane->draw(painter, rect);
                painter->restore();
                if (lane->type == GraphLane::Commit) {
                    commitLane = lane;
                }
            }

            if (commitLane) {
                painter->save();
                commitLane.staticCast<CommitLane>()->drawVertex(painter, rect);
                painter->restore();
            }
        }
    } else {
        const QWidget *widget = option.widget;
        QStyle *style = widget ? widget->style() : QApplication::style();
        initStyleOption(&opt, index);
        const QRect contentRect = opt.rect.adjusted(5, 0, -5, 0);
        const QString text = opt.text;
        opt.text = "";
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter);

        const Commit &commit = index.data(HistoryTableModel::CommitRole).value<Commit>();

        painter->setClipRect(contentRect);

        int badgeOffset = 0;
        if (column == 1) {
            int laneId = -1;
            if (row < m_graphTable.size()) {
                const GraphTableRow &rowLanes = m_graphTable[row];
                for (const QSharedPointer<GraphLane> &l : rowLanes) {
                    if (l->type == GraphLane::Commit) {
                        laneId = l->laneId;
                        break;
                    }
                }
            }

            paintBadges(painter, laneId, commit, contentRect, badgeOffset);
        }

        QRect subjectRect = contentRect.adjusted(badgeOffset + 6, 0, 0, 0);
        if (subjectRect.isValid()) {
            QPalette::ColorGroup cg =
                opt.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
            if (cg == QPalette::Normal && !(opt.state & QStyle::State_Active))
                cg = QPalette::Inactive;
            if (commit.isHEAD) {
                QFont font = painter->font();
                font.setBold(true);
                painter->setFont(font);
                painter->setPen(opt.palette.color(cg, QPalette::Text));
            } else if (opt.state & QStyle::State_Selected) {
                painter->setPen(opt.palette.color(cg, QPalette::HighlightedText));
            } else {
                painter->setPen(opt.palette.color(cg, QPalette::Text));
            }
            painter->drawText(subjectRect, Qt::AlignLeft | Qt::AlignVCenter, text);
        }
    }
    painter->restore();
}

static const QColor &getLaneColor(int laneId)
{
    static const QList<QColor> S_COLORS = {
        QColor(0x6FAECD),
        QColor(0xC19955),
        QColor(0x55C179),
        QColor(0xC16C55),
        QColor(0xB3B456),
        QColor(0xC1558C),
        QColor(0x65563E),
        QColor(0xD97E19),
        QColor(0xD13B2B),
        QColor(0x6D134F),
    };
    return S_COLORS[qMax(0, laneId) % 10];
}

void HistoryGraphDelegate::paintBadges(QPainter *painter, int laneId, const Commit &commit,
    const QRect &contentRect, int &badgeOffset) const
{
    static int nailWidth = 16;
    static int lMargin = 3;
    static int rMargin = 6;
    static int vMargin = 2;
    static int badgeSpacing = 3;
    static int cornerRadius = 2;
    static qreal iconPadding = 3.2;

    int type = 0;
    int count = 0;
    QString text;
    if (commit.heads.size() > 0) {
        type = 0;
        count += commit.heads.size();
        text = commit.heads.first();
    }
    if (commit.remotes.size() > 0) {
        if (count == 0) {
            type = 0;
            text = commit.remotes.first();
        }
        count += commit.remotes.size();
    }
    if (commit.tags.size() > 0) {
        if (count == 0) {
            type = 1;
            text = commit.tags.first();
        }
        count += commit.tags.size();
    }
    if (count == 0) {
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    QFont font = painter->font();
    font.setPointSize(9);
    painter->setFont(font);

    int width = painter->fontMetrics().horizontalAdvance(text) + nailWidth + lMargin + rMargin;
    QRectF badgeRect(contentRect.x() + 0.5, contentRect.top() + vMargin + 0.5, width,
        contentRect.height() - vMargin * 2 - 1);
    QColor borderColor = utils::creatorTheme()->color(utils::Theme::BadgeBorderColor);
    QColor backgroundColor = utils::creatorTheme()->color(utils::Theme::BadgeBackgoundColor);

    // Badge
    QPainterPath path;
    path.addRoundedRect(badgeRect, cornerRadius, cornerRadius);
    painter->setPen(QPen(borderColor, 1));
    painter->fillPath(path, backgroundColor);
    painter->drawPath(path);

    // Nail
    path.clear();
    path.setFillRule(Qt::WindingFill);
    QRectF nailRect = badgeRect.adjusted(0, 0, -(width - nailWidth), 0);
    path.addRoundedRect(nailRect, cornerRadius, cornerRadius);
    path.addRect(nailRect.adjusted(cornerRadius, 0, 0, 0));
    painter->fillPath(path.simplified(), getLaneColor(laneId));

    // Icon
    painter->setPen(QPen(Qt::white, 1.4));
    QRectF iconRect = nailRect.adjusted(iconPadding, iconPadding, -iconPadding, -iconPadding);
    switch (type) {
        case 0:  // branch
        case 1:  // remote
            painter->drawLine(iconRect.center().x() - 1, iconRect.top() + 1,
                iconRect.center().x() - 1, iconRect.bottom());
            painter->drawLine(iconRect.center().x() - 1, iconRect.center().y() + 1,
                iconRect.center().x() + 3, iconRect.center().y() - 2);
            break;
        case 2:  // tag
            painter->save();
            painter->translate(iconRect.center().x() - 0.9, iconRect.center().y() - 0.5);
            painter->rotate(-45);
            path.clear();
            path.moveTo(3.2, -iconRect.height() * 0.17);
            path.lineTo(3.2, iconRect.height() / 2);
            path.lineTo(-3.2, iconRect.height() / 2);
            path.lineTo(-3.2, -iconRect.height() * 0.17);
            path.lineTo(0, -iconRect.height() / 2);
            path.closeSubpath();
            painter->drawPath(path);
            painter->restore();
            break;
    }

    // Text
    QRectF textRect = badgeRect.adjusted(nailWidth + lMargin, 0, -rMargin, 0);
    painter->setPen(qApp->palette().color(QPalette::WindowText));
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);

    // Stacks
    painter->setPen(QPen(borderColor, 1));
    QRectF stackRect = badgeRect.adjusted(width, 0, badgeSpacing, 0);
    for (int i = 1; i < count; ++i) {
        path.clear();
        path.moveTo(stackRect.left(), stackRect.top() + cornerRadius);
        path.arcTo(stackRect.left() - cornerRadius * 2, stackRect.top(), cornerRadius * 2,
            cornerRadius * 2, 0, 90);
        path.lineTo(stackRect.right() - cornerRadius, stackRect.top());
        path.arcTo(stackRect.right() - cornerRadius * 2, stackRect.top(), cornerRadius * 2,
            cornerRadius * 2, 90, -90);
        path.lineTo(stackRect.right(), stackRect.bottom() - cornerRadius);
        path.arcTo(stackRect.right() - cornerRadius * 2, stackRect.bottom() - cornerRadius * 2,
            cornerRadius * 2, cornerRadius * 2, 0, -90);
        path.lineTo(stackRect.left() - cornerRadius, stackRect.bottom());
        path.arcTo(stackRect.left() - cornerRadius * 2, stackRect.bottom() - cornerRadius * 2,
            cornerRadius * 2, cornerRadius * 2, -90, 90);
        path.closeSubpath();

        painter->fillPath(path, backgroundColor);
        painter->drawPath(path);

        stackRect.translate(badgeSpacing, 0);
    }
    badgeOffset = stackRect.right() - contentRect.left() - badgeSpacing;

    painter->restore();
}

QSize HistoryGraphDelegate::sizeHint(
    const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return QStyledItemDelegate::sizeHint(option, index);
}

void HistoryGraphDelegate::reset()
{
    this->m_graphTable.clear();
}

void HistoryGraphDelegate::addGraphTable(const GraphTable &graphTable)
{
    this->m_graphTable.append(graphTable);
}

static qreal LANE_WIDTH = 2;
static qreal LANE_SPACING = 10;
static qreal SLANT_JOINT = 4;
static qreal VERTEX_RADIUS = 3;
static qreal MARGIN_START = 10;

void VerticalLane::draw(QPainter *painter, const QRect &rect) const
{
    qreal centerX = rect.x() + MARGIN_START + slot * (LANE_WIDTH + LANE_SPACING) + LANE_WIDTH / 2;
    //    qreal centerY = rect.center().y();

    const QColor &color = getLaneColor(laneId);
    painter->setPen(QPen(color, LANE_WIDTH));

    if (indentation == 0) {
        painter->drawLine(centerX, rect.top(), centerX, rect.bottom());
    } else {
        qreal centerX2 = centerX - indentation * (LANE_SPACING + LANE_WIDTH);
        QPointF points[4] = {
            QPointF(centerX, rect.top()),
            QPointF(centerX, rect.top() + SLANT_JOINT),
            QPointF(centerX2, rect.bottom() - SLANT_JOINT),
            QPointF(centerX2, rect.bottom()),
        };

        painter->drawPolyline(points, 4);
    }
}

void CommitLane::draw(QPainter *painter, const QRect &rect) const
{
    qreal centerX = rect.x() + MARGIN_START + slot * (LANE_WIDTH + LANE_SPACING) + LANE_WIDTH / 2;
    qreal centerY = rect.center().y();

    const QColor &color = getLaneColor(laneId);
    painter->setPen(QPen(color, LANE_WIDTH));
    painter->setBrush(color);

    if (isHead && isRoot) {
    } else if (isHead) {
        // head commit
        painter->drawLine(centerX, centerY, centerX, rect.bottom());
    } else if (isRoot) {
        // root commit
        painter->drawLine(centerX, rect.top(), centerX, centerY);
    } else {
        // middle
        painter->drawLine(centerX, rect.top(), centerX, rect.bottom());
    }
}

void CommitLane::drawVertex(QPainter *painter, const QRect &rect) const
{
    qreal centerX = rect.x() + MARGIN_START + slot * (LANE_WIDTH + LANE_SPACING) + LANE_WIDTH / 2;
    qreal centerY = rect.center().y();

    const QColor &color = getLaneColor(laneId);
    painter->setPen(QPen(color, LANE_WIDTH));
    painter->setBrush(color);

    painter->drawEllipse(
        centerX - VERTEX_RADIUS, centerY - VERTEX_RADIUS, VERTEX_RADIUS * 2, VERTEX_RADIUS * 2);

#ifdef DEBUG_DRAW
    painter->setPen(QPen(Qt::black));
    painter->drawText(centerX + 5, centerY + 3, QString::number(laneId));
#endif
}

void TwigLane::draw(QPainter *painter, const QRect &rect) const
{
    qreal centerX = rect.x() + MARGIN_START + slot * (LANE_WIDTH + LANE_SPACING) + LANE_WIDTH / 2;
    qreal centerY = rect.center().y();
    qreal centerX2 =
        rect.x() + MARGIN_START + targetSlot * (LANE_WIDTH + LANE_SPACING) + LANE_WIDTH / 2;

    QPointF points[4] = {
        QPointF(centerX, rect.bottom()),
        QPointF(centerX, rect.bottom() - SLANT_JOINT),
        QPointF(centerX2, centerY),
    };

    const QColor &color = getLaneColor(laneId);
    painter->setPen(QPen(color, LANE_WIDTH));
    painter->setBrush(color);
    painter->drawPolyline(points, 3);

#ifdef DEBUG_DRAW
    painter->setPen(QPen(Qt::black));
    painter->drawText(centerX + 1, centerY + 10, QString::number(laneId));
#endif
}

void RootLane::draw(QPainter *painter, const QRect &rect) const
{
    qreal centerX = rect.x() + MARGIN_START + slot * (LANE_WIDTH + LANE_SPACING) + LANE_WIDTH / 2;
    qreal centerY = rect.center().y();
    qreal centerX2 =
        rect.x() + MARGIN_START + targetSlot * (LANE_WIDTH + LANE_SPACING) + LANE_WIDTH / 2;

    QPointF points[4] = {
        QPointF(centerX, rect.top()),
        QPointF(centerX, rect.top() + SLANT_JOINT),
        QPointF(centerX2, centerY),
    };

    const QColor &color = getLaneColor(laneId);
    painter->setPen(QPen(color, LANE_WIDTH));
    painter->drawPolyline(points, 3);
}

QDebug operator<<(QDebug dbg, const GraphLane &l)
{
    QDebugStateSaver saver(dbg);
    switch (l.type) {
        case GraphLane::Vertical:
            {
                const VerticalLane &vl = static_cast<const VerticalLane &>(l);
                dbg.nospace() << QString("VerticalLane (laneId:%1, slot:%2, indentation:%3)")
                                     .arg(vl.laneId)
                                     .arg(vl.slot)
                                     .arg(vl.indentation);
            }
            break;
        case GraphLane::Commit:
            {
                const CommitLane &cl = static_cast<const CommitLane &>(l);
                dbg.nospace() << QString("CommitLane   (laneId:%1, slot:%2, isHead:%3, isRoot:%4)")
                                     .arg(cl.laneId)
                                     .arg(cl.slot)
                                     .arg(cl.isHead)
                                     .arg(cl.isRoot);
            }
            break;
        case GraphLane::Twig:
            {
                const TwigLane &tl = static_cast<const TwigLane &>(l);
                dbg.nospace() << QString("TwigLane     (laneId:%1, slot:%2, targetSlot:%3)")
                                     .arg(tl.laneId)
                                     .arg(tl.slot)
                                     .arg(tl.targetSlot);
            }
            break;
        case GraphLane::Root:
            {
                const RootLane &rl = static_cast<const RootLane &>(l);
                dbg.nospace() << QString("RootLane     (laneId:%1, slot:%2, targetSlot:%3)")
                                     .arg(rl.laneId)
                                     .arg(rl.slot)
                                     .arg(rl.targetSlot);
            }
            break;
    }
    return dbg;
}
