// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Arsenal Launcher
 *  Copyright (C) 2026 Arsenal Launcher Contributors
 *
 *  This program is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation, version 3.
 */

#include "ThemeStyle.h"

#include <QFile>
#include <QDebug>

namespace ThemeStyle {

QString load(const QString& resourcePath, const ThemeTokens::Tokens& tokens)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to load theme stylesheet from" << resourcePath;
        return QString();
    }
    const QString raw = QString::fromUtf8(file.readAll());
    return ThemeTokens::substitute(raw, tokens);
}

QString appStyleSheet(const ThemeTokens::Tokens& tokens)
{
    return load(QStringLiteral(":/themes/arsenal.qss"), tokens);
}

}  // namespace ThemeStyle
