// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QColor>
#include <QIcon>
#include <QString>

namespace ThemedSvgIcon {

/** Render an SVG resource tinted for the active Qt palette (Lucide / currentColor). */
QIcon fromResource(const QString& resourcePath, const QColor& color, int size = 24);

/** Resolve a freedesktop-style icon name from bundled Qt icon themes (:/icons/…). */
QIcon fromBundledThemes(const QString& name, const QString& activeTheme, const QColor& color, int size = 24);

}  // namespace ThemedSvgIcon
