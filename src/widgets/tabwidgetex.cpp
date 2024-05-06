#include "tabwidgetex.h"

#include <qstylefactory.h>

#include <QApplication>
#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOption>

#include "themes/icon.h"
#include "themes/theme.h"

using namespace utils;

class TabBarStyle : public QProxyStyle
{
public:
    void drawPrimitive(
        PrimitiveElement pe, const QStyleOption *opt, QPainter *p, const QWidget *w) const override
    {
        if (pe == QStyle::PE_PanelButtonCommand || pe == QStyle::PE_PanelButtonTool) {  // button bg
            if (opt->state & State_MouseOver) {
                p->save();
                bool isDown = (opt->state & State_Sunken) || (opt->state & State_On);
                p->fillRect(opt->rect, creatorTheme()->color(isDown ? Theme::PanelItemPressed
                                                                    : Theme::PanelItemHovered));
                p->restore();
            }
            return;
        }
        QProxyStyle::drawPrimitive(pe, opt, p, w);
    }

    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *option,
        const QWidget *widget) const override
    {
        if (standardIcon == QStyle::SP_DialogCloseButton) {
            return Icon({{":/icons/close.png", Theme::IconsBaseColor}}).icon();
        }
        return QProxyStyle::standardIcon(standardIcon, option, widget);
    }
};

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
    setStyle(new TabBarStyle());
    setMouseTracking(true);

    m_addButton = new QToolButton(this);
    m_addButton->setIcon(Icon({{":/icons/add.png", Theme::IconsBaseColor}}).icon());

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

QSize TabBarEx::tabSizeHint(int index) const
{
    auto size = QTabBar::tabSizeHint(index);
    size.setWidth(qMax(size.width(), 150));
    return size;
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
