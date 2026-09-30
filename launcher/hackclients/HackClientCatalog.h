// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QList>
#include <QString>

namespace HackClients {

/** Sidebar sections on Create Instance → Hack Clients (display order). */
enum class HackClientCategory {
    Anarchy = 0,
    PvP = 1,
    Utility = 2,
};

/**
 * Stable client ids used by HackClientsPage.
 */
enum HackClientId {
    ClientNone = 0,
    ClientLiquidBounce = 1,
    ClientImpact = 2,
    ClientMeteor = 3,
    ClientLambda = 4,
    ClientBaritone = 5,
    ClientFDP = 6,
    ClientWurst = 7,
    ClientEpsilon = 8,
};

struct HackClientCatalogEntry {
    int clientId = ClientNone;
    QString displayName;
    QString iconKey;  // empty → themed loadermods fallback
    QList<HackClientCategory> categories;
};

/** Sidebar pages in display order (Anarchy, PvP, Utility). */
QList<HackClientCategory> hackClientCategoryOrder();

/** Translated sidebar / page title. */
QString hackClientCategoryTitle(HackClientCategory category);

/** Stable New Instance page id, e.g. hackclients-anarchy. */
QString hackClientCategoryPageId(HackClientCategory category);

/**
 * Unique catalog of installable clients (categories derived from per-category order).
 */
QList<HackClientCatalogEntry> hackClientCatalog();

/**
 * Clients for @p category in preferred display order.
 * Order lists are the single source of membership.
 */
QList<HackClientCatalogEntry> hackClientsForCategory(HackClientCategory category);

}  // namespace HackClients
