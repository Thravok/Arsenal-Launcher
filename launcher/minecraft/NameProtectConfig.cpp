// SPDX-License-Identifier: GPL-3.0-only
#include "NameProtectConfig.h"

#include "Application.h"
#include "FileSystem.h"
#include "minecraft/auth/AuthSession.h"
#include "settings/SettingsObject.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

namespace NameProtect {

bool maskingEnabled()
{
    if (!APPLICATION)
        return false;
    auto* settings = APPLICATION->settings();
    return settings && settings->get(MaskAccountsSetting).toBool();
}

QString maskProfileName(const QString& profileName)
{
    const QString trimmed = profileName.trimmed();
    if (trimmed.isEmpty())
        return QStringLiteral("***");
    if (trimmed.size() == 1)
        return trimmed + QStringLiteral("***");
    if (trimmed.size() == 2)
        return trimmed.left(1) + QStringLiteral("***") + trimmed.right(1);
    return trimmed.left(1) + QStringLiteral("***") + trimmed.right(1);
}

QString maybeMaskProfileName(const QString& profileName)
{
    return maskingEnabled() ? maskProfileName(profileName) : profileName;
}

bool writeInstanceConfig(const QString& gameRoot, SettingsObject* settings, AuthSession* session, QString* errorOut)
{
    if (!settings) {
        if (errorOut)
            *errorOut = QStringLiteral("No settings object.");
        return false;
    }
    if (!settings->get(ManageFromLauncherSetting).toBool())
        return true;

    QJsonObject rules;
    {
        const QString rulesRaw = settings->get(RulesSetting).toString().trimmed();
        if (!rulesRaw.isEmpty()) {
            QJsonParseError err;
            const auto doc = QJsonDocument::fromJson(rulesRaw.toUtf8(), &err);
            if (err.error == QJsonParseError::NoError && doc.isObject())
                rules = doc.object();
        }
    }

    const QString selfAlias = settings->get(SelfAliasSetting).toString().trimmed();
    const bool enabled = settings->get(EnabledSetting).toBool();

    if (session && !session->player_name.isEmpty() && !selfAlias.isEmpty())
        rules.insert(session->player_name, selfAlias);

    QJsonObject root;
    root.insert(QStringLiteral("schemaVersion"), 1);
    root.insert(QStringLiteral("enabled"), enabled);
    root.insert(QStringLiteral("selfAlias"), selfAlias.isEmpty() ? QStringLiteral("Hidden") : selfAlias);
    root.insert(QStringLiteral("rules"), rules);

    const QString configDir = FS::PathCombine(gameRoot, QStringLiteral("config"));
    if (!FS::ensureFolderPathExists(configDir)) {
        if (errorOut)
            *errorOut = QStringLiteral("Could not create config folder.");
        return false;
    }

    const QString path = FS::PathCombine(configDir, ConfigFileName);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (errorOut)
            *errorOut = QStringLiteral("Could not write %1.").arg(ConfigFileName);
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

}  // namespace NameProtect
