// SPDX-License-Identifier: GPL-3.0-only
#include "BaritoneProvider.h"

#include "Version.h"
#include "net/Request.h"

#include <QUrl>
#include <algorithm>

namespace HackClients {

static const int CACHE_TTL_SECS = 60 * 60 * 6;

BaritoneProvider::BaritoneProvider(QNetworkAccessManager* network, QObject* parent)
    : QObject(parent), m_network(network)
{}

void BaritoneProvider::refresh(bool force)
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
    m_job.reset(new NetJob("Baritone index", m_network));
    auto [action, response] = Net::Request::makeByteArray(QUrl(QLatin1String(BaritoneMaven::INDEX_URL)));
    m_job->addNetAction(action);
    connect(m_job.get(), &NetJob::succeeded, this, [this, response] {
        m_response = *response;
        onDownloadSucceeded();
    });
    connect(m_job.get(), &NetJob::failed, this, &BaritoneProvider::onDownloadFailed);
    m_job->start();
}

void BaritoneProvider::onDownloadSucceeded()
{
    m_job.reset();
    if (!parse(m_response)) {
        m_lastError = tr("Failed to parse Baritone version index from maven.2b2t.vc");
        emit failed(m_lastError);
        return;
    }
    m_loaded = true;
    m_fetchedAt = QDateTime::currentDateTimeUtc();
    emit refreshed();
}

void BaritoneProvider::onDownloadFailed(QString reason)
{
    m_job.reset();
    m_lastError = reason;
    emit failed(reason);
}

bool BaritoneProvider::parse(const QByteArray& data)
{
    auto out = BaritoneMaven::parseVersionIndex(data);
    if (out.isEmpty())
        return false;

    BaritoneMaven::appendOfficialOnlyReleases(out);

    std::sort(out.begin(), out.end(), [](const BaritoneRelease& a, const BaritoneRelease& b) {
        return Version(a.minecraftVersion) > Version(b.minecraftVersion);
    });

    m_releases = out;
    return true;
}

BaritoneRelease BaritoneProvider::releaseForMinecraft(const QString& mcVersion) const
{
    for (const auto& rel : m_releases) {
        if (rel.minecraftVersion == mcVersion)
            return rel;
    }
    return BaritoneRelease();
}

}  // namespace HackClients
