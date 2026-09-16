// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "hackclients/HackClientTypes.h"

#include <QDateTime>
#include <QList>
#include <QNetworkAccessManager>
#include <QObject>

#include "QObjectPtr.h"
#include "net/NetJob.h"

namespace HackClients {

class MeteorAddonsProvider : public QObject {
    Q_OBJECT
   public:
    explicit MeteorAddonsProvider(QNetworkAccessManager* network, QObject* parent = nullptr);

    void refresh(bool force = false);

    QList<MeteorAddonEntry> addons() const { return m_addons; }
    QList<MeteorAddonEntry> addonsForMinecraft(const QString& mcVersion, bool includeOtherVersions = false) const;
    QString lastError() const { return m_lastError; }
    bool isLoaded() const { return m_loaded; }

    /** Parse a catalog payload. Public so unit tests can cover catalog edge cases without the network. */
    bool parse(const QByteArray& data);

   signals:
    void refreshed();
    void failed(QString reason);

   private slots:
    void onDownloadSucceeded();
    void onDownloadFailed(QString reason);

   private:
    QNetworkAccessManager* m_network = nullptr;
    NetJob::Ptr m_job;
    QByteArray m_response;
    QList<MeteorAddonEntry> m_addons;
    QString m_lastError;
    bool m_loaded = false;
    QDateTime m_fetchedAt;
};

}  // namespace HackClients
