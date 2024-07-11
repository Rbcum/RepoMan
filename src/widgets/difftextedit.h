#ifndef DIFFPLAINTEXTEDIT_H
#define DIFFPLAINTEXTEDIT_H

#include <QObject>
#include <QPlainTextEdit>
#include <QWidget>

#include "diffutils.h"

class DiffTextEdit : public QPlainTextEdit
{
    Q_OBJECT
public:
    DiffTextEdit(QWidget *parent);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

    void setDiffHunks(const QList<DiffHunk> &hunks);
    void setMode(DiffMode mode);
    void reset();
    int firstVisibleBlockNumber();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private slots:
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    QFont m_fixedFont;
    QWidget *m_lineNumberArea;
    QList<DiffHunk> m_hunks;
    DiffMode m_mode;

    void applyLineStyles();
    void updateLineNumberAreaWidth();
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

#endif  // DIFFPLAINTEXTEDIT_H
