#include "repomanstyle.h"

RepoManStyle::RepoManStyle()
{
}

RepoManStyle::~RepoManStyle()
{
}
QSize RepoManStyle::sizeFromContents(
    ContentsType ct, const QStyleOption *opt, const QSize &contentsSize, const QWidget *w) const
{
    QSize newSize = QProxyStyle::sizeFromContents(ct, opt, contentsSize, w);

    switch (ct) {
        case CT_Splitter:
            if (w) newSize = QSize(1, 1);
            break;
        default:
            break;
    }
    return newSize;
}

int RepoManStyle::pixelMetric(
    PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    int retval = QProxyStyle::pixelMetric(metric, option, widget);
    switch (metric) {
        case PM_SplitterWidth:
            if (widget) retval = 1;
            break;
        default:
            break;
    }
    return retval;
}
