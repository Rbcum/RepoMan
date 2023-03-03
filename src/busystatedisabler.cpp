#include "busystatedisabler.h"

BusyStateDisabler::BusyStateDisabler(QWidget *parent) : QObject(parent), m_count(0), m_widget(parent)
{
}

void BusyStateDisabler::disableCounter(bool state)
{
    if (state) {
        m_count++;
    } else {
        m_count--;
    }
    if (m_count > 0) {
        m_widget->setEnabled(false);
    } else {
        m_widget->setEnabled(true);
    }
}
