// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "QObjectPtr.h"

#include <QList>
#include <QNetworkAccessManager>
#include <QRegularExpression>
#include <QString>

#include <functional>

namespace HackClients {

struct BaritoneRelease {
    QString minecraftVersion;
    /** Maven directory name, e.g. `1.21.11-SNAPSHOT` or `1.20.1`. */
    QString mavenVersion;
};

namespace BaritoneMaven {

static const char* const INDEX_URL = "https://maven.2b2t.vc/releases/com/github/rfresh2/baritone-fabric/";

QList<BaritoneRelease> parseVersionIndex(const QByteArray& html);

/** Prefer `{minecraftVersion}-SNAPSHOT`, then `{minecraftVersion}`. */
QString mavenVersionForMinecraft(const QList<BaritoneRelease>& releases, const QString& minecraftVersion);

/** `baritone-fabric-<snapshotValue>.jar` from maven-metadata.xml, else `baritone-fabric-<minecraftVersion>.jar`. */
inline QString fabricJarNameFromMetadata(const QByteArray& xml, const QString& minecraftVersion)
{
    static const QRegularExpression jarSnapshotRe(QStringLiteral(R"(<extension>jar</extension>\s*<value>([^<]+)</value>)"));
    const auto match = jarSnapshotRe.match(QString::fromUtf8(xml));
    if (match.hasMatch()) {
        const QString snapshotValue = match.captured(1).trimmed();
        if (!snapshotValue.isEmpty()) {
            return QStringLiteral("baritone-fabric-%1.jar").arg(snapshotValue);
        }
    }
    return QStringLiteral("baritone-fabric-%1.jar").arg(minecraftVersion);
}

/** Human-readable list of Minecraft versions that have Baritone builds on the Maven index. */
QString formatSupportedMinecraftVersions(const QList<BaritoneRelease>& releases, int maxShown = 12);

/** Minecraft versions with official cabaletta/baritone Fabric standalone builds (not on rfresh2 Maven). */
QList<BaritoneRelease> officialOnlyReleases();

/** Adds official-only releases when missing from a Maven-derived list. */
void appendOfficialOnlyReleases(QList<BaritoneRelease>& releases);

bool fetchVersionIndex(QNetworkAccessManager* network, QByteArray& indexHtml, QString* errorOut);

bool downloadBaritone(QNetworkAccessManager* network,
                      const QString& mavenVersion,
                      const QString& minecraftVersion,
                      const QString& destPath,
                      QString* errorOut,
                      const std::function<void(QString)>& setStatus = {});

/** Tries `{minecraftVersion}-SNAPSHOT` then `{minecraftVersion}`. */
bool downloadBaritoneForMinecraft(QNetworkAccessManager* network,
                                  const QString& minecraftVersion,
                                  const QString& destPath,
                                  QString* errorOut,
                                  const std::function<void(QString)>& setStatus = {});

}  // namespace BaritoneMaven
}  // namespace HackClients
