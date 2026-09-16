// SPDX-License-Identifier: GPL-3.0-only
#include "BaritoneModInstallTask.h"

#include "Application.h"
#include "BaritoneMaven.h"
#include "FileSystem.h"
#include "HackClientInstanceDetect.h"

#include <QFile>
#include <QFileInfo>

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

    if (modsFolderHasMeteorClient(m_modsDir)) {
        emitFailed(tr("This instance already uses Meteor Client's Baritone fork. "
                      "Standalone Fabric Baritone is not compatible and would replace that JAR."));
        return;
    }

    FS::ensureFolderPathExists(m_modsDir);

    const QString destName = QString("baritone-fabric-%1.jar").arg(m_minecraftVersion);
    const QString destPath = FS::PathCombine(m_modsDir, destName);
    const QString tempPath = destPath + QStringLiteral(".part");
    QFile::remove(tempPath);

    QString error;
    const bool ok = m_mavenVersion.isEmpty()
                        ? BaritoneMaven::downloadBaritoneForMinecraft(
                              APPLICATION->network(), m_minecraftVersion, tempPath, &error,
                              [this](const QString& status) { setStatus(status); })
                        : BaritoneMaven::downloadBaritone(APPLICATION->network(), m_mavenVersion, m_minecraftVersion,
                                                          tempPath, &error,
                                                          [this](const QString& status) { setStatus(status); });

    if (m_abort) {
        QFile::remove(tempPath);
        emitAborted();
        return;
    }
    if (!ok) {
        QFile::remove(tempPath);
        emitFailed(error.isEmpty() ? tr("Failed to download Baritone.") : error);
        return;
    }

    // Only drop previous Baritone JARs after the new file is on disk.
    setStatus(tr("Installing Baritone…"));
    QFile::remove(destPath);
    if (!QFile::rename(tempPath, destPath) && !QFile::copy(tempPath, destPath)) {
        QFile::remove(tempPath);
        emitFailed(tr("Failed to install Baritone into the mods folder."));
        return;
    }
    QFile::remove(tempPath);
    replaceOtherBaritoneJars(m_modsDir, QFileInfo(destPath).fileName());
    emitSucceeded();
}

}  // namespace HackClients
