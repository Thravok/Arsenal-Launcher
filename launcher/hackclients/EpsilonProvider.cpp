// SPDX-License-Identifier: GPL-3.0-only
#include "EpsilonProvider.h"

#include "Version.h"
#include "net/Request.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QRegularExpression>
#include <algorithm>

namespace HackClients {

static const QUrl EPSILON_RELEASES_URL("https://api.github.com/repos/NekoyaHouse/Epsilon/releases?per_page=40");
static const int CACHE_TTL_SECS = 60 * 60 * 6;  // 6h

static bool parseEpsilonFabricJar(const QString& name, QString* minecraftVersion, QString* epsilonVersion)
{
    // epsilon-fabric-26.3-2026.12.0.jar
    // epsilon-fabric-26.1.2-2026.8.0.jar
    // epsilon-fabric-26.3-2026.12.1-634e865.jar
    // open_epsilon-fabric-26.1.2-2026.3.0.jar
    static const QRegularExpression re(
        QStringLiteral(R"(^(?:open_)?epsilon-fabric-(.+)-(\d{4}\.\d+\.\d+(?:-[0-9a-fA-F]+)?)\.jar$)"),
        QRegularExpression::CaseInsensitiveOption);
    const auto match = re.match(name);
    if (!match.hasMatch())
        return false;
    *minecraftVersion = match.captured(1);
    *epsilonVersion = match.captured(2);
    return !minecraftVersion->isEmpty() && !epsilonVersion->isEmpty();
}

EpsilonProvider::EpsilonProvider(QNetworkAccessManager* network, QObject* parent)
    : QObject(parent), m_network(network)
{}

void EpsilonProvider::refresh(bool force)
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
    m_job.reset(new NetJob("Epsilon GitHub releases", m_network));
    auto [action, response] = Net::Request::makeByteArray(EPSILON_RELEASES_URL);
    m_job->addNetAction(action);
    connect(m_job.get(), &NetJob::succeeded, this, [this, response] {
        m_response = *response;
        onDownloadSucceeded();
    });
    connect(m_job.get(), &NetJob::failed, this, &EpsilonProvider::onDownloadFailed);
    m_job->start();
}

void EpsilonProvider::onDownloadSucceeded()
{
    m_job.reset();
    if (!parse(m_response)) {
        m_lastError = tr("Failed to parse Epsilon releases from GitHub");
        emit failed(m_lastError);
        return;
    }
    m_loaded = true;
    m_fetchedAt = QDateTime::currentDateTimeUtc();
    emit refreshed();
}

void EpsilonProvider::onDownloadFailed(QString reason)
{
    m_job.reset();
    m_lastError = reason;
    emit failed(reason);
}

bool EpsilonProvider::parse(const QByteArray& data)
{
    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray())
        return false;

    QList<EpsilonRelease> out;
    for (const auto& value : doc.array()) {
        if (!value.isObject())
            continue;
        auto obj = value.toObject();
        if (obj.value(QStringLiteral("draft")).toBool(false))
            continue;

        const QString tagName = obj.value(QStringLiteral("tag_name")).toString();
        if (tagName.isEmpty())
            continue;

        const bool prerelease =
            obj.value(QStringLiteral("prerelease")).toBool(false) ||
            tagName.compare(QStringLiteral("nightly"), Qt::CaseInsensitive) == 0;

        for (const auto& assetVal : obj.value(QStringLiteral("assets")).toArray()) {
            if (!assetVal.isObject())
                continue;
            auto asset = assetVal.toObject();
            const QString name = asset.value(QStringLiteral("name")).toString();
            if (!name.endsWith(QStringLiteral(".jar"), Qt::CaseInsensitive))
                continue;
            if (name.contains(QStringLiteral("neoforge"), Qt::CaseInsensitive))
                continue;
            if (name.contains(QStringLiteral("-sources"), Qt::CaseInsensitive))
                continue;
            if (name.contains(QStringLiteral("syink"), Qt::CaseInsensitive))
                continue;

            QString mc;
            QString epsilonVer;
            if (!parseEpsilonFabricJar(name, &mc, &epsilonVer))
                continue;

            EpsilonRelease rel;
            rel.tagName = tagName;
            rel.epsilonVersion = epsilonVer;
            rel.minecraftVersion = mc;
            rel.jarName = name;
            rel.jarUrl = asset.value(QStringLiteral("browser_download_url")).toString();
            rel.prerelease = prerelease;
            if (rel.jarUrl.isEmpty())
                continue;
            out.append(rel);
        }
    }

    m_releases = out;
    return !m_releases.isEmpty();
}

QList<EpsilonRelease> EpsilonProvider::releases(bool includePrerelease) const
{
    if (includePrerelease)
        return m_releases;

    QList<EpsilonRelease> stable;
    for (const auto& r : m_releases) {
        if (!r.prerelease)
            stable.append(r);
    }
    return stable;
}

QList<EpsilonRelease> EpsilonProvider::latestStablePerMinecraft() const
{
    QMap<QString, EpsilonRelease> best;
    for (const auto& r : m_releases) {
        if (r.prerelease)
            continue;
        auto it = best.find(r.minecraftVersion);
        if (it == best.end() || Version(r.epsilonVersion) > Version(it->epsilonVersion))
            best[r.minecraftVersion] = r;
    }

    QList<EpsilonRelease> list = best.values();
    std::sort(list.begin(), list.end(), [](const EpsilonRelease& a, const EpsilonRelease& b) {
        return Version(a.minecraftVersion) > Version(b.minecraftVersion);
    });
    return list;
}

EpsilonRelease EpsilonProvider::releaseForSelectionKey(const QString& key) const
{
    for (const auto& r : m_releases) {
        if (r.selectionKey() == key)
            return r;
    }
    return EpsilonRelease();
}

}  // namespace HackClients
