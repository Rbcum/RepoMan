#ifndef COMMITDETAILSCROLLAREA_H
#define COMMITDETAILSCROLLAREA_H

#include <QLabel>
#include <QObject>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <QVBoxLayout>

#include "global.h"

class CommitDetailTextEdit : public QPlainTextEdit
{
    Q_OBJECT
public:
    CommitDetailTextEdit(QWidget *parent = nullptr);
    void setCommit(const Commit &commit, const QString &body);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QFrame *m_refFrame;
    QVBoxLayout *m_refLayout;
    QString m_clickedAnchor;

signals:
    void linkClicked(QString);
};

class BadgeLabel : public QLabel
{
public:
    BadgeLabel(QWidget *parent);
    void setBadget(const QString &text, int type);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_type;
};

class CommitDetailScrollArea : public QScrollArea
{
    Q_OBJECT
public:
    CommitDetailScrollArea(QWidget *parent);
    void setCommit(const Commit &commit, const QString &body);
    void reset();

signals:
    void linkClicked(QString);
};

#endif  // COMMITDETAILSCROLLAREA_H
