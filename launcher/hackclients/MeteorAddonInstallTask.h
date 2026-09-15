// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "tasks/Task.h"

#include <QUrl>

#include "QObjectPtr.h"
#include "net/NetJob.h"

namespace HackClients {

/** Downloads a Meteor addon JAR into an existing instance's mods folder. */
class MeteorAddonInstallTask final : public Task {
    Q_OBJECT
   public:
    MeteorAddonInstallTask(QString modsDir, QUrl downloadUrl, QString destFileName, QObject* parent = nullptr);

    bool abort() override;

   protected:
    void executeTask() override;

   private:
    QString m_modsDir;
    QUrl m_downloadUrl;
    QString m_destFileName;
    NetJob::Ptr m_job;
    bool m_abort = false;
};

}  // namespace HackClients
