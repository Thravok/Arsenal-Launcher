// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "HackClientTypes.h"

#include <QList>

namespace HackClients {

/** Curated anarchy / utility mods for quick install into Fabric instances. */
class AnarchyUtilityCatalog {
   public:
    static QList<AnarchyUtilitySpec> all();
    static AnarchyUtilitySpec byId(const QString& id);
};

}  // namespace HackClients
