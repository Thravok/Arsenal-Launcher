// SPDX-License-Identifier: GPL-3.0-only
#include "BaritoneInstallTask.h"

#include "Application.h"
#include "FileSystem.h"
#include "hackclients/BaritoneMaven.h"
#include "hackclients/HackClientsMeta.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "settings/INISettingsObject.h"

namespace HackClients {

BaritoneInstallTask::BaritoneInstallTask(BaritoneRelease release)
    : InstanceCreationTask(), m_release(std::move(release))
{}

bool BaritoneInstallTask::abort()
{
    if (!canAbort())
        return false;
    m_abort = true;
    if (m_job)
        m_job->abort();
    return Task::abort();
}

bool BaritoneInstallTask::createInstance()
{
    if (m_release.minecraftVersion.isEmpty()) {
        setError(tr("No Minecraft version selected for Baritone."));
        return false;
    }

    setStatus(tr("Resolving Fabric Loader…"));
    QString fabricLoader = resolveRecommendedFabricLoader();
    if (fabricLoader.isEmpty()) {
        setError(tr("Could not resolve a Fabric Loader version from launcher metadata."));
        return false;
    }

    setStatus(tr("Creating Baritone instance for Minecraft %1…").arg(m_release.minecraftVersion));

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

    const QString modsDir = FS::PathCombine(inst.gameRoot(), "mods");
    FS::ensureFolderPathExists(modsDir);

    const QString destName = QString("baritone-fabric-%1.jar").arg(m_release.minecraftVersion);
    QString error;
    const bool ok = BaritoneMaven::downloadBaritoneForMinecraft(
        APPLICATION->network(), m_release.minecraftVersion, FS::PathCombine(modsDir, destName), &error,
        [this](const QString& status) { setStatus(status); });

    if (!ok || m_abort) {
        setError(error.isEmpty() ? tr("Failed to download Baritone.") : error);
        return false;
    }
    return true;
}

}  // namespace HackClients
