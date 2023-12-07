#ifndef TABWIDGETEX_H
#define TABWIDGETEX_H

#include <QObject>
#include <QPushButton>
#include <QTabBar>
#include <QTabWidget>
#include <QToolButton>

class TabWidgetEx : public QTabWidget
{
    Q_OBJECT

public:
    explicit TabWidgetEx(QWidget *parent = nullptr);
    void setTabData(int index, const QVariant &data);
    QVariant tabData(int index) const;

signals:
    void tabAddRequested();
};

class TabBarEx : public QTabBar
{
    Q_OBJECT

public:
    explicit TabBarEx(QWidget *parent = nullptr);
    QSize sizeHint() const override;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void tabLayoutChange() override;

private slots:
    void onTabChanged(int index);

private:
    QToolButton *m_addButton;
    int m_hoverIndex;

    void layoutAddButton();
    void updateCloseButtons();
};

#endif  // TABWIDGETEX_H
