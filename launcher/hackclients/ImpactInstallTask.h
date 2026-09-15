// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "InstanceCreationTask.h"
#include "hackclients/HackClientTypes.h"

#include "QObjectPtr.h"
#include "net/NetJob.h"

#include <QProcess>

namespace HackClients {

class ImpactInstallTask final : public InstanceCreationTask {
    Q_OBJECT
   public:
    explicit ImpactInstallTask(ImpactRelease release);
    ~ImpactInstallTask() override;

    bool abort() override;

   protected:
    bool createInstance() override;

   private:
    bool ensureInstallerJar(QString& outPath);
    bool runInstaller(const QString& installerJar, const QString& mmcRoot);
    bool locateInstalledInstance(const QString& mmcRoot, QString& outInstancePath);
    bool copyIntoStaging(const QString& sourceInstancePath);

    ImpactRelease m_release;
    NetJob::Ptr m_job;
    QProcess* m_process = nullptr;
    QString m_installerLog;
};

}  // namespace HackClients
