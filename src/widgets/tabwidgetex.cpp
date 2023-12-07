#include "tabwidgetex.h"

#include <QDebug>
#include <QMouseEvent>

TabWidgetEx::TabWidgetEx(QWidget *parent) : QTabWidget(parent)
{
    setTabBar(new TabBarEx(this));
}

void TabWidgetEx::setTabData(int index, const QVariant &data)
{
    tabBar()->setTabData(index, data);
}

QVariant TabWidgetEx::tabData(int index) const
{
    return tabBar()->tabData(index);
}

TabBarEx::TabBarEx(QWidget *parent) : QTabBar(parent), m_hoverIndex(-1)
{
    m_addButton = new QToolButton(this);
    m_addButton->setIcon(QIcon("://resources/icon_add.svg"));
    m_addButton->setStyleSheet("QToolButton {border: none;} QToolButton:hover {background-color: "
                               "#DDD;} QToolButton:pressed {background-color: #CCC;}");
    connect(m_addButton, &QPushButton::clicked, this, [this] {
        TabWidgetEx *parent = static_cast<TabWidgetEx *>(parentWidget());
        emit parent->tabAddRequested();
    });
    connect(this, &QTabBar::currentChanged, this, &TabBarEx::onTabChanged);
}

QSize TabBarEx::sizeHint() const
{
    QSize size = QTabBar::sizeHint();
    return QSize(size.width() + 25, size.height());
}

void TabBarEx::resizeEvent(QResizeEvent *event)
{
    QTabBar::resizeEvent(event);
    layoutAddButton();
}

void TabBarEx::tabLayoutChange()
{
    QTabBar::tabLayoutChange();
    layoutAddButton();
}

void TabBarEx::layoutAddButton()
{
    if (height() <= 0) return;

    auto buttonSize = height() - 4;
    m_addButton->setFixedSize(buttonSize, buttonSize);

    auto total = count();
    int x;
    if (total <= 0) {
        x = 0;
    } else {
        auto lastRect = tabRect(total - 1);
        x = lastRect.x() + lastRect.width();
    }
    m_addButton->move(x, 2);
}

void TabBarEx::updateCloseButtons()
{
    auto current = currentIndex();
    if (current < 0) return;

    for (int i = 0; i < count(); ++i) {
        QWidget *closeBtn = tabButton(i, RightSide);
        // nullptr when receive initial currentChanged
        if (!closeBtn) continue;
        if (i == current || i == m_hoverIndex) {
            closeBtn->show();
        } else {
            closeBtn->hide();
        }
    }
}

void TabBarEx::onTabChanged(int index)
{
    updateCloseButtons();
};

void TabBarEx::showEvent(QShowEvent *event)
{
    QTabBar::showEvent(event);
    updateCloseButtons();
}

void TabBarEx::leaveEvent(QEvent *event)
{
    QTabBar::leaveEvent(event);
    m_hoverIndex = -1;
    updateCloseButtons();
}

void TabBarEx::mouseMoveEvent(QMouseEvent *event)
{
    QTabBar::mouseMoveEvent(event);
    m_hoverIndex = tabAt(event->pos());
    updateCloseButtons();
}
