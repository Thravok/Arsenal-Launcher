// SPDX-License-Identifier: GPL-3.0-only
#include "HackClientCatalog.h"

#include <QCoreApplication>
#include <QHash>
#include <QSet>

namespace HackClients {

namespace {

struct ClientMeta {
    QString displayName;
    QString iconKey;
};

/** Sole ordered membership per category — also defines catalog membership. */
const QHash<HackClientCategory, QList<int>>& orderByCategory()
{
    static const QHash<HackClientCategory, QList<int>> kOrder = {
        { HackClientCategory::Anarchy,
          { ClientMeteor, ClientLambda, ClientImpact, ClientLiquidBounce, ClientWurst, ClientEpsilon } },
        { HackClientCategory::PvP, { ClientLiquidBounce, ClientWurst, ClientEpsilon, ClientFDP } },
        { HackClientCategory::Utility,
          { ClientBaritone, ClientMeteor, ClientLambda, ClientWurst, ClientLiquidBounce, ClientEpsilon } },
    };
    return kOrder;
}

const QHash<int, ClientMeta>& clientMeta()
{
    static const QHash<int, ClientMeta> kMeta = {
        { ClientMeteor, { QCoreApplication::translate("HackClients", "Meteor"), QStringLiteral("meteor") } },
        { ClientLambda, { QCoreApplication::translate("HackClients", "Lambda"), QStringLiteral("lambda") } },
        { ClientImpact, { QCoreApplication::translate("HackClients", "Impact"), QStringLiteral("impact") } },
        { ClientLiquidBounce,
          { QCoreApplication::translate("HackClients", "LiquidBounce"), QStringLiteral("liquidbounce") } },
        { ClientWurst, { QCoreApplication::translate("HackClients", "Wurst"), QStringLiteral("wurst") } },
        { ClientEpsilon, { QCoreApplication::translate("HackClients", "Epsilon"), QStringLiteral("epsilon") } },
        { ClientFDP, { QCoreApplication::translate("HackClients", "FDPClient"), QStringLiteral("fdp") } },
        { ClientBaritone,
          { QCoreApplication::translate("HackClients", "Baritone"), QStringLiteral("loadermods") } },
    };
    return kMeta;
}

}  // namespace

QList<HackClientCategory> hackClientCategoryOrder()
{
    return { HackClientCategory::Anarchy, HackClientCategory::PvP, HackClientCategory::Utility };
}

QString hackClientCategoryTitle(HackClientCategory category)
{
    switch (category) {
        case HackClientCategory::Anarchy:
            return QCoreApplication::translate("HackClients", "Anarchy");
        case HackClientCategory::PvP:
            return QCoreApplication::translate("HackClients", "PvP");
        case HackClientCategory::Utility:
            return QCoreApplication::translate("HackClients", "Utility");
    }
    return {};
}

QString hackClientCategoryPageId(HackClientCategory category)
{
    switch (category) {
        case HackClientCategory::Anarchy:
            return QStringLiteral("hackclients-anarchy");
        case HackClientCategory::PvP:
            return QStringLiteral("hackclients-pvp");
        case HackClientCategory::Utility:
            return QStringLiteral("hackclients-utility");
    }
    return QStringLiteral("hackclients");
}

QList<HackClientCatalogEntry> hackClientCatalog()
{
    QHash<int, HackClientCatalogEntry> byId;
    for (const auto category : hackClientCategoryOrder()) {
        for (int id : orderByCategory().value(category)) {
            auto it = byId.find(id);
            if (it == byId.end()) {
                const auto meta = clientMeta().value(id);
                HackClientCatalogEntry entry;
                entry.clientId = id;
                entry.displayName = meta.displayName;
                entry.iconKey = meta.iconKey;
                entry.categories = { category };
                byId.insert(id, entry);
            } else if (!it->categories.contains(category)) {
                it->categories.append(category);
            }
        }
    }

    // Stable overall order: first appearance across category order lists.
    QList<HackClientCatalogEntry> out;
    QSet<int> seen;
    for (const auto category : hackClientCategoryOrder()) {
        for (int id : orderByCategory().value(category)) {
            if (seen.contains(id))
                continue;
            seen.insert(id);
            out.append(byId.value(id));
        }
    }
    return out;
}

QList<HackClientCatalogEntry> hackClientsForCategory(HackClientCategory category)
{
    QList<HackClientCatalogEntry> out;
    const auto& meta = clientMeta();
    for (int id : orderByCategory().value(category)) {
        const auto m = meta.value(id);
        if (m.displayName.isEmpty())
            continue;
        HackClientCatalogEntry entry;
        entry.clientId = id;
        entry.displayName = m.displayName;
        entry.iconKey = m.iconKey;
        entry.categories = { category };
        out.append(entry);
    }
    return out;
}

}  // namespace HackClients
