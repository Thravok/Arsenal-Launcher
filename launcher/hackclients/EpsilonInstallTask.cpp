// SPDX-License-Identifier: GPL-3.0-only
#include "EpsilonInstallTask.h"

#include "Application.h"
#include "FileSystem.h"
#include "hackclients/HackClientsMeta.h"
#include "hackclients/ModrinthModDependencyResolver.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "net/Request.h"
#include "settings/INISettingsObject.h"

#include <QEventLoop>
#include <QFileInfo>
#include <QUrl>

namespace HackClients {

EpsilonInstallTask::EpsilonInstallTask(EpsilonRelease release)
    : InstanceCreationTask(), m_release(std::move(release))
{}

bool EpsilonInstallTask::abort()
{
    if (!canAbort())
        return false;
    m_abort = true;
    if (m_job)
        m_job->abort();
    return Task::abort();
}

bool EpsilonInstallTask::downloadFile(const QUrl& url, const QString& destPath)
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

bool EpsilonInstallTask::downloadFabricApi(const QString& modsDir)
{
    setStatus(tr("Resolving Fabric API for Minecraft %1…").arg(m_release.minecraftVersion));

    ModrinthModDependencyResolver resolver(APPLICATION->network());
    QList<AnarchyUtilityResolvedFile> plan;
    QString error;
    if (!resolver.resolve(m_release.minecraftVersion, modsDir, { QStringLiteral("fabric-api") }, &plan, &error,
                          [this](const QString& status) { setStatus(status); })) {
        if (m_abort)
            return false;
        setError(error.isEmpty() ? tr("Failed to resolve Fabric API from Modrinth.") : error);
        return false;
    }

    for (const auto& file : plan) {
        if (file.alreadyInstalled)
            continue;
        setStatus(tr("Downloading %1…").arg(file.fileName));
        if (!downloadFile(file.downloadUrl, FS::PathCombine(modsDir, file.fileName))) {
            setError(tr("Failed to download %1.").arg(file.fileName));
            return false;
        }
    }
    return true;
}

bool EpsilonInstallTask::downloadMods(const QString& modsDir)
{
    FS::ensureFolderPathExists(modsDir);

    setStatus(tr("Downloading Epsilon %1…").arg(m_release.epsilonVersion));
    QString jarName = m_release.jarName.isEmpty()
                          ? QString("epsilon-fabric-%1-%2.jar").arg(m_release.minecraftVersion, m_release.epsilonVersion)
                          : m_release.jarName;
    if (!downloadFile(QUrl(m_release.jarUrl), FS::PathCombine(modsDir, jarName))) {
        setError(tr("Failed to download Epsilon JAR from GitHub."));
        return false;
    }

    return downloadFabricApi(modsDir);
}

bool EpsilonInstallTask::createInstance()
{
    if (m_release.tagName.isEmpty() || m_release.jarUrl.isEmpty() || m_release.minecraftVersion.isEmpty()) {
        setError(tr("No Epsilon release selected."));
        return false;
    }

    setStatus(tr("Resolving Fabric Loader…"));
    QString fabricLoader = resolveRecommendedFabricLoader();
    if (fabricLoader.isEmpty()) {
        setError(tr("Could not resolve a Fabric Loader version from launcher metadata."));
        return false;
    }

    setStatus(tr("Creating Epsilon instance for Minecraft %1…").arg(m_release.minecraftVersion));

    const QString configPath = FS::PathCombine(m_stagingPath, "instance.cfg");
    auto instanceSettings = std::make_unique<INISettingsObject>(configPath);
    MinecraftInstance inst(m_globalSettings, std::move(instanceSettings), m_stagingPath);
    auto* components = inst.getPackProfile();
    components->buildingFromScratch();
    components->setComponentVersion("net.minecraft", m_release.minecraftVersion, true);
    components->setComponentVersion("net.fabricmc.fabric-loader", fabricLoader);

    inst.setName(name());
    if (!m_instIcon.isEmpty())
        inst.setIconKey(m_instIcon);
    inst.saveNow();

    return downloadMods(FS::PathCombine(inst.gameRoot(), "mods"));
}

}  // namespace HackClients
