#ifndef HISTORYPAGE_H
#define HISTORYPAGE_H

#include <QFuture>
#include <QMutex>
#include <QThreadPool>
#include <QWaitCondition>
#include <QWidget>

#include "global.h"
#include "historygraphdelegate.h"
#include "historytablemodel.h"
#include "reftreemodel.h"
#include "widgets/QProgressIndicator.h"
#include "widgets/diffscrollarea.h"

namespace Ui {
    class HistoryPage;
}

class HistoryPage : public QWidget
{
    Q_OBJECT

private:
    enum UiFlags
    {
        Table = 0x0001,
        Detail = 0x0002,
        Diff = 0x0004,
        All = Table | Detail | Diff,
    };

public:
    HistoryPage(QWidget *parent);
    ~HistoryPage();

    void setCurrentProjectPath(const QString &path);
    void refresh(const HistorySelectionArg &arg = HistorySelectionArg());
    void updateUI(unsigned flags);
    void reset(unsigned flags);
    void jumpToRef(RefTreeItem *ref);

private:
    Ui::HistoryPage *ui;
    QProgressIndicator *m_indicator;
    HistoryTableModel *m_historyModel;
    HistoryGraphDelegate *m_graphDelegate;

    struct LogResult
    {
        QList<Commit> commits;
        GraphTable graphTable;

        // States
        QList<QString> nextCommits;
        QList<int> nextLaneIds;
        int newLaneId = 0;
        int laneCount = 0;
    };
    struct DetailResult
    {
        QString rawBody;
        QList<GitFile> fileList;
    };
    struct DiffResult
    {
        QList<DiffHunk> hunks;
    };

    QString m_projectPath;
    Commit m_currentCommit;
    LogResult m_logResult;
    DetailResult m_detailResult;
    DiffResult m_diffResult;

    void selectTargetRow(const HistorySelectionArg &arg);

    QThreadPool *m_logThreadPool;
    QThreadPool *m_threadPool;
    QFuture<void> m_logWorker;
    QFuture<DetailResult> m_detailWorker;
    QFuture<DiffResult> m_diffWorker;

    static void readCommits(QPromise<void> &promise, const QString &projectPath, int skip,
        int maxCount, int orderType, int branchType, bool showRemotes,
        const HistorySelectionArg &arg, LogResult &result, bool &searchHit);
    static void buildGraphTable(QPromise<void> &promise, LogResult &result);

signals:
    void logResult(HistoryPage::LogResult result, HistorySelectionArg arg, int count);
    void requestRefreshEvent();

private slots:
    void onLogResult(HistoryPage::LogResult result, HistorySelectionArg arg, int count);
    void onFetchMoreCommits(const int skip, const HistorySelectionArg &arg);
    void onCommitSelected(const QModelIndex &current, const QModelIndex &previous);
    void onFileSelected();
    void onParentLinkClicked(const QString &hash);
    void onDisplayParamsChanged();
    void onCancelLoading();
    void onTableMenuRequested(const QPoint &pos);
    void onTableDoubleClick(const QModelIndex &index);
};
#endif  // HISTORYPAGE_H
