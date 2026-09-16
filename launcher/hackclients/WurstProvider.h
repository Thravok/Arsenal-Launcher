// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "HackClientTypes.h"

#include "QObjectPtr.h"
#include "net/NetJob.h"

#include <QDateTime>
#include <QNetworkAccessManager>
#include <QObject>

namespace HackClients {

class WurstProvider : public QObject {
    Q_OBJECT
   public:
    explicit WurstProvider(QNetworkAccessManager* network, QObject* parent = nullptr);

    void refresh(bool force = false);
    bool isLoaded() const { return m_loaded; }
    QString lastError() const { return m_lastError; }

    QList<WurstRelease> releases(bool includePrerelease = false) const;
    /** Latest stable release for each Minecraft version, newest MC first. */
    QList<WurstRelease> latestStablePerMinecraft() const;
    WurstRelease releaseForTag(const QString& tagName) const;

    static bool isUnstableMinecraftVersion(const QString& mc);

    /** Parse a GitHub releases payload. Public so unit tests can cover catalog edge cases without the network. */
    bool parse(const QByteArray& data);

   signals:
    void refreshed();
    void failed(QString reason);

   private slots:
    void onDownloadSucceeded();
    void onDownloadFailed(QString reason);

   private:
    QNetworkAccessManager* m_network;
    NetJob::Ptr m_job;
    QByteArray m_response;
    QList<WurstRelease> m_releases;
    bool m_loaded = false;
    QString m_lastError;
    QDateTime m_fetchedAt;
};

}  // namespace HackClients
