#ifndef HISTORYGRAPHDELEGATE_H
#define HISTORYGRAPHDELEGATE_H

#include <QStyledItemDelegate>

class GraphLane
{
public:
    enum Type
    {
        Vertical,
        Commit,
        Twig,
        Root,
    };

    int laneId = -1;
    int slot = -1;
    Type type;
    virtual void draw(QPainter *painter, const QRect &rect) const = 0;
    virtual ~GraphLane() = default;
};

QDebug operator<<(QDebug dbg, const GraphLane &l);

typedef QList<QList<QSharedPointer<GraphLane>>> GraphTable;
typedef QList<QSharedPointer<GraphLane>> GraphTableRow;

class VerticalLane : public GraphLane
{
public:
    int indentation = 0;

    VerticalLane()
    {
        type = GraphLane::Vertical;
    }
    void draw(QPainter *painter, const QRect &rect) const override;
};

class CommitLane : public GraphLane
{
public:
    bool isHead = false;
    bool isRoot = false;

    CommitLane()
    {
        type = GraphLane::Commit;
    }
    void draw(QPainter *painter, const QRect &rect) const override;
    void drawVertex(QPainter *painter, const QRect &rect) const;
};

class TwigLane : public GraphLane
{
public:
    int targetSlot = -1;

    TwigLane()
    {
        type = GraphLane::Twig;
    }
    void draw(QPainter *painter, const QRect &rect) const override;
};

class RootLane : public GraphLane
{
public:
    int targetSlot = -1;

    RootLane()
    {
        type = GraphLane::Root;
    }
    void draw(QPainter *painter, const QRect &rect) const override;
};

class HistoryGraphDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit HistoryGraphDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
        const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void reset();
    void addGraphTable(const GraphTable &graphTable);

private:
    GraphTable m_graphTable;

    void paintBadges(QPainter *painter, int laneId, int type, const QStringList &badges,
        const QRect &contentRect, int &badgeOffset) const;
};

#endif  // HISTORYGRAPHDELEGATE_H
