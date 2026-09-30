// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "InstanceCreationTask.h"
#include "hackclients/HackClientTypes.h"

#include "QObjectPtr.h"
#include "net/NetJob.h"

namespace HackClients {

class EpsilonInstallTask final : public InstanceCreationTask {
    Q_OBJECT
   public:
    explicit EpsilonInstallTask(EpsilonRelease release);
    ~EpsilonInstallTask() override = default;

    bool abort() override;

   protected:
    bool createInstance() override;

   private:
    bool downloadFile(const QUrl& url, const QString& destPath);
    bool downloadFabricApi(const QString& modsDir);
    bool downloadMods(const QString& modsDir);

    EpsilonRelease m_release;
    NetJob::Ptr m_job;
};

}  // namespace HackClients
