#ifndef PATHFINDERVIEW_H
#define PATHFINDERVIEW_H

#include <QScrollArea>

class PathFinderView : public QScrollArea
{
    Q_OBJECT
public:
    PathFinderView(QWidget *parent);
    ~PathFinderView();
    void setPath(const QString &path);
    void setHiddenParent(const QString &hiddenParent);

signals:
    void pathClicked(const QString &newPath);

private slots:
    void onPathSegmentClicked();

private:
    QList<QString> m_pathSegments;
    QString m_hiddenParent;
    QString m_path;

    void updateUI();
    void clearPathViews();
};

#endif  // PATHFINDERVIEW_H
