// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "HackClientTypes.h"

#include "QObjectPtr.h"
#include "net/NetJob.h"

#include <QNetworkAccessManager>
#include <QObject>
#include <QDateTime>

namespace HackClients {

class LiquidBounceProvider : public QObject {
    Q_OBJECT
   public:
    explicit LiquidBounceProvider(QNetworkAccessManager* network, QObject* parent = nullptr);

    void refresh(bool force = false);
    bool isLoaded() const { return m_loaded; }
    QString lastError() const { return m_lastError; }

    /** Latest release build, with filePid/checksum resolved when possible. */
    LiquidBounceBuild latestRelease() const { return m_latest; }
    QList<LiquidBounceBuild> releaseBuilds() const { return m_builds; }

   signals:
    void refreshed();
    void failed(QString reason);

   private slots:
    void onDownloadPageFinished();
    void onBuildMetaFinished();
    void onQueuePageFinished();
    void onNetFailed(QString reason);

   private:
    bool parseDownloadPage(const QByteArray& html);
    bool parseBuildJson(const QByteArray& data, LiquidBounceBuild& out);
    bool parseQueuePage(const QByteArray& html, LiquidBounceBuild& out);
    void fetchBuildMeta(int buildId);
    void fetchQueuePage(int buildId);

    QNetworkAccessManager* m_network;
    NetJob::Ptr m_job;
    QByteArray m_response;
    QList<int> m_candidateBuildIds;
    QList<LiquidBounceBuild> m_builds;
    LiquidBounceBuild m_latest;
    bool m_loaded = false;
    QString m_lastError;
    QDateTime m_fetchedAt;
};

}  // namespace HackClients
