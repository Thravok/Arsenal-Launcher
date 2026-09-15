// SPDX-License-Identifier: GPL-3.0-only
#include "LiquidBounceProvider.h"

#include "net/Request.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <algorithm>

namespace HackClients {

static const QUrl LB_DOWNLOAD_PAGE("https://liquidbounce.net/download");
static const int CACHE_TTL_SECS = 60 * 60 * 6;  // 6h

LiquidBounceProvider::LiquidBounceProvider(QNetworkAccessManager* network, QObject* parent)
    : QObject(parent), m_network(network)
{}

void LiquidBounceProvider::refresh(bool force)
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
    m_candidateBuildIds.clear();
    m_builds.clear();
    m_latest = LiquidBounceBuild();

    m_job.reset(new NetJob("LiquidBounce download page", m_network));
    auto [pageAction, pageResponse] = Net::Request::makeByteArray(LB_DOWNLOAD_PAGE);
    m_job->addNetAction(pageAction);
    connect(m_job.get(), &NetJob::succeeded, this, [this, pageResponse] {
        m_response = *pageResponse;
        onDownloadPageFinished();
    });
    connect(m_job.get(), &NetJob::failed, this, &LiquidBounceProvider::onNetFailed);
    m_job->start();
}

void LiquidBounceProvider::onNetFailed(QString reason)
{
    m_job.reset();
    m_lastError = reason;
    emit failed(reason);
}

bool LiquidBounceProvider::parseDownloadPage(const QByteArray& html)
{
    // Collect /download/<id> candidates; prefer higher IDs (newer builds).
    static QRegularExpression idRe(R"(/download/(\d+))");
    QSet<int> ids;
    auto it = idRe.globalMatch(QString::fromUtf8(html));
    while (it.hasNext()) {
        auto m = it.next();
        ids.insert(m.captured(1).toInt());
    }
    if (ids.isEmpty())
        return false;

    m_candidateBuildIds = ids.values();
    std::sort(m_candidateBuildIds.begin(), m_candidateBuildIds.end(), std::greater<int>());
    return true;
}

void LiquidBounceProvider::onDownloadPageFinished()
{
    m_job.reset();
    if (!parseDownloadPage(m_response)) {
        m_lastError = tr("Could not find LiquidBounce builds on download page");
        emit failed(m_lastError);
        return;
    }
    // Fetch metadata for the newest candidate first; walk until we find a release.
    fetchBuildMeta(m_candidateBuildIds.first());
}

void LiquidBounceProvider::fetchBuildMeta(int buildId)
{
    m_response.clear();
    auto url = QUrl(QString("https://api.liquidbounce.net/api/v1/version/build/%1").arg(buildId));
    m_job.reset(new NetJob(QString("LiquidBounce build %1").arg(buildId), m_network));
    auto [metaAction, metaResponse] = Net::Request::makeByteArray(url);
    m_job->addNetAction(metaAction);
    connect(m_job.get(), &NetJob::succeeded, this, [this, metaResponse] {
        m_response = *metaResponse;
        onBuildMetaFinished();
    });
    connect(m_job.get(), &NetJob::failed, this, &LiquidBounceProvider::onNetFailed);
    m_job->start();
}

bool LiquidBounceProvider::parseBuildJson(const QByteArray& data, LiquidBounceBuild& out)
{
    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return false;
    auto obj = doc.object();
    out.buildId = obj.value("build_id").toInt();
    out.commitId = obj.value("commit_id").toString();
    out.branch = obj.value("branch").toString();
    out.lbVersion = obj.value("lb_version").toString();
    out.mcVersion = obj.value("mc_version").toString();
    out.release = obj.value("release").toBool(false);
    out.message = obj.value("message").toString();
    out.skipPid = obj.value("skip_pid").toString();
    out.jreVersion = obj.value("jre_version").toInt();
    out.fabricApiVersion = obj.value("fabric_api_version").toString();
    out.fabricLoaderVersion = obj.value("fabric_loader_version").toString();
    out.kotlinVersion = obj.value("kotlin_version").toString();
    out.kotlinModVersion = obj.value("kotlin_mod_version").toString();
    out.queueUrl = obj.value("url").toString();
    out.date = QDateTime::fromString(obj.value("date").toString(), Qt::ISODate);
    return out.buildId > 0 && !out.mcVersion.isEmpty();
}

void LiquidBounceProvider::onBuildMetaFinished()
{
    m_job.reset();
    LiquidBounceBuild build;
    if (!parseBuildJson(m_response, build)) {
        // Try next candidate build id
        if (!m_candidateBuildIds.isEmpty())
            m_candidateBuildIds.removeFirst();
        if (m_candidateBuildIds.isEmpty()) {
            m_lastError = tr("Failed to parse LiquidBounce build metadata");
            emit failed(m_lastError);
            return;
        }
        fetchBuildMeta(m_candidateBuildIds.first());
        return;
    }

    if (!build.release) {
        m_candidateBuildIds.removeAll(build.buildId);
        if (m_candidateBuildIds.isEmpty()) {
            m_lastError = tr("No LiquidBounce release builds found");
            emit failed(m_lastError);
            return;
        }
        fetchBuildMeta(m_candidateBuildIds.first());
        return;
    }

    m_latest = build;
    m_builds = { build };
    fetchQueuePage(build.buildId);
}

void LiquidBounceProvider::fetchQueuePage(int buildId)
{
    m_response.clear();
    auto url = QUrl(QString("https://liquidbounce.net/download/%1/queue").arg(buildId));
    m_job.reset(new NetJob(QString("LiquidBounce queue %1").arg(buildId), m_network));
    auto [queueAction, queueResponse] = Net::Request::makeByteArray(url);
    m_job->addNetAction(queueAction);
    connect(m_job.get(), &NetJob::succeeded, this, [this, queueResponse] {
        m_response = *queueResponse;
        onQueuePageFinished();
    });
    connect(m_job.get(), &NetJob::failed, this, &LiquidBounceProvider::onNetFailed);
    m_job->start();
}

bool LiquidBounceProvider::parseQueuePage(const QByteArray& html, LiquidBounceBuild& out)
{
    QString text = QString::fromUtf8(html);
    // React Router stream may escape quotes as \"
    text.replace(QStringLiteral("\\\""), QStringLiteral("\""));

    // Prefer the fileMetadata block: "pid","…","file_name","….zip",…,"file_checksum","…"
    static QRegularExpression blockRe(
        R"re("pid","([A-Za-z0-9]+)","file_name","([^"]+\.zip)","file_path","[^"]*","file_size",\d+,"file_checksum","([a-fA-F0-9]{64})")re");
    auto blockMatch = blockRe.match(text);
    if (blockMatch.hasMatch()) {
        out.filePid = blockMatch.captured(1);
        out.fileName = blockMatch.captured(2);
        out.fileChecksumSha256 = blockMatch.captured(3);
        return true;
    }

    static QRegularExpression pidRe(R"re("pid","([A-Za-z0-9]+)")re");
    static QRegularExpression nameRe(R"re("file_name","([^"]+\.zip)")re");
    static QRegularExpression sumRe(R"re("file_checksum","([a-fA-F0-9]{64})")re");

    auto pidMatch = pidRe.match(text);
    if (!pidMatch.hasMatch())
        return false;
    out.filePid = pidMatch.captured(1);

    auto nameMatch = nameRe.match(text);
    if (nameMatch.hasMatch())
        out.fileName = nameMatch.captured(1);

    auto sumMatch = sumRe.match(text);
    if (sumMatch.hasMatch())
        out.fileChecksumSha256 = sumMatch.captured(1);

    return !out.filePid.isEmpty();
}

void LiquidBounceProvider::onQueuePageFinished()
{
    m_job.reset();
    if (!parseQueuePage(m_response, m_latest)) {
        // Still usable without file pid if user opens download page manually, but install needs it.
        m_lastError = tr("Could not resolve LiquidBounce download file id from queue page");
        emit failed(m_lastError);
        return;
    }
    if (!m_builds.isEmpty())
        m_builds[0] = m_latest;

    m_loaded = true;
    m_fetchedAt = QDateTime::currentDateTimeUtc();
    emit refreshed();
}

}  // namespace HackClients
