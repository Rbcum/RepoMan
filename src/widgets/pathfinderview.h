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
    void setTitle(const QString &title);

signals:
    void pathClicked(const QString &newPath);

private slots:
    void onPathSegmentClicked();

private:
    QList<QString> m_pathSegments;
    QString m_title;
    QString m_path;

    void updateUI();
    void clearPathViews();
    void addSegmentView(const QString &text, int index);
    void addSeperatorView(const QString &seperator);
};

#endif  // PATHFINDERVIEW_H
