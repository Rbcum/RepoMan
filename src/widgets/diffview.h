#ifndef DIFFVIEW_H
#define DIFFVIEW_H

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
    void updateUI();
    void reset();

private:
    Ui::DiffView *ui;

    GitFile m_file;
    QList<DiffHunk> m_hunks;
    bool m_updatingUI = false;

    void syncScrollBar(QScrollBar *leftScrollBar, QScrollBar *rightScrollBar);
};

#endif  // DIFFVIEW_H
