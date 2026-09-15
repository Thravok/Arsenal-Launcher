// SPDX-License-Identifier: GPL-3.0-only
#include "LambdaProvider.h"

#include "Version.h"
#include "net/Request.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <algorithm>

namespace HackClients {

static const QUrl LAMBDA_RELEASES_URL("https://api.github.com/repos/lambda-client/lambda/releases?per_page=40");
static const int CACHE_TTL_SECS = 60 * 60 * 6;  // 6h

LambdaProvider::LambdaProvider(QNetworkAccessManager* network, QObject* parent)
    : QObject(parent), m_network(network)
{}

void LambdaProvider::refresh(bool force)
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
    m_job.reset(new NetJob("Lambda GitHub releases", m_network));
    auto [action, response] = Net::Request::makeByteArray(LAMBDA_RELEASES_URL);
    m_job->addNetAction(action);
    connect(m_job.get(), &NetJob::succeeded, this, [this, response] {
        m_response = *response;
        onDownloadSucceeded();
    });
    connect(m_job.get(), &NetJob::failed, this, &LambdaProvider::onDownloadFailed);
    m_job->start();
}

void LambdaProvider::onDownloadSucceeded()
{
    m_job.reset();
    if (!parse(m_response)) {
        m_lastError = tr("Failed to parse Lambda releases from GitHub");
        emit failed(m_lastError);
        return;
    }
    m_loaded = true;
    m_fetchedAt = QDateTime::currentDateTimeUtc();
    emit refreshed();
}

void LambdaProvider::onDownloadFailed(QString reason)
{
    m_job.reset();
    m_lastError = reason;
    emit failed(reason);
}

bool LambdaProvider::parse(const QByteArray& data)
{
    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray())
        return false;

    QList<LambdaRelease> out;
    for (const auto& value : doc.array()) {
        if (!value.isObject())
            continue;
        auto obj = value.toObject();
        LambdaRelease rel;
        rel.tagName = obj.value("tag_name").toString();
        rel.prerelease = obj.value("prerelease").toBool(false);
        if (rel.tagName.isEmpty())
            continue;

        const int plus = rel.tagName.indexOf('+');
        if (plus <= 0 || plus + 1 >= rel.tagName.size())
            continue;
        rel.lambdaVersion = rel.tagName.left(plus);
        rel.minecraftVersion = rel.tagName.mid(plus + 1);

        for (const auto& assetVal : obj.value("assets").toArray()) {
            if (!assetVal.isObject())
                continue;
            auto asset = assetVal.toObject();
            QString name = asset.value("name").toString();
            if (!name.endsWith(".jar", Qt::CaseInsensitive))
                continue;
            if (!name.startsWith("lambda-", Qt::CaseInsensitive))
                continue;
            rel.jarName = name;
            rel.jarUrl = asset.value("browser_download_url").toString();
            break;
        }
        if (rel.jarUrl.isEmpty())
            continue;

        out.append(rel);
    }

    m_releases = out;
    return !m_releases.isEmpty();
}

QList<LambdaRelease> LambdaProvider::releases(bool includePrerelease) const
{
    if (includePrerelease)
        return m_releases;

    QList<LambdaRelease> stable;
    for (const auto& r : m_releases) {
        if (!r.prerelease)
            stable.append(r);
    }
    return stable;
}

QList<LambdaRelease> LambdaProvider::latestStablePerMinecraft() const
{
    QMap<QString, LambdaRelease> best;
    for (const auto& r : m_releases) {
        if (r.prerelease)
            continue;
        auto it = best.find(r.minecraftVersion);
        if (it == best.end() || Version(r.lambdaVersion) > Version(it->lambdaVersion))
            best[r.minecraftVersion] = r;
    }

    QList<LambdaRelease> list = best.values();
    std::sort(list.begin(), list.end(), [](const LambdaRelease& a, const LambdaRelease& b) {
        return Version(a.minecraftVersion) > Version(b.minecraftVersion);
    });
    return list;
}

LambdaRelease LambdaProvider::releaseForTag(const QString& tagName) const
{
    for (const auto& r : m_releases) {
        if (r.tagName == tagName)
            return r;
    }
    return LambdaRelease();
}

}  // namespace HackClients
