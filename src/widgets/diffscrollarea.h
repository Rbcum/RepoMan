#ifndef DIFFTEXTEDIT_H
#define DIFFTEXTEDIT_H

#include <QFutureWatcher>
#include <QMutex>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <QThreadPool>
#include <QWaitCondition>

struct DiffHunk
{
    DiffHunk()
    {
    }

    DiffHunk(int oldStart, int oldTotal, int newStart, int newTotal, QStringList lines,
        QList<int> oldLNs, QList<int> newLNs)
        : oldStart(oldStart),
          oldTotal(oldTotal),
          newStart(newStart),
          newTotal(newTotal),
          lines(lines),
          oldLNs(oldLNs),
          newLNs(newLNs)
    {
    }

    int oldStart;
    int oldTotal;
    int newStart;
    int newTotal;
    QStringList lines;
    QList<int> oldLNs;
    QList<int> newLNs;
};

namespace global {
    extern QList<DiffHunk> parseDiffHunks(const QString &diffText);
}

class DiffScrollArea : public QScrollArea
{
    Q_OBJECT
public:
    DiffScrollArea(QWidget *parent);
    void setDiffHunks(const QList<DiffHunk> hunks);
    void reset();

private:
    QList<DiffHunk> diffHunks;
};

class DiffTextEdit : public QPlainTextEdit
{
    Q_OBJECT
public:
    DiffTextEdit(DiffScrollArea *parent = nullptr);
    bool eventFilter(QObject *watched, QEvent *event) override;

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

    void setDiffHunk(const DiffHunk &hunk);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateSizes(int newBlockCount);
    void highlightDiffLines();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    QFont m_customFont;
    QWidget *m_lineNumberArea;
    DiffScrollArea *m_scrollArea;
    DiffHunk m_hunk;
};

class LineNumberArea : public QWidget
{
public:
    LineNumberArea(DiffTextEdit *edit) : QWidget(edit), m_textEdit(edit)
    {
    }

    QSize sizeHint() const override
    {
        return QSize(m_textEdit->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        m_textEdit->lineNumberAreaPaintEvent(event);
    }

private:
    DiffTextEdit *m_textEdit;
};

#endif  // DIFFTEXTEDIT_H
