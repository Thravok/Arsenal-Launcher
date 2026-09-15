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
    t.window = QColor(0x0b, 0x0e, 0x14);
    t.windowText = QColor(0xe8, 0xea, 0xed);
    t.base = QColor(0x0f, 0x13, 0x1a);
    t.alternateBase = QColor(0x1c, 0x23, 0x30);
    t.elevated = QColor(0x16, 0x1b, 0x24);
    t.border = QColor(0x2a, 0x35, 0x48);
    t.mutedText = QColor(0x8b, 0x95, 0xa8);
    t.toolTipBase = QColor(0x16, 0x1b, 0x24);
    t.toolTipText = QColor(0xf0, 0xf2, 0xf5);
    t.text = QColor(0xe8, 0xea, 0xed);
    t.button = QColor(0x1c, 0x23, 0x30);
    t.buttonText = QColor(0xe8, 0xea, 0xed);
    t.brightText = QColor(0xff, 0x6b, 0x6b);
    t.link = QColor(0x5b, 0x8c, 0xff);
    t.accent = QColor(0xc4, 0x1e, 0x3a);
    t.accentText = QColor(0xff, 0xff, 0xff);
    t.highlight = QColor(0xc4, 0x1e, 0x3a);
    t.highlightedText = QColor(0xff, 0xff, 0xff);
    t.placeholderText = QColor(0x6b, 0x72, 0x80);
    t.danger = QColor(0xe0, 0x5a, 0x5a);
    t.fade = QColor(0x0b, 0x0e, 0x14);
    t.fadeAmount = 0.45;
    return t;
}

Tokens bright()
{
    Tokens t;
    t.window = QColor(0xf4, 0xf5, 0xf7);
    t.windowText = QColor(0x1a, 0x1d, 0x23);
    t.base = QColor(0xff, 0xff, 0xff);
    t.alternateBase = QColor(0xee, 0xf0, 0xf4);
    t.elevated = QColor(0xff, 0xff, 0xff);
    t.border = QColor(0xd0, 0xd5, 0xde);
    t.mutedText = QColor(0x5c, 0x64, 0x72);
    t.toolTipBase = QColor(0x2a, 0x2f, 0x38);
    t.toolTipText = QColor(0xf0, 0xf2, 0xf5);
    t.text = QColor(0x1a, 0x1d, 0x23);
    t.button = QColor(0xee, 0xf0, 0xf4);
    t.buttonText = QColor(0x1a, 0x1d, 0x23);
    t.brightText = QColor(0xc0, 0x3a, 0x3a);
    t.link = QColor(0x3d, 0x6b, 0xd9);
    t.accent = QColor(0xc4, 0x1e, 0x3a);
    t.accentText = QColor(0xff, 0xff, 0xff);
    t.highlight = QColor(0xc4, 0x1e, 0x3a);
    t.highlightedText = QColor(0xff, 0xff, 0xff);
    t.placeholderText = QColor(0x8a, 0x92, 0xa0);
    t.danger = QColor(0xc0, 0x3a, 0x3a);
    t.fade = QColor(0xf4, 0xf5, 0xf7);
    t.fadeAmount = 0.5;
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

QString substitute(const QString& stylesheetTemplate, const Tokens& t)
{
    QString out = stylesheetTemplate;
    out.replace(QStringLiteral("@window@"), hex(t.window));
    out.replace(QStringLiteral("@windowText@"), hex(t.windowText));
    out.replace(QStringLiteral("@base@"), hex(t.base));
    out.replace(QStringLiteral("@alternateBase@"), hex(t.alternateBase));
    out.replace(QStringLiteral("@elevated@"), hex(t.elevated));
    out.replace(QStringLiteral("@border@"), hex(t.border));
    out.replace(QStringLiteral("@mutedText@"), hex(t.mutedText));
    out.replace(QStringLiteral("@text@"), hex(t.text));
    out.replace(QStringLiteral("@button@"), hex(t.button));
    out.replace(QStringLiteral("@buttonText@"), hex(t.buttonText));
    out.replace(QStringLiteral("@link@"), hex(t.link));
    out.replace(QStringLiteral("@accent@"), hex(t.accent));
    out.replace(QStringLiteral("@accentText@"), hex(t.accentText));
    out.replace(QStringLiteral("@highlight@"), hex(t.highlight));
    out.replace(QStringLiteral("@highlightedText@"), hex(t.highlightedText));
    out.replace(QStringLiteral("@toolTipBase@"), hex(t.toolTipBase));
    out.replace(QStringLiteral("@toolTipText@"), hex(t.toolTipText));
    out.replace(QStringLiteral("@placeholderText@"), hex(t.placeholderText));
    out.replace(QStringLiteral("@danger@"), hex(t.danger));
    out.replace(QStringLiteral("@radius@"), QString::number(t.radius));
    out.replace(QStringLiteral("@spacing@"), QString::number(t.spacing));
    out.replace(QStringLiteral("@controlHeight@"), QString::number(t.controlHeight));
    out.replace(QStringLiteral("@sidebarRowHeight@"), QString::number(t.sidebarRowHeight));
    return out;
}

}  // namespace ThemeTokens
