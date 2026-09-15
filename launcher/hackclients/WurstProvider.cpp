// SPDX-License-Identifier: GPL-3.0-only
#include "WurstProvider.h"

#include "Version.h"
#include "net/Request.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QRegularExpression>
#include <algorithm>

namespace HackClients {

static const QUrl WURST_RELEASES_URL(
    "https://api.github.com/repos/Wurst-Imperium/Wurst-MCX2/releases?per_page=40");
static const int CACHE_TTL_SECS = 60 * 60 * 6;  // 6h

WurstProvider::WurstProvider(QNetworkAccessManager* network, QObject* parent)
    : QObject(parent), m_network(network)
{}

void WurstProvider::refresh(bool force)
{
    if (m_job)
        return;

    if (!force && m_loaded && m_fetchedAt.isValid() &&
        m_fetchedAt.secsTo(QDateTime::currentDateTimeUtc()) < CACHE_TTL_SECS) {
        emit refreshed();
        return;
    }

    m_response.clear();
    m_lastError.clear();
    m_job.reset(new NetJob("Wurst GitHub releases", m_network));
    auto [action, response] = Net::Request::makeByteArray(WURST_RELEASES_URL);
    m_job->addNetAction(action);
    connect(m_job.get(), &NetJob::succeeded, this, [this, response] {
        m_response = *response;
        onDownloadSucceeded();
    });
    connect(m_job.get(), &NetJob::failed, this, &WurstProvider::onDownloadFailed);
    m_job->start();
}

void WurstProvider::onDownloadSucceeded()
{
    m_job.reset();
    if (!parse(m_response)) {
        m_lastError = tr("Failed to parse Wurst releases from GitHub");
        emit failed(m_lastError);
        return;
    }
    m_loaded = true;
    m_fetchedAt = QDateTime::currentDateTimeUtc();
    emit refreshed();
}

void WurstProvider::onDownloadFailed(QString reason)
{
    m_job.reset();
    m_lastError = reason;
    emit failed(reason);
}

bool WurstProvider::isUnstableMinecraftVersion(const QString& mc)
{
    const QString lower = mc.toLower();
    return lower.contains(QStringLiteral("pre")) || lower.contains(QStringLiteral("rc")) ||
           lower.contains(QStringLiteral("snapshot"));
}

bool WurstProvider::parse(const QByteArray& data)
{
    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray())
        return false;

    static const QRegularExpression jarRe(
        QStringLiteral("^Wurst-Client-(.+)-MC(.+)\\.jar$"), QRegularExpression::CaseInsensitiveOption);

    QList<WurstRelease> out;
    for (const auto& value : doc.array()) {
        if (!value.isObject())
            continue;
        auto obj = value.toObject();

        for (const auto& assetVal : obj.value("assets").toArray()) {
            if (!assetVal.isObject())
                continue;
            auto asset = assetVal.toObject();
            QString name = asset.value("name").toString();
            if (!name.endsWith(".jar", Qt::CaseInsensitive))
                continue;
            if (name.endsWith("-sources.jar", Qt::CaseInsensitive))
                continue;
            if (!name.startsWith("Wurst-Client-", Qt::CaseInsensitive))
                continue;

            auto match = jarRe.match(name);
            if (!match.hasMatch())
                continue;

            WurstRelease rel;
            rel.wurstVersion = match.captured(1);
            rel.minecraftVersion = match.captured(2);
            rel.tagName = rel.wurstVersion + QStringLiteral("-MC") + rel.minecraftVersion;
            rel.jarName = name;
            rel.jarUrl = asset.value("browser_download_url").toString();
            rel.prerelease = isUnstableMinecraftVersion(rel.minecraftVersion);
            if (rel.jarUrl.isEmpty())
                continue;

            out.append(rel);
        }
    }

    m_releases = out;
    return !m_releases.isEmpty();
}

QList<WurstRelease> WurstProvider::releases(bool includePrerelease) const
{
    if (includePrerelease)
        return m_releases;

    QList<WurstRelease> stable;
    for (const auto& r : m_releases) {
        if (!r.prerelease)
            stable.append(r);
    }
    return stable;
}

QList<WurstRelease> WurstProvider::latestStablePerMinecraft() const
{
    QMap<QString, WurstRelease> best;
    for (const auto& r : m_releases) {
        if (r.prerelease)
            continue;
        auto it = best.find(r.minecraftVersion);
        if (it == best.end() || Version(r.wurstVersion) > Version(it->wurstVersion))
            best[r.minecraftVersion] = r;
    }

    QList<WurstRelease> list = best.values();
    std::sort(list.begin(), list.end(), [](const WurstRelease& a, const WurstRelease& b) {
        return Version(a.minecraftVersion) > Version(b.minecraftVersion);
    });
    return list;
}

WurstRelease WurstProvider::releaseForTag(const QString& tagName) const
{
    for (const auto& r : m_releases) {
        if (r.tagName == tagName)
            return r;
    }
    return WurstRelease();
}

}  // namespace HackClients
