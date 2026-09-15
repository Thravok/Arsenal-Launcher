// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QString>

namespace HackClients {

/** True if modsDir contains a Meteor Client JAR. */
bool modsFolderHasMeteorClient(const QString& modsDir);

}  // namespace HackClients
