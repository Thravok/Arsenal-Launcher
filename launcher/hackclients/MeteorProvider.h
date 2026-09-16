// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "HackClientTypes.h"

#include "QObjectPtr.h"
#include "net/NetJob.h"

#include <QDateTime>
#include <QNetworkAccessManager>
#include <QObject>

namespace HackClients {

class MeteorProvider : public QObject {
    Q_OBJECT
   public:
    explicit MeteorProvider(QNetworkAccessManager* network, QObject* parent = nullptr);

    void refresh(bool force = false);
    bool isLoaded() const { return m_loaded; }
    QString lastError() const { return m_lastError; }

    /** Builds sorted by Minecraft version descending (newest first). */
    QList<MeteorBuild> builds() const { return m_builds; }
    MeteorBuild buildForMinecraft(const QString& mcVersion) const;

    /** Parse a stats payload. Public so unit tests can cover catalog edge cases without the network. */
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
    QList<MeteorBuild> m_builds;
    bool m_loaded = false;
    QString m_lastError;
    QDateTime m_fetchedAt;
};

}  // namespace HackClients
