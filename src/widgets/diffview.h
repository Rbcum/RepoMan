#ifndef DIFFVIEW_H
#define DIFFVIEW_H

#include <QActionGroup>
#include <QScrollBar>
#include <QWidget>

#include "diffutils.h"
#include "global.h"

namespace Ui {
    class DiffView;
}

class DiffView : public QWidget
{
    Q_OBJECT

public:
    explicit DiffView(QWidget *parent = nullptr);
    ~DiffView();
    void setDiffHunks(const GitFile &file, const QList<DiffHunk> &hunks);
    int getContextLines();
    void updateUI();
    void reset();

signals:
    void diffParametersChanged();

private slots:
    void onMenuAction();

private:
    Ui::DiffView *ui;
    QActionGroup *m_contextLinesGroup;

    GitFile m_file;
    QList<DiffHunk> m_hunks;
    bool m_updatingUI = false;

    void syncScrollBar(QScrollBar *leftScrollBar, QScrollBar *rightScrollBar);
};

#endif  // DIFFVIEW_H
