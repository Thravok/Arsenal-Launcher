// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QString>
#include <QStringList>

namespace HackClients {

/** True if modsDir contains a Meteor Client JAR. */
bool modsFolderHasMeteorClient(const QString& modsDir);

/** True if fileName looks like a Baritone JAR (standalone or Meteor fork). */
bool isBaritoneJarFileName(const QString& fileName);

/**
 * Remove Baritone JARs from modsDir except keepFileName (compared by file name).
 * Returns the names that were removed.
 */
QStringList replaceOtherBaritoneJars(const QString& modsDir, const QString& keepFileName);

}  // namespace HackClients
