// SPDX-License-Identifier: GPL-3.0-only
#include "ImpactReleasesProvider.h"

#include "Version.h"
#include "net/Request.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QRegularExpression>
#include <algorithm>

namespace HackClients {

static const QUrl IMPACT_RELEASES_URL("http://impactclient.net/releases.json");
static const int CACHE_TTL_SECS = 60 * 60 * 24;  // 24h

ImpactReleasesProvider::ImpactReleasesProvider(QNetworkAccessManager* network, QObject* parent)
    : QObject(parent), m_network(network)
{}

void ImpactReleasesProvider::refresh(bool force)
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
    m_job.reset(new NetJob("Impact releases.json", m_network));
    auto [action, response] = Net::Request::makeByteArray(IMPACT_RELEASES_URL);
    m_job->addNetAction(action);
    connect(m_job.get(), &NetJob::succeeded, this, [this, response] {
        m_response = *response;
        onDownloadSucceeded();
    });
    connect(m_job.get(), &NetJob::failed, this, &ImpactReleasesProvider::onDownloadFailed);
    m_job->start();
}

void ImpactReleasesProvider::onDownloadSucceeded()
{
    m_job.reset();
    if (!parse(m_response)) {
        m_lastError = tr("Failed to parse Impact releases.json");
        emit failed(m_lastError);
        return;
    }
    m_loaded = true;
    m_fetchedAt = QDateTime::currentDateTimeUtc();
    emit refreshed();
}

void ImpactReleasesProvider::onDownloadFailed(QString reason)
{
    m_job.reset();
    m_lastError = reason;
    emit failed(reason);
}

bool ImpactReleasesProvider::parse(const QByteArray& data)
{
    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray())
        return false;

    QList<ImpactRelease> out;
    static QRegularExpression tagRe(R"(^(.+)-(\d+\.\d+(?:\.\d+)?)$)");

    for (const auto& value : doc.array()) {
        if (!value.isObject())
            continue;
        auto obj = value.toObject();
        ImpactRelease rel;
        rel.tagName = obj.value("tag_name").toString();
        rel.prerelease = obj.value("prerelease").toBool(false);
        if (rel.tagName.isEmpty())
            continue;

        auto match = tagRe.match(rel.tagName);
        if (!match.hasMatch())
            continue;
        rel.impactVersion = match.captured(1);
        rel.minecraftVersion = match.captured(2);
        out.append(rel);
    }

    m_releases = out;
    return !m_releases.isEmpty();
}

QList<ImpactRelease> ImpactReleasesProvider::releases(bool includePrerelease) const
{
    if (includePrerelease)
        return m_releases;

    QList<ImpactRelease> stable;
    for (const auto& r : m_releases) {
        if (!r.prerelease)
            stable.append(r);
    }
    return stable;
}

QList<ImpactRelease> ImpactReleasesProvider::latestStablePerMinecraft() const
{
    QMap<QString, ImpactRelease> best;
    for (const auto& r : m_releases) {
        if (r.prerelease)
            continue;
        auto it = best.find(r.minecraftVersion);
        if (it == best.end() || Version(r.impactVersion) > Version(it->impactVersion)) {
            best[r.minecraftVersion] = r;
        }
    }

    QList<ImpactRelease> list = best.values();
    std::sort(list.begin(), list.end(), [](const ImpactRelease& a, const ImpactRelease& b) {
        return Version(a.minecraftVersion) > Version(b.minecraftVersion);
    });
    return list;
}

ImpactRelease ImpactReleasesProvider::latestStableForMinecraft(const QString& mcVersion) const
{
    ImpactRelease best;
    for (const auto& r : m_releases) {
        if (r.prerelease || r.minecraftVersion != mcVersion)
            continue;
        if (best.tagName.isEmpty() || Version(r.impactVersion) > Version(best.impactVersion))
            best = r;
    }
    return best;
}

}  // namespace HackClients
