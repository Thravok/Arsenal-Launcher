// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Arsenal Launcher
 *  Copyright (C) 2026 Arsenal Launcher Contributors
 */

#pragma once

#include "BaseInstance.h"
#include "minecraft/auth/MinecraftAccount.h"

namespace LaunchAccountUtils {

MinecraftAccountPtr accountForLaunch(BaseInstance* instance);

QString accountKindLabel(const MinecraftAccountPtr& account);
QString accountStatusLabel(const MinecraftAccountPtr& account);
QString accountMetaLine(const MinecraftAccountPtr& account);

bool blocksLaunch(const MinecraftAccountPtr& account);
QString launchBlockReason(const MinecraftAccountPtr& account);

}  // namespace LaunchAccountUtils
