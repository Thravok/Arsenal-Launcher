// SPDX-License-Identifier: GPL-3.0-only
#include "MeteorAddonsProvider.h"

#include "net/Request.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

namespace HackClients {

static const QUrl METEOR_ADDONS_CATALOG_URL(
    "https://raw.githubusercontent.com/cqb13/meteor-addon-scanner/refs/heads/addons/addons.json");
static const int CACHE_TTL_SECS = 60 * 60 * 6;

MeteorAddonsProvider::MeteorAddonsProvider(QNetworkAccessManager* network, QObject* parent)
    : QObject(parent), m_network(network)
{}

void MeteorAddonsProvider::refresh(bool force)
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
    m_job.reset(new NetJob("Meteor addons catalog", m_network));
    auto [action, response] = Net::Request::makeByteArray(METEOR_ADDONS_CATALOG_URL);
    m_job->addNetAction(action);
    connect(m_job.get(), &NetJob::succeeded, this, [this, response] {
        m_response = *response;
        onDownloadSucceeded();
    });
    connect(m_job.get(), &NetJob::failed, this, &MeteorAddonsProvider::onDownloadFailed);
    m_job->start();
}

void MeteorAddonsProvider::onDownloadSucceeded()
{
    m_job.reset();
    if (!parse(m_response)) {
        m_lastError = tr("Failed to parse Meteor addons catalog");
        emit failed(m_lastError);
        return;
    }
    m_loaded = true;
    m_fetchedAt = QDateTime::currentDateTimeUtc();
    emit refreshed();
}

void MeteorAddonsProvider::onDownloadFailed(QString reason)
{
    m_job.reset();
    m_lastError = reason;
    emit failed(reason);
}

bool MeteorAddonsProvider::parse(const QByteArray& data)
{
    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray())
        return false;

    QList<MeteorAddonEntry> out;
    for (const auto& value : doc.array()) {
        if (!value.isObject())
            continue;
        auto obj = value.toObject();

        MeteorAddonEntry entry;
        entry.name = obj.value("name").toString().trimmed();
        if (entry.name.isEmpty())
            continue;

        entry.description = obj.value("description").toString().trimmed();
        entry.minecraftVersion = obj.value("mc_version").toString().trimmed();
        entry.verified = obj.value("verified").toBool(false);

        for (const auto& authorVal : obj.value("authors").toArray()) {
            if (authorVal.isString())
                entry.authors.append(authorVal.toString());
        }

        if (auto custom = obj.value("custom").toObject(); !custom.isEmpty()) {
            if (entry.description.isEmpty())
                entry.description = custom.value("description").toString().trimmed();
            for (const auto& verVal : custom.value("supported_versions").toArray()) {
                if (verVal.isString())
                    entry.supportedVersions.append(verVal.toString());
            }
        }

        if (auto repo = obj.value("repo").toObject(); !repo.isEmpty()) {
            entry.repoOwner = repo.value("owner").toString();
            entry.repoName = repo.value("name").toString();
        }

        if (auto links = obj.value("links").toObject(); !links.isEmpty()) {
            entry.githubUrl = links.value("github").toString();
            entry.latestReleaseUrl = links.value("latest_release").toString();
            for (const auto& dlVal : links.value("downloads").toArray()) {
                if (dlVal.isString())
                    entry.downloadUrls.append(dlVal.toString());
            }
        }

        if (entry.pickJarDownloadUrl().isEmpty() && entry.githubUrl.isEmpty())
            continue;

        out.append(entry);
    }

    std::sort(out.begin(), out.end(), [](const MeteorAddonEntry& a, const MeteorAddonEntry& b) {
        if (a.verified != b.verified)
            return a.verified > b.verified;
        return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
    });

    m_addons = out;
    return true;
}

QList<MeteorAddonEntry> MeteorAddonsProvider::addonsForMinecraft(const QString& mcVersion,
                                                                 bool includeOtherVersions) const
{
    QList<MeteorAddonEntry> out;
    for (const auto& entry : m_addons) {
        if (!includeOtherVersions && !mcVersion.isEmpty() && !entry.supportsMinecraft(mcVersion))
            continue;
        if (entry.pickJarDownloadUrl().isEmpty())
            continue;
        out.append(entry);
    }
    return out;
}

}  // namespace HackClients
