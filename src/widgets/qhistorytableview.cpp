#include "qhistorytableview.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QProxyStyle>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollBar>

const QStringList QHistoryTableView::HEADERS = {"Graph", "Subject", "Date", "Author", "Hash"};

QHistoryTableView::QHistoryTableView(QWidget *parent)
    : QTableView(parent), m_widthSet(false), m_loading(0)
{
    viewport()->installEventFilter(this);

    QFontMetrics fm(font());
    m_colWidths << fm.horizontalAdvance("xxxxxxxxxxxxxxxx");
    m_colWidths << 0;
    m_colWidths << fm.horizontalAdvance("2022-00-00 00:00   ");
    m_colWidths << fm.horizontalAdvance("xxxxxxxxxxxxxxxx");
    m_colWidths << fm.horizontalAdvance("a6827f1013");

    m_loadingFrame = new QFrame(this);
    m_loadingFrame->hide();
    m_loadingFrame->setStyleSheet("* {font-size: 14px;} QFrame {background-color: #B2BABC;}");
    m_loadingLabel = new QLabel(m_loadingFrame);
    m_cancelButton = new QPushButton("Cancel", m_loadingFrame);
    connect(m_cancelButton, &QPushButton::clicked, this, [this]() {
        emit cancelLoading();
    });

    QHBoxLayout *layout = new QHBoxLayout(m_loadingFrame);
    layout->setContentsMargins(10, 2, 10, 2);
    layout->setSpacing(20);
    layout->addWidget(m_loadingLabel);
    layout->addWidget(m_cancelButton);
    layout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum));
}

bool QHistoryTableView::eventFilter(QObject *watched, QEvent *event)
{
    if (event->isInputEvent()) {
        return m_loading;
    }
    return false;
}

bool QHistoryTableView::event(QEvent *event)
{
    if (m_loading && event->isInputEvent()) {
        return true;
    }
    if (event->type() == QEvent::Resize) {
        if (!m_widthSet) {
            m_widthSet = true;
            int width = static_cast<QResizeEvent *>(event)->size().width() - 16;
            m_colWidths[1] = qMax(
                200, width - m_colWidths[0] - m_colWidths[2] - m_colWidths[3] - m_colWidths[4]);
            for (int i = 0; i < m_colWidths.size(); ++i) {
                setColumnWidth(i, m_colWidths[i]);
            }
        }

        updateGeometries();
        m_loadingFrame->setGeometry(0, 0, width(), 28);
    }
    return QTableView::event(event);
}

void QHistoryTableView::setLoading(bool loading)
{
    loading ? m_loading++ : m_loading--;
    if (m_loading) {
        m_loadingFrame->show();
    } else {
        m_loadingFrame->hide();
    }
}

void QHistoryTableView::updateLoadingLabel(const HistorySelectionArg &arg, int count)
{
    QString ref;
    switch (arg.type) {
        case HistorySelectionArg::Hash:
            ref = arg.data.first(7);
            break;
        case HistorySelectionArg::Tag:
        case HistorySelectionArg::Remote:
        case HistorySelectionArg::Head:
            ref = arg.data;
            break;
        default:
            Q_UNREACHABLE();
    }
    m_loadingLabel->setText(
        QString("Searching for %1 (%2 commits)...").arg(ref, QString::number(count)));
    if (count) {
        m_loadingFrame->show();
    } else {
        m_loadingFrame->hide();
    }
}
