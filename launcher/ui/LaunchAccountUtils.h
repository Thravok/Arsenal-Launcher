// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Arsenal Launcher
 *  Copyright (C) 2026 Arsenal Launcher Contributors
 */

#pragma once

#include "BaseInstance.h"
#include "LaunchMode.h"
#include "minecraft/auth/MinecraftAccount.h"

namespace LaunchAccountUtils {

/** Pick the account for launch without silently substituting the global default
 *  when a pinned instance account is missing. */
MinecraftAccountPtr resolveAccountForLaunch(bool useInstanceAccount,
                                            const MinecraftAccountPtr& instanceAccount,
                                            const MinecraftAccountPtr& defaultAccount);

/** Offline accounts carry a dummy token and must never start in online mode.
 *  canPlayFullGame is true when any stored account owns Minecraft. */
LaunchMode launchModeForOfflineAccount(bool canPlayFullGame);

MinecraftAccountPtr accountForLaunch(BaseInstance* instance);

QString accountKindLabel(const MinecraftAccountPtr& account);
QString accountStatusLabel(const MinecraftAccountPtr& account);
QString accountMetaLine(const MinecraftAccountPtr& account);

bool blocksLaunch(const MinecraftAccountPtr& account);
QString launchBlockReason(const MinecraftAccountPtr& account);

}  // namespace LaunchAccountUtils
