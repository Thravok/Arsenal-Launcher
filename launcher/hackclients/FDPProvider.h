// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "HackClientTypes.h"

#include "QObjectPtr.h"
#include "net/NetJob.h"

#include <QNetworkAccessManager>
#include <QObject>
#include <QDateTime>

namespace HackClients {

class FDPProvider : public QObject {
    Q_OBJECT
   public:
    explicit FDPProvider(QNetworkAccessManager* network, QObject* parent = nullptr);

    void refresh(bool force = false);
    bool isLoaded() const { return m_loaded; }
    QString lastError() const { return m_lastError; }

    QList<FDPRelease> releases(bool includePrerelease = false) const;
    FDPRelease releaseForTag(const QString& tagName) const;
    /** Newest stable release, or empty if none. */
    FDPRelease latestStable() const;

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
    QList<FDPRelease> m_releases;
    bool m_loaded = false;
    QString m_lastError;
    QDateTime m_fetchedAt;
};

}  // namespace HackClients
