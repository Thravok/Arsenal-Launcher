// SPDX-License-Identifier: GPL-3.0-only
#include "MeteorProvider.h"

#include "Version.h"
#include "net/Request.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

namespace HackClients {

static const QUrl METEOR_STATS_URL("https://meteorclient.com/api/stats");
static const int CACHE_TTL_SECS = 60 * 60 * 6;  // 6h

MeteorProvider::MeteorProvider(QNetworkAccessManager* network, QObject* parent)
    : QObject(parent), m_network(network)
{}

void MeteorProvider::refresh(bool force)
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
    m_job.reset(new NetJob("Meteor stats", m_network));
    auto [action, response] = Net::Request::makeByteArray(METEOR_STATS_URL);
    m_job->addNetAction(action);
    connect(m_job.get(), &NetJob::succeeded, this, [this, response] {
        m_response = *response;
        onDownloadSucceeded();
    });
    connect(m_job.get(), &NetJob::failed, this, &MeteorProvider::onDownloadFailed);
    m_job->start();
}

void MeteorProvider::onDownloadSucceeded()
{
    m_job.reset();
    if (!parse(m_response)) {
        m_lastError = tr("Failed to parse Meteor stats from meteorclient.com");
        emit failed(m_lastError);
        return;
    }
    m_loaded = true;
    m_fetchedAt = QDateTime::currentDateTimeUtc();
    emit refreshed();
}

void MeteorProvider::onDownloadFailed(QString reason)
{
    m_job.reset();
    m_lastError = reason;
    emit failed(reason);
}

bool MeteorProvider::parse(const QByteArray& data)
{
    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return false;

    auto buildsObj = doc.object().value("builds").toObject();
    if (buildsObj.isEmpty())
        return false;

    QList<MeteorBuild> out;
    for (auto it = buildsObj.begin(); it != buildsObj.end(); ++it) {
        MeteorBuild build;
        build.minecraftVersion = it.key();
        build.buildNumber = it.value().toInt(0);
        if (build.minecraftVersion.isEmpty() || build.buildNumber <= 0)
            continue;
        out.append(build);
    }

    if (out.isEmpty())
        return false;

    std::sort(out.begin(), out.end(), [](const MeteorBuild& a, const MeteorBuild& b) {
        return Version(a.minecraftVersion) > Version(b.minecraftVersion);
    });

    m_builds = out;
    return true;
}

MeteorBuild MeteorProvider::buildForMinecraft(const QString& mcVersion) const
{
    for (const auto& b : m_builds) {
        if (b.minecraftVersion == mcVersion)
            return b;
    }
    return MeteorBuild();
}

}  // namespace HackClients
