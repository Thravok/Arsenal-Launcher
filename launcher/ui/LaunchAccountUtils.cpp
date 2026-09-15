// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Arsenal Launcher
 *  Copyright (C) 2026 Arsenal Launcher Contributors
 */

#include "LaunchAccountUtils.h"

#include <QCoreApplication>

#include "Application.h"
#include "minecraft/auth/AccountData.h"
#include "minecraft/auth/AccountList.h"

namespace LaunchAccountUtils {

namespace {

QString tr(const char* text, const char* comment = nullptr)
{
    return QCoreApplication::translate("LaunchAccountUtils", text, comment);
}

}  // namespace

MinecraftAccountPtr accountForLaunch(BaseInstance* instance)
{
    if (!instance) {
        return nullptr;
    }

    auto accounts = APPLICATION->accounts();
    if (instance->settings()->get("UseAccountForInstance").toBool()) {
        const QString profileId = instance->settings()->get("InstanceAccountId").toString();
        if (profileId.isEmpty()) {
            return nullptr;
        }
        const int index = accounts->findAccountByProfileId(profileId);
        if (index >= 0) {
            return accounts->at(index);
        }
        return nullptr;
    }

    return accounts->defaultAccount();
}

QString accountKindLabel(const MinecraftAccountPtr& account)
{
    if (!account) {
        return QString();
    }

    switch (account->accountData()->type) {
    case AccountType::MSA:
        return tr("Microsoft");
    case AccountType::TheAltening:
        return tr("The Altening");
    case AccountType::Offline:
        return tr("Offline");
    default:
        break;
    }

    return tr("Unknown");
}

QString accountStatusLabel(const MinecraftAccountPtr& account)
{
    if (!account) {
        return tr("No account", "Account status");
    }

    switch (account->accountState()) {
    case AccountState::Unchecked:
        return tr("Unchecked", "Account status");
    case AccountState::Offline:
        return tr("Offline", "Account status");
    case AccountState::Online:
        return tr("Ready", "Account status");
    case AccountState::Working:
        return tr("Working", "Account status");
    case AccountState::Errored:
        return tr("Errored", "Account status");
    case AccountState::Expired:
        return tr("Expired", "Account status");
    case AccountState::Disabled:
        return tr("Disabled", "Account status");
    case AccountState::Gone:
        return tr("Gone", "Account status");
    }

    return tr("Unknown", "Account status");
}

QString accountMetaLine(const MinecraftAccountPtr& account)
{
    if (!account) {
        return QString();
    }
    return accountKindLabel(account) + QStringLiteral(" · ") + accountStatusLabel(account);
}

bool blocksLaunch(const MinecraftAccountPtr& account)
{
    if (!account) {
        return true;
    }

    switch (account->accountState()) {
    case AccountState::Expired:
    case AccountState::Disabled:
    case AccountState::Gone:
        return true;
    default:
        return false;
    }
}

QString launchBlockReason(const MinecraftAccountPtr& account)
{
    if (!account) {
        return tr("Choose an account before launching.");
    }

    switch (account->accountState()) {
    case AccountState::Expired:
        return tr("This account has expired. Sign in again in Manage Accounts, or pick another account here.");
    case AccountState::Disabled:
        return tr("This account is disabled. Remove it and add it again in Manage Accounts, or pick another account.");
    case AccountState::Gone:
        return tr("This account no longer exists on Mojang's servers. Pick another account or add a replacement.");
    default:
        break;
    }

    return tr("This account cannot be used to launch. Pick another account.");
}

}  // namespace LaunchAccountUtils
