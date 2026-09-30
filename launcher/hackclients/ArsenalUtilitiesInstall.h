// SPDX-License-Identifier: GPL-3.0-only
#pragma once

class MinecraftInstance;
class QWidget;

namespace HackClients {

/**
 * Resolve and download Arsenal NameProtect into a Fabric instance's mods folder.
 * No-ops (returns true) for non-Fabric instances or when already installed.
 * Shows a progress dialog when parent is non-null; soft-fails with a warning box.
 */
bool installArsenalUtilities(MinecraftInstance* instance, QWidget* parent = nullptr);

}  // namespace HackClients
