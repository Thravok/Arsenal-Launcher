// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Arsenal Launcher
 *  Copyright (C) 2026 Arsenal Launcher Contributors
 *
 *  This program is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation, version 3.
 */

#pragma once

#include <QColor>
#include <QPalette>
#include <QString>

/**
 * Shared design tokens for Arsenal Launcher widget themes.
 * Qt stylesheets have no CSS variables; placeholders like @accent@ are
 * substituted when building the final stylesheet string.
 *
 * Opaque colors use @name@ → #rrggbb. Colors with alpha use @nameRgba@ → rgba(...).
 */
namespace ThemeTokens {

struct Tokens {
    QColor window;
    QColor windowText;
    QColor base;
    QColor alternateBase;
    QColor elevated;
    QColor border;
    QColor mutedText;
    QColor toolTipBase;
    QColor toolTipText;
    QColor text;
    QColor button;
    QColor buttonText;
    QColor brightText;
    QColor link;
    QColor accent;
    QColor accentText;
    QColor highlight;
    QColor highlightedText;
    QColor placeholderText;
    QColor danger;
    QColor dangerHover;
    QColor success;
    QColor successHover;
    QColor fade;
    double fadeAmount = 0.5;

    /** Recessed well behind cards / page content. */
    QColor glassWell;
    /** Mid glass fill for inputs and inset controls. */
    QColor glassFill;
    /** Elevated glass (sidebars, menus, rails, chrome). */
    QColor glassElevated;
    /** Visible glass edge so regions don't melt into each other. */
    QColor glassBorder;
    /** Specular / top-edge highlight for glass surfaces. */
    QColor glassHighlight;

    int radius = 14;
    int spacing = 8;
    int controlHeight = 36;
    int sidebarRowHeight = 40;
};

Tokens dark();
Tokens bright();

QPalette toPalette(const Tokens& t);
QString substitute(const QString& stylesheetTemplate, const Tokens& t);

}  // namespace ThemeTokens
