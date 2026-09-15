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

#include "ThemeTokens.h"

#include <QString>

namespace ThemeStyle {

/** Load a QSS template from Qt resources and substitute design tokens. */
QString load(const QString& resourcePath, const ThemeTokens::Tokens& tokens);

/** Bundled Arsenal Launcher app stylesheet with the given tokens applied. */
QString appStyleSheet(const ThemeTokens::Tokens& tokens);

}  // namespace ThemeStyle
