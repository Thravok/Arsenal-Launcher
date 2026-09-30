// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "HackClientTypes.h"

#include "QObjectPtr.h"
#include "net/NetJob.h"

#include <QDateTime>
#include <QNetworkAccessManager>
#include <QObject>

namespace HackClients {

class EpsilonProvider : public QObject {
    Q_OBJECT
   public:
    explicit EpsilonProvider(QNetworkAccessManager* network, QObject* parent = nullptr);

    void refresh(bool force = false);
    bool isLoaded() const { return m_loaded; }
    QString lastError() const { return m_lastError; }

    QList<EpsilonRelease> releases(bool includePrerelease = false) const;
    /** Latest non-prerelease Fabric build per Minecraft version, newest MC first. */
    QList<EpsilonRelease> latestStablePerMinecraft() const;
    EpsilonRelease releaseForSelectionKey(const QString& key) const;

   signals:
    void refreshed();
    void failed(QString reason);

   private slots:
    void onDownloadSucceeded();
    void onDownloadFailed(QString reason);

   private:
    bool parse(const QByteArray& data);

    QNetworkAccessManager* m_network;
    NetJob::Ptr m_job;
    QByteArray m_response;
    QList<EpsilonRelease> m_releases;
    bool m_loaded = false;
    QString m_lastError;
    QDateTime m_fetchedAt;
};

}  // namespace HackClients
