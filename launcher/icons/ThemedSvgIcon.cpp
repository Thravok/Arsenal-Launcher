// SPDX-License-Identifier: GPL-3.0-only
#include "ThemedSvgIcon.h"

#include <QFile>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>

namespace ThemedSvgIcon {

QIcon fromResource(const QString& resourcePath, const QColor& color, int size);

namespace {

QStringList themeCandidates(const QString& activeTheme)
{
    QStringList list;
    if (!activeTheme.isEmpty())
        list << activeTheme;
    if (activeTheme != QStringLiteral("lucide"))
        list << QStringLiteral("lucide");
    list << QStringLiteral("flat") << QStringLiteral("pe_colored") << QStringLiteral("multimc");
    list.removeDuplicates();
    return list;
}

QIcon loadThemedSvg(const QString& path, const QColor& color, int size)
{
    if (!QFile::exists(path))
        return {};
    return fromResource(path, color, size);
}

QIcon loadFromThemeDir(const QString& theme, const QString& name, const QColor& color, int size)
{
    const QString svgPath = QStringLiteral(":/icons/%1/scalable/%2.svg").arg(theme, name);
    if (auto icon = loadThemedSvg(svgPath, color, size); !icon.isNull())
        return icon;

    const QString instanceSvg = QStringLiteral(":/icons/%1/scalable/instances/%2.svg").arg(theme, name);
    if (auto icon = loadThemedSvg(instanceSvg, color, size); !icon.isNull())
        return icon;

    for (int px : {32, 24, 48, 16}) {
        const QString pngPath = QStringLiteral(":/icons/%1/%2x%2/%3.png").arg(theme).arg(px).arg(name);
        if (QFile::exists(pngPath))
            return QIcon(pngPath);
    }
    return {};
}

}  // namespace

QIcon fromResource(const QString& resourcePath, const QColor& color, int size)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly))
        return {};

    QByteArray data = file.readAll();
    const QByteArray colorBytes = color.name(QColor::HexRgb).toUtf8();
    data.replace("currentColor", colorBytes);

    QSvgRenderer renderer(data);
    if (!renderer.isValid())
        return {};

    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    renderer.render(&painter);
    painter.end();

    QIcon icon;
    icon.addPixmap(pixmap);
    return icon;
}

QIcon fromBundledThemes(const QString& name, const QString& activeTheme, const QColor& color, int size)
{
    // Prefer Lucide for Arsenal chrome even when a legacy theme is selected but missing an icon.
    for (const QString& theme : themeCandidates(activeTheme.isEmpty() ? QStringLiteral("lucide") : activeTheme)) {
        const QIcon icon = loadFromThemeDir(theme, name, color, size);
        if (!icon.isNull())
            return icon;
    }
    return {};
}

}  // namespace ThemedSvgIcon
