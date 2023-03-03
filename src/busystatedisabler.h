#ifndef BUSYSTATEENABLER_H
#define BUSYSTATEENABLER_H

#include <QObject>
#include <QWidget>

class BusyStateDisabler : public QObject
{
    Q_OBJECT
public:
    explicit BusyStateDisabler(QWidget *parent = nullptr);

public slots:
    void disableCounter(bool state);

private:
    int m_count;
    QWidget *m_widget;
};

#endif  // BUSYSTATEENABLER_H
