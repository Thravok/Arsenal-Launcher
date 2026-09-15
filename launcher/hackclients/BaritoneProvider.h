// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "BaritoneMaven.h"

#include "QObjectPtr.h"
#include "net/NetJob.h"

#include <QNetworkAccessManager>
#include <QObject>
#include <QDateTime>

namespace HackClients {

class BaritoneProvider : public QObject {
    Q_OBJECT
   public:
    explicit BaritoneProvider(QNetworkAccessManager* network, QObject* parent = nullptr);

    void refresh(bool force = false);
    bool isLoaded() const { return m_loaded; }
    QString lastError() const { return m_lastError; }

    QList<BaritoneRelease> releases() const { return m_releases; }
    BaritoneRelease releaseForMinecraft(const QString& mcVersion) const;

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
    QList<BaritoneRelease> m_releases;
    bool m_loaded = false;
    QString m_lastError;
    QDateTime m_fetchedAt;
};

}  // namespace HackClients
