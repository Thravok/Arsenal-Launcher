// SPDX-License-Identifier: GPL-3.0-only
#include "BaritoneMaven.h"

#include "FileSystem.h"
#include "Version.h"
#include "net/Request.h"
#include "net/NetJob.h"
#include "tasks/Task.h"

#include <QEventLoop>
#include <QFileInfo>
#include <QHash>
#include <QMap>
#include <QRegularExpression>
#include <QSet>
#include <QUrl>

namespace HackClients {
namespace BaritoneMaven {

namespace {

/** Minecraft version → Baritone release artifact version (cabaletta/baritone README). */
const QHash<QString, QString>& officialFabricBuildVersions()
{
    static const QHash<QString, QString> map = {
        {"1.16.5", "1.6.5"},
        {"1.17.1", "1.7.3"},
        {"1.18.2", "1.8.5"},
        {"1.19.2", "1.9.4"},
        {"1.19.3", "1.9.1"},
        {"1.19.4", "1.9.3"},
        {"1.20.1", "1.10.1"},
    };
    return map;
}

QString officialBaritoneArtifactVersion(const QString& minecraftVersion)
{
    return officialFabricBuildVersions().value(minecraftVersion);
}

QUrl officialStandaloneFabricUrl(const QString& baritoneArtifactVersion)
{
    return QUrl(QString("https://github.com/cabaletta/baritone/releases/download/v%1/baritone-standalone-fabric-%1.jar")
                    .arg(baritoneArtifactVersion));
}

}  // namespace

QList<BaritoneRelease> parseVersionIndex(const QByteArray& html)
{
    static const QRegularExpression dirRe(R"(href="\./([^"/]+)/")");
    QMap<QString, QString> byMc;
    auto it = dirRe.globalMatch(QString::fromUtf8(html));
    while (it.hasNext()) {
        const QString dir = it.next().captured(1).trimmed();
        if (dir.isEmpty() || dir.startsWith("maven-metadata"))
            continue;
        QString mc = dir;
        const bool snapshot = mc.endsWith("-SNAPSHOT");
        if (snapshot)
            mc.chop(QStringLiteral("-SNAPSHOT").size());
        if (mc.isEmpty())
            continue;
        const auto existing = byMc.find(mc);
        if (existing == byMc.end()) {
            byMc.insert(mc, dir);
            continue;
        }
        if (snapshot && !existing.value().endsWith("-SNAPSHOT"))
            *existing = dir;
    }

    QList<BaritoneRelease> out;
    for (auto mapIt = byMc.begin(); mapIt != byMc.end(); ++mapIt) {
        BaritoneRelease rel;
        rel.minecraftVersion = mapIt.key();
        rel.mavenVersion = mapIt.value();
        out.append(rel);
    }
    return out;
}

QString mavenVersionForMinecraft(const QList<BaritoneRelease>& releases, const QString& minecraftVersion)
{
    for (const auto& rel : releases) {
        if (rel.minecraftVersion == minecraftVersion)
            return rel.mavenVersion;
    }
    return {};
}

QList<BaritoneRelease> officialOnlyReleases()
{
    QList<BaritoneRelease> out;
    for (auto it = officialFabricBuildVersions().constBegin(); it != officialFabricBuildVersions().constEnd(); ++it) {
        BaritoneRelease rel;
        rel.minecraftVersion = it.key();
        rel.mavenVersion = QString();
        out.append(rel);
    }
    return out;
}

void appendOfficialOnlyReleases(QList<BaritoneRelease>& releases)
{
    QSet<QString> seen;
    for (const auto& rel : releases)
        seen.insert(rel.minecraftVersion);

    for (auto it = officialFabricBuildVersions().constBegin(); it != officialFabricBuildVersions().constEnd(); ++it) {
        if (seen.contains(it.key()))
            continue;
        BaritoneRelease rel;
        rel.minecraftVersion = it.key();
        rel.mavenVersion = QString();
        releases.append(rel);
        seen.insert(it.key());
    }
}

QString formatSupportedMinecraftVersions(const QList<BaritoneRelease>& releases, int maxShown)
{
    if (releases.isEmpty())
        return QObject::tr("(none listed)");

    QStringList versions;
    for (const auto& rel : releases)
        versions.append(rel.minecraftVersion);

    std::sort(versions.begin(), versions.end(), [](const QString& a, const QString& b) {
        return Version(a) > Version(b);
    });

    if (versions.size() <= maxShown)
        return versions.join(", ");

    QStringList head = versions.mid(0, maxShown);
    return QObject::tr("%1, … (%2 total)").arg(head.join(", ")).arg(versions.size());
}

static bool downloadByteArray(QNetworkAccessManager* network, const QUrl& url, QByteArray& out, QString* errorOut)
{
    out.clear();
    NetJob job(QString("Fetch %1").arg(url.fileName()), network);
    auto [action, response] = Net::Request::makeByteArray(url);
    job.addNetAction(action);

    QEventLoop loop;
    bool ok = false;
    QObject::connect(&job, &NetJob::succeeded, &loop, [&] {
        ok = true;
        out = *response;
    });
    QObject::connect(&job, &Task::finished, &loop, &QEventLoop::quit, Qt::QueuedConnection);
    job.start();
    loop.exec();

    if (!ok || out.isEmpty()) {
        if (errorOut)
            *errorOut = QObject::tr("Failed to fetch %1.").arg(url.toString());
        return false;
    }
    return true;
}

static bool downloadFile(QNetworkAccessManager* network, const QUrl& url, const QString& destPath, QString* errorOut)
{
    FS::ensureFilePathExists(destPath);
    NetJob job(QString("Download %1").arg(url.fileName()), network);
    job.addNetAction(Net::Request::makeFile(url, destPath));

    QEventLoop loop;
    bool ok = false;
    QObject::connect(&job, &NetJob::succeeded, &loop, [&] { ok = true; });
    QObject::connect(&job, &Task::finished, &loop, &QEventLoop::quit, Qt::QueuedConnection);
    job.start();
    loop.exec();

    if (!ok || !QFileInfo::exists(destPath)) {
        if (errorOut)
            *errorOut = QObject::tr("Failed to download %1.").arg(url.toString());
        return false;
    }
    return true;
}

bool fetchVersionIndex(QNetworkAccessManager* network, QByteArray& indexHtml, QString* errorOut)
{
    return downloadByteArray(network, QUrl(QLatin1String(INDEX_URL)), indexHtml, errorOut);
}

bool downloadBaritone(QNetworkAccessManager* network,
                      const QString& mavenVersion,
                      const QString& minecraftVersion,
                      const QString& destPath,
                      QString* errorOut,
                      const std::function<void(QString)>& setStatus)
{
    if (mavenVersion.isEmpty() || minecraftVersion.isEmpty()) {
        if (errorOut)
            *errorOut = QObject::tr("No Baritone build is available for Minecraft %1.").arg(minecraftVersion);
        return false;
    }

    const QString basePath =
        QString("%1%2").arg(QLatin1String(INDEX_URL), mavenVersion);

    if (setStatus)
        setStatus(QObject::tr("Resolving Baritone for Minecraft %1…").arg(minecraftVersion));

    QByteArray metadata;
    QString remoteJarName;
    if (downloadByteArray(network, QUrl(basePath + "/maven-metadata.xml"), metadata, nullptr)) {
        static const QRegularExpression jarSnapshotRe(
            R"(<extension>jar</extension>\s*<value>([^<]+)</value>)");
        const auto match = jarSnapshotRe.match(QString::fromUtf8(metadata));
        if (match.hasMatch())
            remoteJarName = QString("baritone-fabric-%1.jar").arg(match.captured(1).trimmed());
    }

    if (remoteJarName.isEmpty())
        remoteJarName = QString("baritone-fabric-%1.jar").arg(minecraftVersion);

    if (setStatus)
        setStatus(QObject::tr("Downloading %1…").arg(QFileInfo(destPath).fileName()));
    if (downloadFile(network, QUrl(basePath + "/" + remoteJarName), destPath, errorOut))
        return true;

    if (remoteJarName != QString("baritone-fabric-%1.jar").arg(mavenVersion)) {
        remoteJarName = QString("baritone-fabric-%1.jar").arg(mavenVersion);
        return downloadFile(network, QUrl(basePath + "/" + remoteJarName), destPath, errorOut);
    }
    return false;
}

static bool downloadOfficialBaritone(QNetworkAccessManager* network,
                                     const QString& minecraftVersion,
                                     const QString& destPath,
                                     QString* errorOut,
                                     const std::function<void(QString)>& setStatus)
{
    const QString artifactVersion = officialBaritoneArtifactVersion(minecraftVersion);
    if (artifactVersion.isEmpty())
        return false;

    const QUrl url = officialStandaloneFabricUrl(artifactVersion);
    if (setStatus) {
        setStatus(QObject::tr("Downloading official Baritone for Minecraft %1 from GitHub…").arg(minecraftVersion));
    }
    return downloadFile(network, url, destPath, errorOut);
}

bool downloadBaritoneForMinecraft(QNetworkAccessManager* network,
                                  const QString& minecraftVersion,
                                  const QString& destPath,
                                  QString* errorOut,
                                  const std::function<void(QString)>& setStatus)
{
    if (setStatus)
        setStatus(QObject::tr("Checking Baritone availability for Minecraft %1…").arg(minecraftVersion));

    QByteArray indexHtml;
    QString indexError;
    if (!fetchVersionIndex(network, indexHtml, &indexError)) {
        if (errorOut)
            *errorOut = indexError;
        return false;
    }

    const auto mavenReleases = parseVersionIndex(indexHtml);
    auto releases = mavenReleases;
    appendOfficialOnlyReleases(releases);

    const QString mavenVersion = mavenVersionForMinecraft(mavenReleases, minecraftVersion);
    if (!mavenVersion.isEmpty()) {
        if (downloadBaritone(network, mavenVersion, minecraftVersion, destPath, errorOut, setStatus))
            return true;
    }

    QString officialError;
    if (downloadOfficialBaritone(network, minecraftVersion, destPath, &officialError, setStatus))
        return true;

    if (errorOut) {
        *errorOut = QObject::tr("Could not download Baritone for Minecraft %1.\n\n%2\n\nSupported versions "
                                 "include: %3")
                        .arg(minecraftVersion,
                             officialError.isEmpty() ? QObject::tr("No rfresh2 Maven build and no official GitHub "
                                                                   "release mapping for this version.")
                                                     : officialError,
                             formatSupportedMinecraftVersions(releases));
    }
    return false;
}

}  // namespace BaritoneMaven
}  // namespace HackClients
