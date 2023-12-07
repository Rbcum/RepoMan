#ifndef PAGEHOST_H
#define PAGEHOST_H

#include <QWidget>

#include "changespage.h"
#include "historypage.h"

namespace Ui {
    class PageHost;
}

class PageHost : public QWidget
{
    Q_OBJECT

public:
    explicit PageHost(const RepoProject &project);
    ~PageHost();

    void onActionPush();
    void onActionPull();
    void onActionFetch();
    void onActionClean();
    void onActionTerm();
    void onActionFolder();

    void onRefClicked(const QModelIndex &index);
    void onChangeMode();
    void refresh(const HistorySelectionArg &arg = HistorySelectionArg());

private:
    Ui::PageHost *ui;

    ChangesPage *m_changesPage;
    HistoryPage *m_historyPage;

    RepoProject m_project;
};

#endif  // PAGEHOST_H
