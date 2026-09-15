// SPDX-License-Identifier: GPL-3.0-only
#include "WurstInstallTask.h"

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

WurstInstallTask::WurstInstallTask(WurstRelease release)
    : InstanceCreationTask(), m_release(std::move(release))
{}

bool WurstInstallTask::abort()
{
    if (!canAbort())
        return false;
    m_abort = true;
    if (m_job)
        m_job->abort();
    return Task::abort();
}

bool WurstInstallTask::downloadFile(const QUrl& url, const QString& destPath)
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

bool WurstInstallTask::fetchGradleProperties()
{
    setStatus(tr("Fetching Wurst dependency versions…"));
    QByteArray data;
    QString encodedTag = QString::fromUtf8(QUrl::toPercentEncoding(m_release.tagName));
    QUrl url(QString("https://raw.githubusercontent.com/Wurst-Imperium/Wurst7/%1/gradle.properties")
                 .arg(encodedTag));

    m_job.reset();
    m_job.reset(new NetJob("Wurst gradle.properties", APPLICATION->network()));
    auto [action, response] = Net::Request::makeByteArray(url);
    m_job->addNetAction(action);

    QEventLoop loop;
    bool ok = false;
    connect(m_job.get(), &NetJob::succeeded, &loop, [&] {
        ok = true;
        data = *response;
    });
    connect(m_job.get(), &Task::finished, &loop, &QEventLoop::quit, Qt::QueuedConnection);
    m_job->start();
    loop.exec();
    m_job.reset();

    if (!ok || m_abort || data.isEmpty()) {
        setError(tr("Failed to fetch gradle.properties for Wurst %1.").arg(m_release.tagName));
        return false;
    }

    const auto lines = QString::fromUtf8(data).split('\n');
    auto take = [&](const QString& key) -> QString {
        const QString prefix = key + '=';
        for (QString line : lines) {
            line = line.trimmed();
            if (line.isEmpty() || line.startsWith('#'))
                continue;
            if (line.startsWith(prefix))
                return line.mid(prefix.size()).trimmed();
        }
        return {};
    };

    m_release.fabricLoaderVersion = take("loader_version");
    m_release.fabricApiVersion = take("fabric_api_version");

    QString mcFromProps = take("minecraft_version");
    if (!mcFromProps.isEmpty())
        m_release.minecraftVersion = mcFromProps;

    if (m_release.fabricLoaderVersion.isEmpty() || m_release.fabricApiVersion.isEmpty()) {
        setError(tr("Wurst gradle.properties is missing required Fabric versions."));
        return false;
    }
    return true;
}

bool WurstInstallTask::downloadMods(const QString& modsDir)
{
    FS::ensureFolderPathExists(modsDir);

    setStatus(tr("Downloading Wurst %1…").arg(m_release.tagName));
    QString wurstName = m_release.jarName.isEmpty()
                            ? QString("Wurst-Client-%1.jar").arg(m_release.tagName)
                            : m_release.jarName;
    if (!downloadFile(QUrl(m_release.jarUrl), FS::PathCombine(modsDir, wurstName))) {
        setError(tr("Failed to download Wurst JAR from GitHub."));
        return false;
    }

    {
        const QString ver = m_release.fabricApiVersion;
        QString enc = QString::fromUtf8(QUrl::toPercentEncoding(ver));
        QString filename = QString("fabric-api-%1.jar").arg(ver);
        setStatus(tr("Downloading %1…").arg(filename));
        QUrl url(QString("https://maven.fabricmc.net/net/fabricmc/fabric-api/fabric-api/%1/fabric-api-%2.jar")
                     .arg(enc, enc));
        if (!downloadFile(url, FS::PathCombine(modsDir, filename))) {
            setError(tr("Failed to download Fabric API %1.").arg(ver));
            return false;
        }
    }

    return true;
}

bool WurstInstallTask::createInstance()
{
    if (m_release.tagName.isEmpty() || m_release.jarUrl.isEmpty()) {
        setError(tr("No Wurst release selected."));
        return false;
    }

    if (!fetchGradleProperties())
        return false;

    setStatus(tr("Creating Wurst instance for Minecraft %1…").arg(m_release.minecraftVersion));

    const QString configPath = FS::PathCombine(m_stagingPath, "instance.cfg");
    auto instanceSettings = std::make_unique<INISettingsObject>(configPath);
    MinecraftInstance inst(m_globalSettings, std::move(instanceSettings), m_stagingPath);
    auto* components = inst.getPackProfile();
    components->buildingFromScratch();
    components->setComponentVersion("net.minecraft", m_release.minecraftVersion, true);
    components->setComponentVersion("net.fabricmc.fabric-loader", m_release.fabricLoaderVersion);

    inst.setName(name());
    if (!m_instIcon.isEmpty())
        inst.setIconKey(m_instIcon);
    inst.saveNow();

    return downloadMods(FS::PathCombine(inst.gameRoot(), "mods"));
}

}  // namespace HackClients
