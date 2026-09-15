// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QString>
#include <QList>
#include <QDateTime>

namespace HackClients {

struct ImpactRelease {
    QString tagName;      // e.g. "4.9.1-1.16.5"
    QString impactVersion; // e.g. "4.9.1"
    QString minecraftVersion; // e.g. "1.16.5"
    bool prerelease = false;
    /** Shown as the instance version line (name is always "Impact"). */
    QString instanceVersionLabel() const
    {
        if (!impactVersion.isEmpty() && !minecraftVersion.isEmpty())
            return QString("%1 (%2)").arg(impactVersion, minecraftVersion);
        return tagName;
    }
};

struct LiquidBounceBuild {
    int buildId = 0;
    QString commitId;
    QString branch;
    QString lbVersion;
    QString mcVersion;
    bool release = false;
    QDateTime date;
    QString message;
    QString skipPid;
    int jreVersion = 0;
    QString fabricApiVersion;
    QString fabricLoaderVersion;
    QString kotlinVersion;
    QString kotlinModVersion;
    QString queueUrl;
    // Filled after resolving the download queue / file metadata
    QString filePid;
    QString fileName;
    QString fileChecksumSha256;
};

struct MeteorBuild {
    QString minecraftVersion;  // e.g. "1.21.8" or "26.2"
    int buildNumber = 0;       // from meteorclient.com/api/stats
    /** Shown as the instance version line (name is always "Meteor"). */
    QString instanceVersionLabel() const
    {
        if (buildNumber > 0)
            return QString("%1 · #%2").arg(minecraftVersion).arg(buildNumber);
        return minecraftVersion;
    }
};

struct FDPRelease {
    QString tagName;           // e.g. "b17"
    QString jarName;
    QString jarUrl;
    bool prerelease = false;
    QString minecraftVersion = QStringLiteral("1.8.9");
    /** Shown as the instance version line (name is always "FDPClient"). */
    QString instanceVersionLabel() const
    {
        if (!tagName.isEmpty())
            return QString("%1 (%2)").arg(tagName, minecraftVersion);
        return minecraftVersion;
    }
};

struct LambdaRelease {
    QString tagName;           // e.g. "0.1.0+1.21.11"
    QString lambdaVersion;     // e.g. "0.1.0"
    QString minecraftVersion;  // e.g. "1.21.11"
    /** Shown as the instance version line (name is always "Lambda"). */
    QString instanceVersionLabel() const
    {
        if (!lambdaVersion.isEmpty() && !minecraftVersion.isEmpty())
            return QString("%1 (%2)").arg(lambdaVersion, minecraftVersion);
        return tagName;
    }
    QString jarName;
    QString jarUrl;
    bool prerelease = false;
    // Filled during install from the tag's gradle.properties
    QString fabricLoaderVersion;
    QString fabricApiVersion;
    QString kotlinFabricVersion;  // e.g. "1.13.8+kotlin"
    QString kotlinVersion;        // e.g. "2.3.0"
    QString baritoneVersion;      // e.g. "1.14.0"
};

struct MeteorAddonEntry {
    QString name;
    QString description;
    QString minecraftVersion;
    QStringList supportedVersions;
    QStringList authors;
    bool verified = false;
    QString githubUrl;
    QString latestReleaseUrl;
    QStringList downloadUrls;
    QString repoOwner;
    QString repoName;

    bool supportsMinecraft(const QString& mcVersion) const
    {
        if (mcVersion.isEmpty())
            return true;
        if (minecraftVersion == mcVersion)
            return true;
        for (const auto& v : supportedVersions) {
            if (v == mcVersion)
                return true;
        }
        return false;
    }

    QString pickJarDownloadUrl() const
    {
        auto acceptable = [](const QString& url) {
            if (!url.endsWith(".jar", Qt::CaseInsensitive))
                return false;
            if (url.contains("-dev.jar", Qt::CaseInsensitive))
                return false;
            if (url.contains("-sources.jar", Qt::CaseInsensitive))
                return false;
            return true;
        };
        if (acceptable(latestReleaseUrl))
            return latestReleaseUrl;
        for (const auto& url : downloadUrls) {
            if (acceptable(url))
                return url;
        }
        return {};
    }
};

struct WurstRelease {
    QString tagName;           // e.g. "v7.55.1-MC26.2" (matches Wurst7 git tag)
    QString wurstVersion;      // e.g. "v7.55.1"
    QString minecraftVersion;  // e.g. "26.2"
    QString jarName;
    QString jarUrl;
    bool prerelease = false;
    // Filled during install from the tag's gradle.properties
    QString fabricLoaderVersion;
    QString fabricApiVersion;  // full maven id e.g. "0.158.0+26.2"
    /** Shown as the instance version line (name is always "Wurst"). */
    QString instanceVersionLabel() const
    {
        if (!wurstVersion.isEmpty() && !minecraftVersion.isEmpty())
            return QString("%1 (%2)").arg(wurstVersion, minecraftVersion);
        return tagName;
    }
};

}  // namespace HackClients
