#ifndef NEWTABPAGE_H
#define NEWTABPAGE_H

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QWidget>

#include "global.h"
#include "repocontext.h"

namespace Ui {
    class NewTabPage;
}

class NewTabPage : public QWidget
{
    Q_OBJECT

public:
    explicit NewTabPage(const RepoContext &context);
    ~NewTabPage();
signals:
    void projectDoubleClicked(const Project &project);

private:
    Ui::NewTabPage *ui;
    RepoContext m_context;
};

class ProjectListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    ProjectListModel(QObject *parent, const RepoContext &context);
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

private:
    RepoContext m_context;
};

class ProjectListDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    ProjectListDelegate(QObject *parent);
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
        const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class ProjectListFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    ProjectListFilterModel(QObject *parent);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
};

#endif  // NEWTABPAGE_H
