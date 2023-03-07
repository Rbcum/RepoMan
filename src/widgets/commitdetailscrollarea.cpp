#include "commitdetailscrollarea.h"

#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QScrollBar>
#include <QStyle>
#include <QTextBlock>
#include <QTextCursor>
#include <QVBoxLayout>

CommitDetailTextEdit::CommitDetailTextEdit(QWidget *parent) : QPlainTextEdit(parent)
{
    setStyleSheet("QPlainTextEdit{border: none;}");
    QFont font;
    font.setPointSize(10);
    setFont(font);
    setReadOnly(true);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void CommitDetailTextEdit::setCommit(const Commit &commit, const QString &body)
{
    clear();
    appendHtml("<b>Hash:  </b>" + commit.hash);
    if (commit.parents.size() > 0) {
        QString parentStr = "<b>Parents:  </b>";
        for (const QString &p : commit.parents) {
            QString html = "<a href='%1'>%2</a>, ";
            parentStr.append(html.arg(p, p.first(8)));
        }
        appendHtml(parentStr.chopped(2));
    }
    appendHtml("<b>Author:  </b>" + commit.author + " &lt;" + commit.authorEmail + "&gt;");
    appendHtml("<b>Date:  </b>" + commit.authorDate);
    appendHtml("<b>Committer:  </b>" + commit.committer);
    if (commit.commitDate != commit.authorDate) {
        appendHtml("<b>Commit Date:  </b>" + commit.commitDate);
    }
    appendHtml("<br>" + QString(body).trimmed().replace("\n", "<br>"));

    moveCursor(QTextCursor::Start);
}

void CommitDetailTextEdit::mousePressEvent(QMouseEvent *event)
{
    QPlainTextEdit::mousePressEvent(event);
    m_clickedAnchor = (event->button() & Qt::LeftButton) ? anchorAt(event->pos()) : "";
}

void CommitDetailTextEdit::mouseReleaseEvent(QMouseEvent *event)
{
    QPlainTextEdit::mouseReleaseEvent(event);
    if (event->button() & Qt::LeftButton && !m_clickedAnchor.isEmpty() &&
        anchorAt(event->pos()) == m_clickedAnchor) {
        CommitDetailScrollArea *scrollArea =
            static_cast<CommitDetailScrollArea *>((parent()->parent()->parent()));
        emit scrollArea->linkClicked(m_clickedAnchor);
    }
}

void CommitDetailTextEdit::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    QTextBlock block = firstVisibleBlock();
    int height = qRound(
        blockBoundingGeometry(block).height() / block.lineCount() * (document()->lineCount() + 1));
    setFixedHeight(height);
}

BadgeLabel::BadgeLabel(QWidget *parent) : QLabel(parent), m_type(0)
{
    QFont font;
    font.setPointSize(10);
    setFont(font);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
    setTextInteractionFlags(Qt::TextSelectableByMouse);
    setContentsMargins(20, 2, 4, 2);
}

void BadgeLabel::setBadget(const QString &text, int type)
{
    m_type = type;
    setText(text);
}

void BadgeLabel::paintEvent(QPaintEvent *event)
{
    static int nailWidth = 16;
    static int cornerRadius = 2;
    static int iconPadding = 4;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QRectF badgeRect(rect().x() + 0.5, rect().top() + 0.5, width() - 1, rect().height() - 1);

    // Badge
    QPainterPath path;
    path.addRoundedRect(badgeRect, cornerRadius, cornerRadius);
    painter.setPen(QPen(QColor(0xAAAAAA), 1));
    painter.fillPath(path, QColor(Qt::white));
    painter.drawPath(path);

    // Text Body
    path.clear();
    path.setFillRule(Qt::WindingFill);
    QRectF bodyRect = badgeRect.adjusted(nailWidth, 0, 0, 0);
    path.addRoundedRect(bodyRect, cornerRadius, cornerRadius);
    path.addRect(bodyRect.adjusted(0, 0, -cornerRadius, 0));
    painter.fillPath(path.simplified(), QColor(0xF4F4F4));
    painter.drawPath(path.simplified());

    // Icon
    painter.setPen(QPen(QColor(0x2A2A2A), 1.4));
    QRectF iconRect =
        badgeRect.adjusted(iconPadding, iconPadding, -(width() - nailWidth + 1), -iconPadding);
    switch (m_type) {
        case 0:  // branch
        case 1:  // remote
            painter.drawLine(iconRect.center().x() - 2, iconRect.top(), iconRect.center().x() - 2,
                iconRect.bottom());
            painter.drawLine(iconRect.center().x() - 2, iconRect.center().y() + 1,
                iconRect.center().x() + 2, iconRect.center().y() - 2);
            break;
        case 2:  // tag
            painter.save();
            painter.translate(iconRect.center().x() - 1.5, iconRect.center().y() - 0.5);
            painter.rotate(-45);
            path.clear();
            path.moveTo(3.2, -iconRect.height() * 0.17);
            path.lineTo(3.2, iconRect.height() / 2);
            path.lineTo(-3.2, iconRect.height() / 2);
            path.lineTo(-3.2, -iconRect.height() * 0.17);
            path.lineTo(0, -iconRect.height() / 2);
            path.closeSubpath();
            painter.drawPath(path);
            painter.restore();
            break;
    }

    QLabel::paintEvent(event);
}

CommitDetailScrollArea::CommitDetailScrollArea(QWidget *parent) : QScrollArea(parent)
{
    viewport()->setStyleSheet("QWidget#detailScrollAreaWidget {background:White;}");
}

void CommitDetailScrollArea::setCommit(const Commit &commit, const QString &body)
{
    reset();

    CommitDetailTextEdit *detailTextEdit = new CommitDetailTextEdit(this);
    detailTextEdit->setCommit(commit, body);
    widget()->layout()->addWidget(detailTextEdit);

    if (!(commit.heads.isEmpty() && commit.remotes.isEmpty() && commit.tags.isEmpty())) {
        //        QFrame *line = new QFrame(this);
        //        line->setFrameShape(QFrame::HLine);
        //        line->setFrameShadow(QFrame::Sunken);
        //        widget()->layout()->addWidget(line);

        QFrame *refFrame = new QFrame(this);
        QVBoxLayout *refLayout = new QVBoxLayout(refFrame);
        refLayout->setContentsMargins(6, 0, 6, 6);

        for (const QString &head : commit.heads) {
            BadgeLabel *label = new BadgeLabel(this);
            label->setBadget(head, 0);
            refLayout->addWidget(label);
        }
        for (const QString &remote : commit.remotes) {
            BadgeLabel *label = new BadgeLabel(this);
            label->setBadget(remote, 1);
            refLayout->addWidget(label);
        }
        for (const QString &tag : commit.tags) {
            BadgeLabel *label = new BadgeLabel(this);
            label->setBadget(tag, 2);
            refLayout->addWidget(label);
        }

        refLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));

        widget()->layout()->addWidget(refFrame);
    }
}

void CommitDetailScrollArea::reset()
{
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
