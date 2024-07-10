// Copyright (C) 2016 Thorben Kroeger <thorbenkroeger@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QBrush>  // QGradientStops
#include <QObject>

QT_BEGIN_NAMESPACE
class QMenu;
class QPalette;
class QSettings;
QT_END_NAMESPACE

namespace utils {

    class ThemePrivate;

    class Theme : public QObject
    {
        Q_OBJECT
    public:
        Theme(const QString &id, QObject *parent = nullptr);
        ~Theme() override;

        enum Color
        {
            /* Palette for QPalette */

            PaletteWindow,
            PaletteWindowText,
            PaletteBase,
            PaletteAlternateBase,
            PaletteToolTipBase,
            PaletteToolTipText,
            PaletteText,
            PaletteButton,
            PaletteButtonText,
            PaletteBrightText,
            PaletteHighlight,
            PaletteHighlightedText,
            PaletteLink,
            PaletteLinkVisited,

            PaletteLight,
            PaletteMidlight,
            PaletteDark,
            PaletteMid,
            PaletteShadow,

            PaletteWindowDisabled,
            PaletteWindowTextDisabled,
            PaletteBaseDisabled,
            PaletteAlternateBaseDisabled,
            PaletteToolTipBaseDisabled,
            PaletteToolTipTextDisabled,
            PaletteTextDisabled,
            PaletteButtonDisabled,
            PaletteButtonTextDisabled,
            PaletteBrightTextDisabled,
            PaletteHighlightDisabled,
            PaletteHighlightedTextDisabled,
            PaletteLinkDisabled,
            PaletteLinkVisitedDisabled,

            PaletteLightDisabled,
            PaletteMidlightDisabled,
            PaletteDarkDisabled,
            PaletteMidDisabled,
            PaletteShadowDisabled,

            PalettePlaceholderText,
            PalettePlaceholderTextDisabled,

            /* Icons */

            IconsBaseColor,
            IconsDisabledColor,

            /* Badge */
            BadgeBorderColor,
            BadgeBackgoundColor,

            PanelItemPressed,
            PanelItemHovered,
            LoadingBarBackground,
            DiffLineAdd,
            DiffLineRemove,
            DiffLineDummy,
            DiffLineMeta,
            LineNumber,
            LineNumberBackground
        };

        enum ImageFile
        {
            IconOverlayCSource,
            IconOverlayCppHeader,
            IconOverlayCppSource,
            IconOverlayPri,
            IconOverlayPrf,
            IconOverlayPro,
            StandardPixmapFileIcon,
            StandardPixmapDirIcon
        };

        enum Flag
        {
            DarkUserInterface,
            DerivePaletteFromTheme,
            DerivePaletteFromThemeIfNeeded,
            WindowColorAsBase,
            ToolBarIconShadow
        };

        Q_ENUM(Color)
        Q_ENUM(ImageFile)
        Q_ENUM(Flag)

        Q_INVOKABLE bool flag(utils::Theme::Flag f) const;
        Q_INVOKABLE QColor color(utils::Theme::Color role) const;
        QString imageFile(ImageFile imageFile, const QString &fallBack) const;
        QPalette palette() const;
        QStringList preferredStyles() const;
        QString defaultTextEditorColorScheme() const;

        QString id() const;
        QString filePath() const;
        QString displayName() const;
        void setDisplayName(const QString &displayName);

        void readSettings(QSettings &settings);

        static bool systemUsesDarkMode();
        static QPalette initialPalette();

        static void setInitialPalette(Theme *initTheme);

        static void setHelpMenu(QMenu *menu);

    protected:
        Theme(Theme *originTheme, QObject *parent = nullptr);
        ThemePrivate *d;

    private:
        void readSettingsInternal(QSettings &settings);
        friend Theme *creatorTheme();
        friend Theme *proxyTheme();
        QColor readNamedColorNoWarning(const QString &color) const;
        QPair<QColor, QString> readNamedColor(const QString &color) const;
    };

    Theme *creatorTheme();
    Theme *proxyTheme();
    QColor borderColor();

}  // namespace utils
