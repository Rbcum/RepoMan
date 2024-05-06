// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "icon.h"

#include <QApplication>
#include <QDebug>
#include <QIcon>
#include <QImage>
#include <QPainter>
#include <QWidget>

#include "theme.h"

namespace utils {

    static const qreal PunchEdgeWidth = 0.5;
    static const qreal PunchEdgeIntensity = 0.6;

    static QPixmap maskToColorAndAlpha(const QPixmap &mask, const QColor &color)
    {
        QImage result(mask.toImage().convertToFormat(QImage::Format_ARGB32));
        result.setDevicePixelRatio(mask.devicePixelRatio());
        auto bitsStart = reinterpret_cast<QRgb *>(result.bits());
        const QRgb *bitsEnd = bitsStart + result.width() * result.height();
        const QRgb tint = color.rgb() & 0x00ffffff;
        const auto alpha = QRgb(color.alpha());
        for (QRgb *pixel = bitsStart; pixel < bitsEnd; ++pixel) {
            QRgb pixelAlpha = (((~*pixel) & 0xff) * alpha) >> 8;
            *pixel = (pixelAlpha << 24) | tint;
        }
        return QPixmap::fromImage(result);
    }

    using MaskAndColor = QPair<QPixmap, QColor>;
    using MasksAndColors = QList<MaskAndColor>;
    static MasksAndColors masksAndColors(const QList<IconMaskAndColor> &icon, int dpr)
    {
        MasksAndColors result;
        for (const IconMaskAndColor &i : icon) {
            const QString &fileName = i.first;
            const QColor color = creatorTheme()->color(i.second);
            QPixmap pixmap;
            if (!pixmap.load(fileName)) {
                pixmap = QPixmap(1, 1);
                qWarning() << "Could not load image: " << fileName;
            }
            result.append({pixmap, color});
        }
        return result;
    }

    static void smearPixmap(QPainter *painter, const QPixmap &pixmap, qreal radius)
    {
        const qreal nagative = -radius - 0.01;  // Workaround for QPainter rounding behavior
        const qreal positive = radius;
        painter->drawPixmap(QPointF(nagative, nagative), pixmap);
        painter->drawPixmap(QPointF(0, nagative), pixmap);
        painter->drawPixmap(QPointF(positive, nagative), pixmap);
        painter->drawPixmap(QPointF(positive, 0), pixmap);
        painter->drawPixmap(QPointF(positive, positive), pixmap);
        painter->drawPixmap(QPointF(0, positive), pixmap);
        painter->drawPixmap(QPointF(nagative, positive), pixmap);
        painter->drawPixmap(QPointF(nagative, 0), pixmap);
    }

    static QPixmap combinedMask(const MasksAndColors &masks, Icon::IconStyleOptions style)
    {
        if (masks.count() == 1) return masks.first().first;

        QPixmap result(masks.first().first);
        QPainter p(&result);
        p.setCompositionMode(QPainter::CompositionMode_Darken);
        auto maskImage = masks.constBegin();
        maskImage++;
        for (; maskImage != masks.constEnd(); ++maskImage) {
            if (style & Icon::PunchEdges) {
                p.save();
                p.setOpacity(PunchEdgeIntensity);
                p.setCompositionMode(QPainter::CompositionMode_Lighten);
                smearPixmap(&p, maskToColorAndAlpha((*maskImage).first, Qt::white), PunchEdgeWidth);
                p.restore();
            }
            p.drawPixmap(0, 0, (*maskImage).first);
        }
        p.end();
        return result;
    }

    static QPixmap masksToIcon(
        const MasksAndColors &masks, const QPixmap &combinedMask, Icon::IconStyleOptions style)
    {
        QPixmap result(combinedMask.size());
        result.setDevicePixelRatio(combinedMask.devicePixelRatio());
        result.fill(Qt::transparent);
        QPainter p(&result);

        for (MasksAndColors::const_iterator maskImage = masks.constBegin();
             maskImage != masks.constEnd(); ++maskImage) {
            if (style & Icon::PunchEdges && maskImage != masks.constBegin()) {
                // Punch a transparent outline around an overlay.
                p.save();
                p.setOpacity(PunchEdgeIntensity);
                p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
                smearPixmap(&p, maskToColorAndAlpha((*maskImage).first, Qt::white), PunchEdgeWidth);
                p.restore();
            }
            p.drawPixmap(0, 0, maskToColorAndAlpha((*maskImage).first, (*maskImage).second));
        }

        if (style & Icon::DropShadow && creatorTheme()->flag(Theme::ToolBarIconShadow)) {
            const QPixmap shadowMask = maskToColorAndAlpha(combinedMask, Qt::black);
            p.setCompositionMode(QPainter::CompositionMode_DestinationOver);
            p.setOpacity(0.08);
            p.drawPixmap(QPointF(0, -0.501), shadowMask);
            p.drawPixmap(QPointF(-0.501, 0), shadowMask);
            p.drawPixmap(QPointF(0.5, 0), shadowMask);
            p.drawPixmap(QPointF(0.5, 0.5), shadowMask);
            p.drawPixmap(QPointF(-0.501, 0.5), shadowMask);
            p.setOpacity(0.3);
            p.drawPixmap(0, 1, shadowMask);
        }

        p.end();

        return result;
    }

    Icon::Icon() = default;

    Icon::Icon(const QList<IconMaskAndColor> &args, Icon::IconStyleOptions style)
        : m_iconSourceList(args), m_style(style)
    {
    }

    // Icon::Icon(const QString &imageFileName) : m_iconSourceList({{imageFileName,
    // Theme::Color(-1)}})
    // {
    // }

    QIcon Icon::icon() const
    {
        if (m_iconSourceList.isEmpty()) return QIcon();

        if (m_style == None) return QIcon(m_iconSourceList.constFirst().first);

        const int maxDpr = qRound(qApp->devicePixelRatio());
        if (maxDpr == m_lastDevicePixelRatio) return m_lastIcon;

        m_lastDevicePixelRatio = maxDpr;
        m_lastIcon = QIcon();
        for (int dpr = 1; dpr <= maxDpr; dpr++) {
            const MasksAndColors masks = masksAndColors(m_iconSourceList, dpr);
            const QPixmap combinedMask = utils::combinedMask(masks, m_style);
            m_lastIcon.addPixmap(masksToIcon(masks, combinedMask, m_style));

            const QColor disabledColor = creatorTheme()->color(Theme::IconsDisabledColor);
            m_lastIcon.addPixmap(maskToColorAndAlpha(combinedMask, disabledColor), QIcon::Disabled);
        }
        return m_lastIcon;
    }

    QPixmap Icon::pixmap(QIcon::Mode iconMode) const
    {
        if (m_iconSourceList.isEmpty()) {
            return QPixmap();
        } else if (m_style == None) {
            return QPixmap(m_iconSourceList.constFirst().first);
        } else {
            const MasksAndColors masks =
                masksAndColors(m_iconSourceList, qRound(qApp->devicePixelRatio()));
            const QPixmap combinedMask = utils::combinedMask(masks, m_style);
            return iconMode == QIcon::Disabled
                       ? maskToColorAndAlpha(
                             combinedMask, creatorTheme()->color(Theme::IconsDisabledColor))
                       : masksToIcon(masks, combinedMask, m_style);
        }
    }

    QString Icon::imageFilePath() const
    {
        return m_iconSourceList.first().first;
    }

    QIcon Icon::combinedIcon(const QList<QIcon> &icons)
    {
        QIcon result;
        QWindow *window = QApplication::allWidgets().constFirst()->windowHandle();
        for (const QIcon &icon : icons)
            for (const QIcon::Mode mode : {QIcon::Disabled, QIcon::Normal})
                for (const QSize &size : icon.availableSizes(mode))
                    result.addPixmap(icon.pixmap(window, size, mode), mode);
        return result;
    }

    QIcon Icon::combinedIcon(const QList<Icon> &icons)
    {
        QList<QIcon> qIcons;
        for (const Icon &ic : icons) {
            qIcons.push_back(ic.icon());
        }
        return combinedIcon(qIcons);
    }

    QIcon Icon::fromTheme(const QString &name)
    {
        static QHash<QString, QIcon> cache;

        auto found = cache.find(name);
        if (found != cache.end()) return *found;

        QIcon icon = QIcon::fromTheme(name);
        cache.insert(name, icon);

        return cache[name];
    }

}  // namespace utils
