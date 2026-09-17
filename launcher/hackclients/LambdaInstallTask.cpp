// SPDX-License-Identifier: GPL-3.0-only
#include "LambdaInstallTask.h"

#include "Application.h"
#include "BaritoneMaven.h"
#include "FileSystem.h"
#include "GradleProperties.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "net/Request.h"
#include "settings/INISettingsObject.h"

#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QUrl>

namespace HackClients {

LambdaInstallTask::LambdaInstallTask(LambdaRelease release) : InstanceCreationTask(), m_release(std::move(release)) {}

bool LambdaInstallTask::abort()
{
    if (!canAbort())
        return false;
    m_abort = true;
    if (m_job)
        m_job->abort();
    return Task::abort();
}

bool LambdaInstallTask::downloadFile(const QUrl& url, const QString& destPath)
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

bool LambdaInstallTask::downloadByteArray(const QUrl& url, QByteArray& out)
{
    out.clear();
    m_job.reset();
    m_job.reset(new NetJob(QString("Fetch %1").arg(url.fileName()), APPLICATION->network()));
    auto [action, response] = Net::Request::makeByteArray(url);
    m_job->addNetAction(action);

    QEventLoop loop;
    bool ok = false;
    connect(m_job.get(), &NetJob::succeeded, &loop, [&] {
        ok = true;
        out = *response;
    });
    connect(m_job.get(), &Task::finished, &loop, &QEventLoop::quit, Qt::QueuedConnection);
    m_job->start();
    loop.exec();
    m_job.reset();
    return ok && !m_abort && !out.isEmpty();
}

bool LambdaInstallTask::fetchGradleProperties()
{
    setStatus(tr("Fetching Lambda dependency versions…"));
    QByteArray data;
    // Tag contains '+'; percent-encode the whole path segment.
    QString encodedTag = QString::fromUtf8(QUrl::toPercentEncoding(m_release.tagName));
    QUrl url(QString("https://raw.githubusercontent.com/lambda-client/lambda/%1/gradle.properties").arg(encodedTag));

    m_job.reset();
    m_job.reset(new NetJob("Lambda gradle.properties", APPLICATION->network()));
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
        setError(tr("Failed to fetch gradle.properties for Lambda %1.").arg(m_release.tagName));
        return false;
    }

    const QString props = QString::fromUtf8(data);
    m_release.fabricLoaderVersion = gradleProperty(props, QStringLiteral("fabricLoaderVersion"));
    m_release.fabricApiVersion = gradleProperty(props, QStringLiteral("fabricApiVersion"));
    m_release.kotlinFabricVersion = gradleProperty(props, QStringLiteral("kotlinFabricVersion"));
    m_release.kotlinVersion = gradleProperty(props, QStringLiteral("kotlinVersion"));
    m_release.baritoneVersion = gradleProperty(props, QStringLiteral("baritoneVersion"));

    QString mcFromProps = gradleProperty(props, QStringLiteral("minecraftVersion"));
    if (!mcFromProps.isEmpty())
        m_release.minecraftVersion = mcFromProps;

    if (m_release.fabricLoaderVersion.isEmpty() || m_release.fabricApiVersion.isEmpty() || m_release.kotlinFabricVersion.isEmpty() ||
        m_release.kotlinVersion.isEmpty()) {
        setError(tr("Lambda gradle.properties is missing required Fabric/Kotlin versions."));
        return false;
    }
    return true;
}

bool LambdaInstallTask::downloadMods(const QString& modsDir)
{
    FS::ensureFolderPathExists(modsDir);

    setStatus(tr("Downloading Lambda %1…").arg(m_release.tagName));
    QString lambdaName = m_release.jarName.isEmpty() ? QString("lambda-%1.jar").arg(m_release.tagName) : m_release.jarName;
    if (!downloadFile(QUrl(m_release.jarUrl), FS::PathCombine(modsDir, lambdaName))) {
        setError(tr("Failed to download Lambda JAR from GitHub."));
        return false;
    }

    // fabric-api-{api}+{mc}.jar
    {
        QString ver = QString("%1+%2").arg(m_release.fabricApiVersion, m_release.minecraftVersion);
        QString enc = QString::fromUtf8(QUrl::toPercentEncoding(ver));
        QString filename = QString("fabric-api-%1.jar").arg(ver);
        setStatus(tr("Downloading %1…").arg(filename));
        QUrl url(QString("https://maven.fabricmc.net/net/fabricmc/fabric-api/fabric-api/%1/fabric-api-%2.jar").arg(enc, enc));
        if (!downloadFile(url, FS::PathCombine(modsDir, filename))) {
            setError(tr("Failed to download Fabric API %1.").arg(ver));
            return false;
        }
    }

    // fabric-language-kotlin-{kotlinFabric}.{kotlinVersion}.jar  e.g. 1.13.8+kotlin.2.3.0
    {
        QString ver = QString("%1.%2").arg(m_release.kotlinFabricVersion, m_release.kotlinVersion);
        QString enc = QString::fromUtf8(QUrl::toPercentEncoding(ver));
        QString filename = QString("fabric-language-kotlin-%1.jar").arg(ver);
        setStatus(tr("Downloading %1…").arg(filename));
        QUrl url(QString("https://maven.fabricmc.net/net/fabricmc/fabric-language-kotlin/%1/fabric-language-kotlin-%2.jar").arg(enc, enc));
        if (!downloadFile(url, FS::PathCombine(modsDir, filename))) {
            setError(tr("Failed to download Fabric Language Kotlin %1.").arg(ver));
            return false;
        }
    }

    const QString mc = m_release.minecraftVersion;
    if (mc.isEmpty()) {
        setError(tr("Cannot download Baritone: Minecraft version is unknown."));
        return false;
    }

    QString error;
    const QString destName = QString("baritone-fabric-%1.jar").arg(mc);
    if (!BaritoneMaven::downloadBaritoneForMinecraft(APPLICATION->network(), mc, FS::PathCombine(modsDir, destName), &error,
                                                     [this](const QString& status) { setStatus(status); })) {
        setError(error.isEmpty() ? tr("Failed to download Baritone for Minecraft %1.").arg(mc) : error);
        return false;
    }

    return true;
}

bool LambdaInstallTask::createInstance()
{
    if (m_release.tagName.isEmpty() || m_release.jarUrl.isEmpty()) {
        setError(tr("No Lambda release selected."));
        return false;
    }

    if (!fetchGradleProperties())
        return false;

    setStatus(tr("Creating Lambda instance for Minecraft %1…").arg(m_release.minecraftVersion));

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
