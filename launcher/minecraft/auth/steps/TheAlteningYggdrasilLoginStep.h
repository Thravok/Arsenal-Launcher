#pragma once

#include "minecraft/auth/AuthStep.h"
#include "net/NetJob.h"
#include "net/Request.h"

class TheAlteningYggdrasilLoginStep : public AuthStep {
    Q_OBJECT
   public:
    explicit TheAlteningYggdrasilLoginStep(AccountData* data, bool refresh);
    QString describe() override;
    void perform() override;

   private slots:
    void onRequestDone(QByteArray* response);

   private:
    bool m_refresh = false;
    Net::Request::Ptr m_request;
    NetJob::Ptr m_task;
};
