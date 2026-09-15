// SPDX-License-Identifier: GPL-3.0-only
#include "LiquidBounceInstallTask.h"

#include "Application.h"
#include "FileSystem.h"
#include "MMCZip.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "net/ChecksumValidator.h"
#include "net/Request.h"
#include "settings/INISettingsObject.h"

#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QUrl>

namespace HackClients {

LiquidBounceInstallTask::LiquidBounceInstallTask(LiquidBounceBuild build)
    : InstanceCreationTask(), m_build(std::move(build))
{}

bool LiquidBounceInstallTask::abort()
{
    if (!canAbort())
        return false;
    m_abort = true;
    if (m_job)
        m_job->abort();
    return Task::abort();
}

bool LiquidBounceInstallTask::downloadFile(const QUrl& url, const QString& destPath, const QByteArray& sha256)
{
    FS::ensureFilePathExists(destPath);
    m_job.reset();
    m_job.reset(new NetJob(QString("Download %1").arg(url.fileName()), APPLICATION->network()));
    auto dl = Net::Request::makeFile(url, destPath);
    if (!sha256.isEmpty())
        dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha256, sha256));
    m_job->addNetAction(dl);

    QEventLoop loop;
    bool ok = false;
    connect(m_job.get(), &NetJob::succeeded, &loop, [&] { ok = true; });
    connect(m_job.get(), &Task::finished, &loop, &QEventLoop::quit, Qt::QueuedConnection);
    m_job->start();
    loop.exec();
    m_job.reset();
    return ok && !m_abort && QFileInfo::exists(destPath);
}

bool LiquidBounceInstallTask::downloadAndExtractClient(const QString& modsDir)
{
    if (m_build.filePid.isEmpty()) {
        setError(tr("LiquidBounce download file id is missing. Refresh the Hack Clients list and try again."));
        return false;
    }

    setStatus(tr("Downloading LiquidBounce %1…").arg(m_build.lbVersion));
    QString zipPath = FS::PathCombine(m_stagingPath, "liquidbounce-download.zip");
    QByteArray expected;
    if (!m_build.fileChecksumSha256.isEmpty())
        expected = QByteArray::fromHex(m_build.fileChecksumSha256.toLatin1());

    QUrl url(QString("https://api.liquidbounce.net/api/v3/file/%1").arg(m_build.filePid));
    if (!downloadFile(url, zipPath, expected)) {
        setError(tr("Failed to download LiquidBounce release ZIP."));
        return false;
    }

    setStatus(tr("Extracting LiquidBounce…"));
    QString extractDir = FS::PathCombine(m_stagingPath, "liquidbounce-extract");
    FS::ensureFolderPathExists(extractDir);
    auto files = MMCZip::extractDir(zipPath, extractDir);
    QFile::remove(zipPath);
    if (!files || files->isEmpty()) {
        setError(tr("Failed to extract LiquidBounce ZIP."));
        return false;
    }

    FS::ensureFolderPathExists(modsDir);
    bool foundJar = false;
    for (const auto& path : *files) {
        if (!path.endsWith(".jar", Qt::CaseInsensitive))
            continue;
        QFileInfo fi(path);
        if (!fi.exists())
            continue;
        QString dest = FS::PathCombine(modsDir, fi.fileName());
        if (QFile::exists(dest))
            QFile::remove(dest);
        if (!QFile::copy(fi.absoluteFilePath(), dest)) {
            setError(tr("Failed to copy %1 into mods/.").arg(fi.fileName()));
            return false;
        }
        foundJar = true;
    }
    FS::deletePath(extractDir);

    if (!foundJar) {
        setError(tr("LiquidBounce ZIP did not contain a JAR file."));
        return false;
    }
    return true;
}

bool LiquidBounceInstallTask::downloadCompanionMods(const QString& modsDir)
{
    // Fabric API + Fabric Language Kotlin (required by LiquidBounce nextgen)
    struct ModDl {
        QString url;
        QString filename;
    };
    QList<ModDl> mods;

    if (!m_build.fabricApiVersion.isEmpty()) {
        auto ver = QString::fromUtf8(QUrl::toPercentEncoding(m_build.fabricApiVersion));
        mods.append({ QString("https://maven.fabricmc.net/net/fabricmc/fabric-api/fabric-api/%1/fabric-api-%2.jar")
                          .arg(ver, m_build.fabricApiVersion),
                      QString("fabric-api-%1.jar").arg(m_build.fabricApiVersion) });
    }
    if (!m_build.kotlinModVersion.isEmpty()) {
        auto ver = QString::fromUtf8(QUrl::toPercentEncoding(m_build.kotlinModVersion));
        mods.append({ QString("https://maven.fabricmc.net/net/fabricmc/fabric-language-kotlin/%1/fabric-language-kotlin-%2.jar")
                          .arg(ver, m_build.kotlinModVersion),
                      QString("fabric-language-kotlin-%1.jar").arg(m_build.kotlinModVersion) });
    }

    for (const auto& mod : mods) {
        if (m_abort)
            return false;
        setStatus(tr("Downloading %1…").arg(mod.filename));
        QString dest = FS::PathCombine(modsDir, mod.filename);
        if (!downloadFile(QUrl(mod.url), dest)) {
            setError(tr("Failed to download required mod %1.").arg(mod.filename));
            return false;
        }
    }
    return true;
}

bool LiquidBounceInstallTask::createInstance()
{
    if (m_build.mcVersion.isEmpty() || m_build.fabricLoaderVersion.isEmpty()) {
        setError(tr("LiquidBounce build metadata is incomplete."));
        return false;
    }

    setStatus(tr("Creating LiquidBounce instance for Minecraft %1…").arg(m_build.mcVersion));

    const QString configPath = FS::PathCombine(m_stagingPath, "instance.cfg");
    auto instanceSettings = std::make_unique<INISettingsObject>(configPath);
    MinecraftInstance inst(m_globalSettings, std::move(instanceSettings), m_stagingPath);
    auto* components = inst.getPackProfile();
    components->buildingFromScratch();
    components->setComponentVersion("net.minecraft", m_build.mcVersion, true);
    components->setComponentVersion("net.fabricmc.fabric-loader", m_build.fabricLoaderVersion);

    inst.setName(name());
    if (!m_instIcon.isEmpty())
        inst.setIconKey(m_instIcon);
    inst.saveNow();

    QString modsDir = FS::PathCombine(inst.gameRoot(), "mods");

    if (!downloadAndExtractClient(modsDir))
        return false;
    if (!downloadCompanionMods(modsDir))
        return false;

    return true;
}

}  // namespace HackClients
