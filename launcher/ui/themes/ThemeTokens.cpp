// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Arsenal Launcher
 *  Copyright (C) 2026 Arsenal Launcher Contributors
 *
 *  This program is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation, version 3.
 */

#include "ThemeTokens.h"

#include "ITheme.h"

namespace ThemeTokens {

Tokens dark()
{
    Tokens t;
    t.window = QColor(0x0a, 0x0b, 0x0e, 255);
    t.windowText = QColor(0xee, 0xf1, 0xf6);
    t.base = QColor(0x12, 0x14, 0x1a);
    t.alternateBase = QColor(0x1a, 0x1d, 0x26, 255);
    t.elevated = QColor(0x14, 0x16, 0x1c);
    t.border = QColor(0x2a, 0x2f, 0x3a);
    t.mutedText = QColor(0x8b, 0x93, 0xa7);
    t.toolTipBase = QColor(0x16, 0x1b, 0x24, 240);
    t.toolTipText = QColor(0xf0, 0xf2, 0xf5);
    t.text = QColor(0xe8, 0xea, 0xed);
    t.button = QColor(0x18, 0x1b, 0x22, 255);
    t.buttonText = QColor(0xee, 0xf1, 0xf6);
    t.brightText = QColor(0xff, 0x6b, 0x6b);
    t.link = QColor(0x5b, 0x8c, 0xff);
    t.accent = QColor(0xc4, 0x1e, 0x3a);
    t.accentText = QColor(0xff, 0xff, 0xff);
    t.highlight = QColor(0xc4, 0x1e, 0x3a);
    t.highlightedText = QColor(0xff, 0xff, 0xff);
    t.placeholderText = QColor(0x6b, 0x72, 0x80);
    t.danger = QColor(0xe0, 0x5a, 0x5a);
    t.dangerHover = QColor(0xec, 0x6e, 0x6e);
    t.success = QColor(0x2f, 0xbf, 0x71);
    t.successHover = QColor(0x39, 0xc9, 0x7a);
    t.fade = QColor(0x0a, 0x0b, 0x0e);
    t.fadeAmount = 0.45;

    // Flat dark slabs (Lunar-like) — opaque, low-contrast edges
    t.glassWell = QColor(0x0c, 0x0e, 0x12, 255);
    t.glassFill = QColor(0x16, 0x19, 0x20, 255);
    t.glassElevated = QColor(0x14, 0x16, 0x1c, 255);
    t.glassBorder = QColor(0x2a, 0x2f, 0x3a, 255);
    t.glassHighlight = QColor(0x2a, 0x2f, 0x3a, 180);

    t.radius = 12;
    return t;
}

Tokens bright()
{
    Tokens t;
    t.window = QColor(0xdc, 0xe0, 0xe8, 245);
    t.windowText = QColor(0x16, 0x1a, 0x22);
    t.base = QColor(0xff, 0xff, 0xff);
    t.alternateBase = QColor(0xe8, 0xec, 0xf2, 230);
    t.elevated = QColor(0xff, 0xff, 0xff);
    t.border = QColor(0xb4, 0xbc, 0xc8);
    t.mutedText = QColor(0x4e, 0x57, 0x66);
    t.toolTipBase = QColor(0x2a, 0x2f, 0x38, 235);
    t.toolTipText = QColor(0xf0, 0xf2, 0xf5);
    t.text = QColor(0x1a, 0x1d, 0x23);
    t.button = QColor(0xff, 0xff, 0xff, 245);
    t.buttonText = QColor(0x16, 0x1a, 0x22);
    t.brightText = QColor(0xc0, 0x3a, 0x3a);
    t.link = QColor(0x3d, 0x6b, 0xd9);
    t.accent = QColor(0xc4, 0x1e, 0x3a);
    t.accentText = QColor(0xff, 0xff, 0xff);
    t.highlight = QColor(0xc4, 0x1e, 0x3a);
    t.highlightedText = QColor(0xff, 0xff, 0xff);
    t.placeholderText = QColor(0x8a, 0x92, 0xa0);
    t.danger = QColor(0xc0, 0x3a, 0x3a);
    t.dangerHover = QColor(0xd4, 0x4a, 0x4a);
    t.success = QColor(0x28, 0xa7, 0x5e);
    t.successHover = QColor(0x32, 0xc0, 0x70);
    t.fade = QColor(0xf4, 0xf5, 0xf7);
    t.fadeAmount = 0.5;

    t.glassWell = QColor(0xd0, 0xd5, 0xde, 240);
    t.glassFill = QColor(0xf4, 0xf6, 0xfa, 240);
    t.glassElevated = QColor(255, 255, 255, 250);
    t.glassBorder = QColor(0, 0, 0, 48);
    t.glassHighlight = QColor(255, 255, 255, 200);

    t.radius = 14;
    return t;
}

QPalette toPalette(const Tokens& t)
{
    QPalette p;
    p.setColor(QPalette::Window, t.window);
    p.setColor(QPalette::WindowText, t.windowText);
    p.setColor(QPalette::Base, t.base);
    p.setColor(QPalette::AlternateBase, t.alternateBase);
    p.setColor(QPalette::ToolTipBase, t.toolTipBase);
    p.setColor(QPalette::ToolTipText, t.toolTipText);
    p.setColor(QPalette::Text, t.text);
    p.setColor(QPalette::Button, t.button);
    p.setColor(QPalette::ButtonText, t.buttonText);
    p.setColor(QPalette::BrightText, t.brightText);
    p.setColor(QPalette::Link, t.link);
    p.setColor(QPalette::Highlight, t.highlight);
    p.setColor(QPalette::HighlightedText, t.highlightedText);
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    p.setColor(QPalette::PlaceholderText, t.placeholderText);
#endif
    return ITheme::fadeInactive(p, t.fadeAmount, t.fade);
}

static QString hex(const QColor& c)
{
    return c.name(QColor::HexRgb);
}

static QString rgba(const QColor& c)
{
    return QStringLiteral("rgba(%1, %2, %3, %4)")
        .arg(c.red())
        .arg(c.green())
        .arg(c.blue())
        .arg(c.alpha());
}

static void replaceColor(QString& out, const QString& name, const QColor& c)
{
    out.replace(QStringLiteral("@%1@").arg(name), hex(c));
    out.replace(QStringLiteral("@%1Rgba@").arg(name), rgba(c));
}

QString substitute(const QString& stylesheetTemplate, const Tokens& t)
{
    QString out = stylesheetTemplate;
    replaceColor(out, QStringLiteral("window"), t.window);
    replaceColor(out, QStringLiteral("windowText"), t.windowText);
    replaceColor(out, QStringLiteral("base"), t.base);
    replaceColor(out, QStringLiteral("alternateBase"), t.alternateBase);
    replaceColor(out, QStringLiteral("elevated"), t.elevated);
    replaceColor(out, QStringLiteral("border"), t.border);
    replaceColor(out, QStringLiteral("mutedText"), t.mutedText);
    replaceColor(out, QStringLiteral("text"), t.text);
    replaceColor(out, QStringLiteral("button"), t.button);
    replaceColor(out, QStringLiteral("buttonText"), t.buttonText);
    replaceColor(out, QStringLiteral("link"), t.link);
    replaceColor(out, QStringLiteral("accent"), t.accent);
    replaceColor(out, QStringLiteral("accentText"), t.accentText);
    replaceColor(out, QStringLiteral("highlight"), t.highlight);
    replaceColor(out, QStringLiteral("highlightedText"), t.highlightedText);
    replaceColor(out, QStringLiteral("toolTipBase"), t.toolTipBase);
    replaceColor(out, QStringLiteral("toolTipText"), t.toolTipText);
    replaceColor(out, QStringLiteral("placeholderText"), t.placeholderText);
    replaceColor(out, QStringLiteral("danger"), t.danger);
    replaceColor(out, QStringLiteral("dangerHover"), t.dangerHover);
    replaceColor(out, QStringLiteral("success"), t.success);
    replaceColor(out, QStringLiteral("successHover"), t.successHover);
    replaceColor(out, QStringLiteral("glassWell"), t.glassWell);
    replaceColor(out, QStringLiteral("glassFill"), t.glassFill);
    replaceColor(out, QStringLiteral("glassElevated"), t.glassElevated);
    replaceColor(out, QStringLiteral("glassBorder"), t.glassBorder);
    replaceColor(out, QStringLiteral("glassHighlight"), t.glassHighlight);
    out.replace(QStringLiteral("@radius@"), QString::number(t.radius));
    out.replace(QStringLiteral("@spacing@"), QString::number(t.spacing));
    out.replace(QStringLiteral("@controlHeight@"), QString::number(t.controlHeight));
    out.replace(QStringLiteral("@sidebarRowHeight@"), QString::number(t.sidebarRowHeight));
    return out;
}

}  // namespace ThemeTokens
