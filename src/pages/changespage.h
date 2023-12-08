#ifndef COMMITPAGE_H
#define COMMITPAGE_H

#include <QFutureWatcher>
#include <QMutex>
#include <QPromise>
#include <QTableWidgetItem>
#include <QThreadPool>
#include <QWaitCondition>
#include <QWidget>

#include "global.h"
#include "pages/historytablemodel.h"
#include "widgets/QProgressIndicator.h"
#include "widgets/diffscrollarea.h"

namespace Ui {
    class ChangesPage;
}

class ChangesPage : public QWidget
{
    Q_OBJECT

public:
    ChangesPage(QWidget *parent, const RepoProject &project);
    ~ChangesPage();

    void refresh();
    void updateUI(unsigned flags);
    void reset(unsigned flags);

private:
    enum UiFlags
    {
        List = 0x0001,
        Diff = 0x0002,
        Commit = 0x0004,
        All = List | Diff | Commit,
    };

    Ui::ChangesPage *ui;
    QProgressIndicator *m_indicator;

    RepoProject m_project;
    QList<GitFile> m_stagedList;
    QList<GitFile> m_unstagedList;
    QList<DiffHunk> m_diffHunks;
    int m_stagedSelection = -1;
    int m_unstagedSelection = -1;

    struct ChangesResult
    {
        QList<GitFile> unstagedList;
        QList<GitFile> stagedList;
        int fileCount = 0;
    };
    struct DiffResult
    {
        QList<DiffHunk> hunks;
    };
    QFuture<ChangesResult> m_changesWorker;
    QFuture<DiffResult> m_diffWorker;
    QFuture<QString> m_amendWorker;
    void getChangesAsync(const QString &projectPath, QProgressIndicator *const indicator);
    void getDiffAsync(const QString &projectPath, const GitFile &file, bool staged,
        QProgressIndicator *const indicator);
    void updateGitIndex(QObject *stageSender);

signals:
    void commitEvent(const HistorySelectionArg &arg = HistorySelectionArg());
    void newChangesEvent(int count);

private slots:
    void onFileDoubleClicked(int row, int column);
    void onFileSelected(QTableWidgetItem *current, QTableWidgetItem *previous);
    void onTableButtonClicked();
    void onAmendToggled(bool checked);
    void onCommit();

    // QWidget interface
protected:
    void showEvent(QShowEvent *event) override;
};

#endif  // COMMITPAGE_H
