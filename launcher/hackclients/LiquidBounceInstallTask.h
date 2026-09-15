// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "InstanceCreationTask.h"
#include "hackclients/HackClientTypes.h"

#include "QObjectPtr.h"
#include "net/NetJob.h"

namespace HackClients {

class LiquidBounceInstallTask final : public InstanceCreationTask {
    Q_OBJECT
   public:
    explicit LiquidBounceInstallTask(LiquidBounceBuild build);
    ~LiquidBounceInstallTask() override = default;

    bool abort() override;

   protected:
    bool createInstance() override;

   private:
    bool downloadFile(const QUrl& url, const QString& destPath, const QByteArray& sha256 = QByteArray());
    bool downloadAndExtractClient(const QString& modsDir);
    bool downloadCompanionMods(const QString& modsDir);

    LiquidBounceBuild m_build;
    NetJob::Ptr m_job;
};

}  // namespace HackClients
