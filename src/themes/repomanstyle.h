#ifndef REPOMANSTYLE_H
#define REPOMANSTYLE_H

#include <QObject>
#include <QProxyStyle>

class RepoManStyle : public QProxyStyle
{
    Q_OBJECT
public:
    RepoManStyle();
    ~RepoManStyle();

    QSize sizeFromContents(ContentsType ct, const QStyleOption *opt, const QSize &contentsSize,
        const QWidget *w) const override;
    int pixelMetric(
        PixelMetric metric, const QStyleOption *option, const QWidget *widget) const override;
};

#endif  // REPOMANSTYLE_H
