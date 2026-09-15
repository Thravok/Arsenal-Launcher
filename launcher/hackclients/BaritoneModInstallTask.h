// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "tasks/Task.h"

#include "QObjectPtr.h"
#include "net/NetJob.h"

namespace HackClients {

/** Downloads Baritone into an existing instance's mods folder. */
class BaritoneModInstallTask final : public Task {
    Q_OBJECT
   public:
    BaritoneModInstallTask(QString modsDir, QString minecraftVersion, QString mavenVersion, QObject* parent = nullptr);

    bool abort() override;

   protected:
    void executeTask() override;

   private:
    QString m_modsDir;
    QString m_minecraftVersion;
    QString m_mavenVersion;
    NetJob::Ptr m_job;
    bool m_abort = false;
};

}  // namespace HackClients
