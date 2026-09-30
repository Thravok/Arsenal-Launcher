// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QString>

class SettingsObject;
struct AuthSession;

namespace NameProtect {

/** Setting keys registered on the global SettingsObject. */
inline constexpr auto EnabledSetting = "NameProtectEnabled";
inline constexpr auto SelfAliasSetting = "NameProtectSelfAlias";
inline constexpr auto RulesSetting = "NameProtectRules";  // JSON object as string
inline constexpr auto MaskAccountsSetting = "NameProtectMaskAccounts";
inline constexpr auto ManageFromLauncherSetting = "NameProtectManageFromLauncher";

inline constexpr auto ConfigFileName = "arsenal-nameprotect.json";

/** True when launcher UI should obfuscate profile names. */
bool maskingEnabled();

/** Mask a profile name for launcher UI (e.g. A***n). */
QString maskProfileName(const QString& profileName);

/** Return masked name when masking is on, otherwise the original. */
QString maybeMaskProfileName(const QString& profileName);

/**
 * Write config/arsenal-nameprotect.json under gameRoot when
 * NameProtectManageFromLauncher is enabled.
 * Injects session player_name → selfAlias into rules.
 * Returns false only on I/O failure (caller may log and continue).
 */
bool writeInstanceConfig(const QString& gameRoot, SettingsObject* settings, AuthSession* session, QString* errorOut = nullptr);

}  // namespace NameProtect
