#include "diffscrollarea.h"

#include <QCoreApplication>
#include <QPainter>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSize>
#include <QStyle>
#include <QTextBlock>
#include <QVBoxLayout>
#include <QtConcurrent>

// Performance issue, consider using QListView
#define HUNK_MAX_LINES 120

// Special line number marks
#define LN_EMPTY 0
#define LN_CHOPPED -1

DiffScrollArea::DiffScrollArea(QWidget *parent) : QScrollArea(parent)
{
    viewport()->setStyleSheet("QWidget#diffScrollAreaWidget{background:White;}");
}

void DiffScrollArea::setDiffHunks(const QList<DiffHunk> hunks)
{
    reset();
    this->diffHunks = hunks;
    for (int i = 0; i < diffHunks.size(); ++i) {
        DiffTextEdit *edit = new DiffTextEdit(this);
        edit->setDiffHunk(diffHunks.at(i));
        widget()->layout()->addWidget(edit);
    }
}

void DiffScrollArea::reset()
{
    diffHunks.clear();
    if (widget()->layout() == nullptr) {
        QVBoxLayout *layout = new QVBoxLayout(widget());
        layout->setAlignment(Qt::AlignTop);
        layout->setContentsMargins(0, 0, 0, 0);
    } else {
        QLayoutItem *child;
        while ((child = widget()->layout()->takeAt(0)) != nullptr) {
            delete child->widget();
            delete child;
        }
    }
}

DiffTextEdit::DiffTextEdit(DiffScrollArea *parent) : QPlainTextEdit(parent), m_scrollArea(parent)
{
    m_customFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_customFont.setPointSize(9);
    setFont(m_customFont);
    setStyleSheet("QPlainTextEdit{border-bottom: 1px solid #B8B8B8; border-top: 1px solid #B8B8B8;"
                  "border-right: none; border-left: none;}");
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setReadOnly(true);

    m_lineNumberArea = new LineNumberArea(this);

    QWidget *scrollBarMargin = new QWidget(this);
    scrollBarMargin->setFixedWidth(lineNumberAreaWidth());
    addScrollBarWidget(scrollBarMargin, Qt::AlignLeft);

    connect(this, &DiffTextEdit::blockCountChanged, this, &DiffTextEdit::updateSizes);
    connect(this, &DiffTextEdit::updateRequest, this, &DiffTextEdit::updateLineNumberArea);

    updateSizes(0);
}

bool DiffTextEdit::eventFilter(QObject *watched, QEvent *event)
{
    // Don't scroll the h-bar when scroll vertically
    if (watched == reinterpret_cast<QObject *>(horizontalScrollBar()) &&
        event->type() == QEvent::Wheel) {
        QCoreApplication::postEvent(m_scrollArea->verticalScrollBar(), event->clone());
        return true;
    }
    return QPlainTextEdit::eventFilter(watched, event);
}

int DiffTextEdit::lineNumberAreaWidth()
{
    //    int digits = 1;
    //    int max = qMax(1, blockCount());
    //    while (max >= 10) {
    //        max /= 10;
    //        ++digits;
    //    }

    int space = 15 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * 5 * 2;

    return space;
}

void DiffTextEdit::setDiffHunk(const DiffHunk &hunk)
{
    this->m_hunk = hunk;
    for (int i = 0; i < hunk.lines.size(); ++i) {
        const QString &line = hunk.lines.at(i);
        if (line.startsWith("+") || line.startsWith("-")) {
            appendPlainText(line.mid(1));
        } else {
            appendPlainText(line);
        }
    }
    highlightDiffLines();
    horizontalScrollBar()->setValue(0);
}

void DiffTextEdit::updateSizes(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void DiffTextEdit::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect())) updateSizes(0);
}

void DiffTextEdit::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QTextBlock block = firstVisibleBlock();
    int height = qRound(blockBoundingGeometry(block).height() * (blockCount() + 2));
    setFixedHeight(height);

    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void DiffTextEdit::highlightDiffLines()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    moveCursor(QTextCursor::Start);
    for (int i = 0; i < m_hunk.lines.size(); ++i) {
        QString line = m_hunk.lines.at(i);
        QColor color;
        if (line.startsWith("+")) {
            color = QColor(Qt::green).lighter(185);
        } else if (line.startsWith("-")) {
            color = QColor(Qt::red).lighter(185);
        }
        if (color.isValid()) {
            QTextEdit::ExtraSelection selection;
            selection.format.setBackground(color);
            selection.format.setProperty(QTextFormat::FullWidthSelection, true);
            selection.cursor = textCursor();
            extraSelections.append(selection);
        }
        moveCursor(QTextCursor::NextBlock);
    }

    setExtraSelections(extraSelections);
}

void DiffTextEdit::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(m_lineNumberArea);
    painter.setFont(m_customFont);
    painter.setPen(Qt::black);
    painter.fillRect(event->rect(), QColor(0xF8F9FA));
    if (horizontalScrollBar()->isVisible()) {
        const int sbHeight = horizontalScrollBar()->height();
        painter.fillRect(lineNumberAreaWidth() - 1, contentsRect().bottom() - sbHeight, 1, sbHeight,
            QColor(0xB8B8B8));
    }

    QTextBlock block = firstVisibleBlock();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());
    int width = m_lineNumberArea->width();
    for (int i = 0; i < m_hunk.lines.size(); ++i) {
        int oldLN = m_hunk.oldLNs.at(i);
        int newLN = m_hunk.newLNs.at(i);
        if (oldLN > LN_EMPTY) {
            painter.drawText(
                1, top, width / 2, fontMetrics().height(), Qt::AlignLeft, QString::number(oldLN));
        }
        if (newLN > LN_EMPTY) {
            painter.drawText(width / 2, top, width / 2, fontMetrics().height(), Qt::AlignLeft,
                QString::number(newLN));
        }
        if (newLN == LN_CHOPPED) {
            painter.drawText(
                width / 2, top, width / 2, fontMetrics().height(), Qt::AlignLeft, "...");
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
    }
}

namespace global {
    QList<DiffHunk> parseDiffHunks(const QString &diffText)
    {
        static QRegularExpression splitRE("\r?\n");
        QStringList lines = diffText.trimmed().split(splitRE);

        QList<DiffHunk> hunks;
        bool firstHunk = true;
        bool skipHunkBody = false;
        DiffHunk hk;
        int oldLN = 0;
        int newLN = 0;
        int n = 0;
        // Skip header
        while (n < lines.size() && !lines[n].startsWith("@@")) {
            n++;
        }
        for (int i = n; i < lines.size(); i++) {
            QString line = lines[i];
            if (line.startsWith("@@")) {
                if (!firstHunk) {
                    hunks.append(hk);
                    hk = DiffHunk();
                    skipHunkBody = false;
                }
                firstHunk = false;
                QStringList nums = line.split(" ");
                //                qDebug() << line;
                QStringList oldNums = nums[1].mid(1).split(",");
                QStringList newNums = nums[2].mid(1).split(",");
                hk.oldStart = oldNums[0].toInt();
                hk.oldTotal = oldNums.last().toInt();
                hk.newStart = newNums[0].toInt();
                hk.newTotal = newNums.last().toInt();
                //                qDebug() << hk.oldStart << hk.oldTotal << hk.newStart <<
                //                hk.newTotal;
                oldLN = hk.oldStart;
                newLN = hk.newStart;
            } else if (!skipHunkBody) {
                hk.lines.append(line);
                if (line.startsWith(" ")) {
                    hk.oldLNs.append(oldLN++);
                    hk.newLNs.append(newLN++);
                } else if (line.startsWith("-")) {
                    hk.oldLNs.append(oldLN++);
                    hk.newLNs.append(LN_EMPTY);
                } else if (line.startsWith("+")) {
                    hk.oldLNs.append(LN_EMPTY);
                    hk.newLNs.append(newLN++);
                } else if (line == "\\ No newline at end of file") {
                    hk.newLNs.append(LN_EMPTY);
                    hk.oldLNs.append(LN_EMPTY);
                }
                if (hk.lines.size() >= HUNK_MAX_LINES) {
                    skipHunkBody = true;
                    hk.lines.append(" ...");
                    hk.newLNs.append(LN_CHOPPED);
                    hk.oldLNs.append(LN_CHOPPED);
                }
            }
            if (i == lines.size() - 1) {
                hunks.append(hk);
            }
        }
        return hunks;
    }
}  // namespace global
