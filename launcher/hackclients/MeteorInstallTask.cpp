// SPDX-License-Identifier: GPL-3.0-only
#include "MeteorInstallTask.h"

#include "Application.h"
#include "FileSystem.h"
#include "hackclients/HackClientsMeta.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "net/Request.h"
#include "net/Mode.h"
#include "settings/INISettingsObject.h"
#include "tasks/Task.h"

#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QUrl>

namespace HackClients {

MeteorInstallTask::MeteorInstallTask(MeteorBuild build)
    : InstanceCreationTask(), m_build(std::move(build))
{}

bool MeteorInstallTask::abort()
{
    if (!canAbort())
        return false;
    m_abort = true;
    if (m_job)
        m_job->abort();
    return Task::abort();
}

bool MeteorInstallTask::downloadFile(const QUrl& url, const QString& destPath)
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

bool MeteorInstallTask::downloadMods(const QString& modsDir)
{
    FS::ensureFolderPathExists(modsDir);

    setStatus(tr("Downloading Meteor Client for Minecraft %1…").arg(m_build.minecraftVersion));
    QString meteorName = QString("meteor-client-%1-%2.jar").arg(m_build.minecraftVersion).arg(m_build.buildNumber);
    QString meteorPath = FS::PathCombine(modsDir, meteorName);
    QUrl meteorUrl(QString("https://meteorclient.com/api/download?version=%1")
                       .arg(QString::fromUtf8(QUrl::toPercentEncoding(m_build.minecraftVersion))));
    if (!downloadFile(meteorUrl, meteorPath)) {
        setError(tr("Failed to download Meteor Client from meteorclient.com."));
        return false;
    }

    // Optional Meteor-forked Baritone (no longer bundled in the client).
    setStatus(tr("Downloading Meteor Baritone…"));
    QString baritoneName = QString("baritone-meteor-%1.jar").arg(m_build.minecraftVersion);
    QString baritonePath = FS::PathCombine(modsDir, baritoneName);
    QUrl baritoneUrl(QString("https://meteorclient.com/api/downloadBaritone?version=%1")
                         .arg(QString::fromUtf8(QUrl::toPercentEncoding(m_build.minecraftVersion))));
    if (!downloadFile(baritoneUrl, baritonePath)) {
        // Non-fatal: Meteor runs without Baritone; pathfinding just won't be available.
        QFile::remove(baritonePath);
    }

    return true;
}

bool MeteorInstallTask::createInstance()
{
    if (m_build.minecraftVersion.isEmpty()) {
        setError(tr("No Meteor Minecraft version selected."));
        return false;
    }

    setStatus(tr("Resolving Fabric Loader…"));
    QString fabricLoader = resolveRecommendedFabricLoader();
    if (fabricLoader.isEmpty()) {
        setError(tr("Could not resolve a Fabric Loader version from launcher metadata."));
        return false;
    }

    setStatus(tr("Creating Meteor instance for Minecraft %1…").arg(m_build.minecraftVersion));

    const QString configPath = FS::PathCombine(m_stagingPath, "instance.cfg");
    auto instanceSettings = std::make_unique<INISettingsObject>(configPath);
    MinecraftInstance inst(m_globalSettings, std::move(instanceSettings), m_stagingPath);
    auto* components = inst.getPackProfile();
    components->buildingFromScratch();
    components->setComponentVersion("net.minecraft", m_build.minecraftVersion, true);
    components->setComponentVersion("net.fabricmc.fabric-loader", fabricLoader);

    inst.setName(name());
    if (!m_instIcon.isEmpty())
        inst.setIconKey(m_instIcon);
    inst.saveNow();

    return downloadMods(FS::PathCombine(inst.gameRoot(), "mods"));
}

}  // namespace HackClients
