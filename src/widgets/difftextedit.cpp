#include "difftextedit.h"

#include <QPainter>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTextBlock>
#include <QTimer>

#include "themes/theme.h"

using namespace utils;

DiffTextEdit::DiffTextEdit(QWidget *parent) : QPlainTextEdit(parent)
{
    m_fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_fixedFont.setPointSize(9);
    setFont(m_fixedFont);

    m_lineNumberArea = new LineNumberArea(this);
    m_fixedFont.setPointSizeF(7.5);
    m_lineNumberArea->setFont(m_fixedFont);

    setLineWrapMode(QPlainTextEdit::NoWrap);
    setReadOnly(true);

    connect(this, &DiffTextEdit::updateRequest, this, &DiffTextEdit::updateLineNumberArea);
}

void DiffTextEdit::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), creatorTheme()->color(Theme::LineNumberBackground));
    painter.setPen(creatorTheme()->color(Theme::LineNumber));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int hunkIndex;
    int lineIndex;
    findTargetHunk(m_mode, m_hunks, blockNumber, hunkIndex, lineIndex);

    int charWidth = m_lineNumberArea->fontMetrics().horizontalAdvance(QLatin1Char('9'));
    qreal blockHeight = blockBoundingRect(block).height();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockHeight);

    while (block.isValid() && top <= event->rect().bottom()) {
        auto &hunk = m_hunks[hunkIndex];
        if (bottom >= event->rect().top()) {
            switch (m_mode) {
                case SplitOld:
                    {
                        int ln = hunk.splitOldLNs[lineIndex];
                        if (ln) {
                            int width = m_lineNumberArea->width() - charWidth;
                            painter.drawText(0, top, width, blockHeight,
                                Qt::AlignRight | Qt::AlignVCenter, QString::number(ln));
                        }
                        break;
                    }
                case SplitNew:
                    {
                        int ln = hunk.splitNewLNs[lineIndex];
                        if (ln) {
                            painter.drawText(0, top, m_lineNumberArea->width() - charWidth,
                                blockHeight, Qt::AlignRight | Qt::AlignVCenter,
                                QString::number(ln));
                        }
                        break;
                    }
                default:
                    int oldLN = hunk.unifiedOldLNs[lineIndex];
                    int newLN = hunk.unifiedNewLNs[lineIndex];
                    if (oldLN) {
                        int width = (m_lineNumberArea->width() - charWidth) / 2;
                        painter.drawText(0, top, width, blockHeight,
                            Qt::AlignRight | Qt::AlignVCenter, QString::number(oldLN));
                    }
                    if (newLN) {
                        painter.drawText(0, top, m_lineNumberArea->width() - charWidth, blockHeight,
                            Qt::AlignRight | Qt::AlignVCenter, QString::number(newLN));
                    }
                    break;
            }
        }

        lineIndex++;
        if (lineIndex >= hunk.lines[m_mode].size()) {
            lineIndex = 0;
            hunkIndex++;
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockHeight);
        ++blockNumber;
    }
}

int DiffTextEdit::lineNumberAreaWidth()
{
    if (m_hunks.isEmpty()) return 0;
    auto &lastHunk = m_hunks.last();
    int max;
    switch (m_mode) {
        case SplitOld:
            max = lastHunk.oldStart + lastHunk.oldTotal;
            break;
        case SplitNew:
            max = lastHunk.newStart + lastHunk.newTotal;
            break;
        default:
            max =
                qMax(lastHunk.newStart + lastHunk.newTotal, lastHunk.oldStart + lastHunk.oldTotal);
            break;
    }
    int digits = 1;
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    int charWidth = m_lineNumberArea->fontMetrics().horizontalAdvance(QLatin1Char('9'));
    int space =
        (m_mode == Unified ? 3 + 2 * digits /*_xxx_xxx_*/ : 2 + digits /*_xxx_*/) * charWidth;
    return space;
}

void DiffTextEdit::setDiffHunks(const QList<DiffHunk> &hunks)
{
    m_hunks = hunks;

    clear();
    updateLineNumberAreaWidth();

    for (auto &hunk : hunks) {
        const QStringList &lines = hunk.lines[m_mode];
        for (auto &line : lines) {
            if (line.startsWith("@") || line.isEmpty()) {
                appendPlainText(line);
            } else if (line.startsWith("\\")) {
                appendPlainText(line.sliced(2));
            } else {
                appendPlainText(line.sliced(1));
            }
        }
    }
    applyLineStyles();
    moveCursor(QTextCursor::Start);
}

void DiffTextEdit::setMode(DiffMode mode)
{
    m_mode = mode;
}

void DiffTextEdit::reset()
{
    setDiffHunks({});
}

int DiffTextEdit::firstVisibleBlockNumber()
{
    return firstVisibleBlock().blockNumber();
}

void DiffTextEdit::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void DiffTextEdit::paintEvent(QPaintEvent *event)
{
    QPlainTextEdit::paintEvent(event);
}

void DiffTextEdit::applyLineStyles()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    moveCursor(QTextCursor::Start);

    for (auto &hunk : m_hunks) {
        for (auto &line : hunk.lines[m_mode]) {
            QColor color;
            bool foreground = false;
            if (line.startsWith("+")) {
                color = creatorTheme()->color(Theme::DiffLineAdd);
            } else if (line.startsWith("-")) {
                color = creatorTheme()->color(Theme::DiffLineRemove);
            } else if (line.startsWith("@") || line.startsWith("\\")) {
                foreground = true;
                color = creatorTheme()->color(Theme::DiffLineMeta);
            } else if (line.isEmpty()) {
                color = creatorTheme()->color(Theme::DiffLineDummy);
            }
            if (color.isValid()) {
                QTextEdit::ExtraSelection selection;
                if (foreground) {
                    selection.format.setForeground(color);
                } else {
                    selection.format.setBackground(color);
                }
                selection.format.setProperty(QTextFormat::FullWidthSelection, true);
                selection.cursor = textCursor();
                extraSelections.append(selection);
            }
            moveCursor(QTextCursor::NextBlock);
        }
    }

    setExtraSelections(extraSelections);
}

void DiffTextEdit::updateLineNumberAreaWidth()
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void DiffTextEdit::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy) {
        m_lineNumberArea->scroll(0, dy);
    } else {
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
    }
    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth();
    }
}
