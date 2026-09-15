// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "InstanceCreationTask.h"
#include "hackclients/HackClientTypes.h"

#include "QObjectPtr.h"
#include "net/NetJob.h"

namespace HackClients {

class LambdaInstallTask final : public InstanceCreationTask {
    Q_OBJECT
   public:
    explicit LambdaInstallTask(LambdaRelease release);
    ~LambdaInstallTask() override = default;

    bool abort() override;

   protected:
    bool createInstance() override;

   private:
    bool downloadFile(const QUrl& url, const QString& destPath);
    bool downloadByteArray(const QUrl& url, QByteArray& out);
    bool fetchGradleProperties();
    bool downloadMods(const QString& modsDir);

    LambdaRelease m_release;
    NetJob::Ptr m_job;
};

}  // namespace HackClients
