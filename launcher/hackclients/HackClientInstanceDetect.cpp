// SPDX-License-Identifier: GPL-3.0-only
#include "HackClientInstanceDetect.h"

#include "FileSystem.h"

#include <QDir>
#include <QFile>

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

bool isBaritoneJarFileName(const QString& fileName)
{
    return fileName.endsWith(".jar", Qt::CaseInsensitive) && fileName.contains("baritone", Qt::CaseInsensitive);
}

QStringList replaceOtherBaritoneJars(const QString& modsDir, const QString& keepFileName)
{
    QStringList removed;
    const QDir mods(modsDir);
    if (!mods.exists())
        return removed;

    for (const auto& entry : mods.entryList(QDir::Files)) {
        if (!isBaritoneJarFileName(entry))
            continue;
        if (!keepFileName.isEmpty() && entry.compare(keepFileName, Qt::CaseInsensitive) == 0)
            continue;
        if (QFile::remove(FS::PathCombine(modsDir, entry)))
            removed.append(entry);
    }
    return removed;
}

}  // namespace HackClients
