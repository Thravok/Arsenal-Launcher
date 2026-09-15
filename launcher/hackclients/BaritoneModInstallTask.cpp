// SPDX-License-Identifier: GPL-3.0-only
#include "BaritoneModInstallTask.h"

#include "Application.h"
#include "BaritoneMaven.h"
#include "FileSystem.h"

#include <QDir>
#include <QFile>

namespace HackClients {

BaritoneModInstallTask::BaritoneModInstallTask(QString modsDir, QString minecraftVersion, QString mavenVersion,
                                                 QObject* parent)
    : Task(parent)
    , m_modsDir(std::move(modsDir))
    , m_minecraftVersion(std::move(minecraftVersion))
    , m_mavenVersion(std::move(mavenVersion))
{
    setAbortable(true);
}

bool BaritoneModInstallTask::abort()
{
    if (!canAbort())
        return false;
    m_abort = true;
    if (m_job)
        m_job->abort();
    return Task::abort();
}

void BaritoneModInstallTask::executeTask()
{
    if (m_minecraftVersion.isEmpty()) {
        emitFailed(tr("Cannot install Baritone: Minecraft version is unknown."));
        return;
    }

    FS::ensureFolderPathExists(m_modsDir);

    setStatus(tr("Removing older Baritone jars…"));
    const QDir mods(m_modsDir);
    for (const auto& entry : mods.entryList(QDir::Files)) {
        if (entry.contains("baritone", Qt::CaseInsensitive))
            QFile::remove(FS::PathCombine(m_modsDir, entry));
    }

    const QString destName = QString("baritone-fabric-%1.jar").arg(m_minecraftVersion);
    QString error;
    const bool ok = m_mavenVersion.isEmpty()
                        ? BaritoneMaven::downloadBaritoneForMinecraft(
                              APPLICATION->network(), m_minecraftVersion, FS::PathCombine(m_modsDir, destName), &error,
                              [this](const QString& status) { setStatus(status); })
                        : BaritoneMaven::downloadBaritone(APPLICATION->network(), m_mavenVersion, m_minecraftVersion,
                                                          FS::PathCombine(m_modsDir, destName), &error,
                                                          [this](const QString& status) { setStatus(status); });

    if (m_abort) {
        emitAborted();
        return;
    }
    if (!ok) {
        emitFailed(error.isEmpty() ? tr("Failed to download Baritone.") : error);
        return;
    }
    emitSucceeded();
}

}  // namespace HackClients
