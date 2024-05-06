// Copyright (C) 2016 Thorben Kroeger <thorbenkroeger@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QColor>
#include <QMap>

#include "theme.h"

namespace utils {

    class ThemePrivate
    {
    public:
        ThemePrivate();

        QString id;
        QString fileName;
        QString displayName;
        QStringList preferredStyles;
        QString defaultTextEditorColorScheme;
        QList<QPair<QColor, QString>> colors;
        QList<QString> imageFiles;
        QList<bool> flags;
        QMap<QString, QColor> palette;
        QMap<QString, QString> unresolvedPalette;
        QMap<QString, QString> unresolvedColors;
    };

    void setCreatorTheme(Theme *theme);
    void setThemeApplicationPalette();

}  // namespace utils
