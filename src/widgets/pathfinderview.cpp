#include "pathfinderview.h"

#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollBar>
#include <QTimer>

PathFinderView::PathFinderView(QWidget *parent) : QScrollArea(parent)
{
    setFixedHeight(30);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setStyleSheet("PathFinderView {border: none;}");
}

PathFinderView::~PathFinderView()
{
}

void PathFinderView::setPath(const QString &path)
{
    m_path = path;
    updateUI();
}

void PathFinderView::setTitle(const QString &title)
{
    m_title = title;
    updateUI();
}

void PathFinderView::onPathSegmentClicked()
{
    int index = sender()->property("index").toInt();
    QString path;
    for (int i = 0; i <= index; ++i) {
        path.append("/").append(m_pathSegments.at(i));
    }
    setPath(path);
    emit pathClicked(path);
}

void PathFinderView::updateUI()
{
    clearPathViews();

    if (!m_title.isEmpty()) {
        addSegmentView(m_title, -1);
    }

    QString path = m_path;
    if (path.startsWith("/")) path.remove(0, 1);
    if (path.endsWith("/")) path.chop(1);

    m_pathSegments = path.split("/");
    for (int i = 0; i < m_pathSegments.size(); ++i) {
        addSegmentView(m_pathSegments.at(i), i);
        if (i != m_pathSegments.size() - 1) {
            addSeperatorView("/");
        }
    }
    QTimer::singleShot(0, this, [&]() {
        horizontalScrollBar()->setValue(horizontalScrollBar()->maximum());
    });
}

void PathFinderView::clearPathViews()
{
    if (widget()->layout() == nullptr) {
        auto layout = new QHBoxLayout(widget());
        layout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
    } else {
        QLayoutItem *child;
        while ((child = widget()->layout()->takeAt(0)) != nullptr) {
            delete child->widget();
            delete child;
        }
    }
}

void PathFinderView::addSegmentView(const QString &text, int index)
{
    auto segView = new QPushButton(this);
    segView->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    segView->setStyleSheet("QPushButton {padding-left: 8px; padding-right: 8px; border: none}"
                           "QPushButton:hover {background-color: #DDD;} "
                           "QPushButton:pressed {background-color: #CCC;}");
    segView->setText(text);
    segView->setProperty("index", index);
    connect(segView, &QPushButton::clicked, this, &PathFinderView::onPathSegmentClicked);
    widget()->layout()->addWidget(segView);
}

void PathFinderView::addSeperatorView(const QString &seperator)
{
    auto sepratorView = new QLabel(this);
    sepratorView->setText(seperator);
    widget()->layout()->addWidget(sepratorView);
}
