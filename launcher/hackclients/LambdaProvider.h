// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "HackClientTypes.h"

#include "QObjectPtr.h"
#include "net/NetJob.h"

#include <QNetworkAccessManager>
#include <QObject>
#include <QDateTime>

namespace HackClients {

class LambdaProvider : public QObject {
    Q_OBJECT
   public:
    explicit LambdaProvider(QNetworkAccessManager* network, QObject* parent = nullptr);

    void refresh(bool force = false);
    bool isLoaded() const { return m_loaded; }
    QString lastError() const { return m_lastError; }

    QList<LambdaRelease> releases(bool includePrerelease = false) const;
    /** Latest stable release for each Minecraft version, newest MC first. */
    QList<LambdaRelease> latestStablePerMinecraft() const;
    LambdaRelease releaseForTag(const QString& tagName) const;

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
    QList<LambdaRelease> m_releases;
    bool m_loaded = false;
    QString m_lastError;
    QDateTime m_fetchedAt;
};

}  // namespace HackClients
