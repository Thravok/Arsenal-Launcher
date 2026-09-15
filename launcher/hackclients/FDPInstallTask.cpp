// SPDX-License-Identifier: GPL-3.0-only
#include "FDPInstallTask.h"

#include "Application.h"
#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "net/Request.h"
#include "settings/INISettingsObject.h"

#include <QEventLoop>
#include <QFileInfo>
#include <QUrl>

namespace HackClients {

/** Forge version documented in SkidderMC/FDPClient docs/INSTALLING.md */
static const QString FDP_MINECRAFT_VERSION = QStringLiteral("1.8.9");
static const QString FDP_FORGE_VERSION = QStringLiteral("11.15.1.2318");

FDPInstallTask::FDPInstallTask(FDPRelease release) : InstanceCreationTask(), m_release(std::move(release)) {}

bool FDPInstallTask::abort()
{
    if (!canAbort())
        return false;
    m_abort = true;
    if (m_job)
        m_job->abort();
    return Task::abort();
}

bool FDPInstallTask::downloadFile(const QUrl& url, const QString& destPath)
{
    FS::ensureFilePathExists(destPath);
    m_job.reset();
    m_job.reset(new NetJob(QString("Download %1").arg(url.fileName()), APPLICATION->network()));
    m_job->addNetAction(Net::Request::makeFile(url, destPath));

    QEventLoop loop;
    bool ok = false;
    connect(m_job.get(), &NetJob::succeeded, &loop, [&] { ok = true; });
    connect(m_job.get(), &Task::finished, &loop, &QEventLoop::quit, Qt::QueuedConnection);
    m_job->start();
    loop.exec();
    m_job.reset();
    return ok && !m_abort && QFileInfo::exists(destPath);
}

bool FDPInstallTask::downloadMods(const QString& modsDir)
{
    FS::ensureFolderPathExists(modsDir);

    setStatus(tr("Downloading FDPClient %1…").arg(m_release.tagName));
    QString jarName = m_release.jarName.isEmpty()
                          ? QString("FDPClient-%1.jar").arg(m_release.tagName)
                          : m_release.jarName;
    if (!downloadFile(QUrl(m_release.jarUrl), FS::PathCombine(modsDir, jarName))) {
        setError(tr("Failed to download FDPClient JAR from GitHub."));
        return false;
    }
    return true;
}

bool FDPInstallTask::createInstance()
{
    if (m_release.tagName.isEmpty() || m_release.jarUrl.isEmpty()) {
        setError(tr("No FDPClient release selected."));
        return false;
    }

    setStatus(tr("Creating FDPClient instance for Minecraft %1…").arg(FDP_MINECRAFT_VERSION));

    const QString configPath = FS::PathCombine(m_stagingPath, "instance.cfg");
    auto instanceSettings = std::make_unique<INISettingsObject>(configPath);
    MinecraftInstance inst(m_globalSettings, std::move(instanceSettings), m_stagingPath);
    auto* components = inst.getPackProfile();
    components->buildingFromScratch();
    components->setComponentVersion("net.minecraft", FDP_MINECRAFT_VERSION, true);
    components->setComponentVersion("net.minecraftforge", FDP_FORGE_VERSION);

    inst.setName(name());
    if (!m_instIcon.isEmpty())
        inst.setIconKey(m_instIcon);
    inst.saveNow();

    return downloadMods(FS::PathCombine(inst.gameRoot(), "mods"));
}

}  // namespace HackClients
