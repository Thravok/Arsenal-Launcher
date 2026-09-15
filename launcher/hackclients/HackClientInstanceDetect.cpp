// SPDX-License-Identifier: GPL-3.0-only
#include "HackClientInstanceDetect.h"

#include <QDir>

namespace HackClients {

bool modsFolderHasMeteorClient(const QString& modsDir)
{
    const QDir mods(modsDir);
    if (!mods.exists())
        return false;
    for (const auto& entry : mods.entryList(QDir::Files)) {
        if (!entry.endsWith(".jar", Qt::CaseInsensitive))
            continue;
        if (entry.contains("meteor-client", Qt::CaseInsensitive) || entry.contains("meteor_client", Qt::CaseInsensitive))
            return true;
    }
    return false;
}

}  // namespace HackClients
