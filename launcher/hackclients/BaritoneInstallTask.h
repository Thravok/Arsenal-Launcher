// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "InstanceCreationTask.h"
#include "hackclients/BaritoneMaven.h"

#include "QObjectPtr.h"
#include "net/NetJob.h"

namespace HackClients {

class BaritoneInstallTask final : public InstanceCreationTask {
    Q_OBJECT
   public:
    explicit BaritoneInstallTask(BaritoneRelease release);
    ~BaritoneInstallTask() override = default;

    bool abort() override;

   protected:
    bool createInstance() override;

   private:
    BaritoneRelease m_release;
    NetJob::Ptr m_job;
    bool m_abort = false;
};

}  // namespace HackClients
