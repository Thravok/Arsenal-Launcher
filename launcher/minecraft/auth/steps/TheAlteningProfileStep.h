#pragma once

#include "minecraft/auth/AuthStep.h"
#include "net/NetJob.h"
#include "net/Request.h"

class TheAlteningProfileStep : public AuthStep {
    Q_OBJECT
   public:
    explicit TheAlteningProfileStep(AccountData* data);
    QString describe() override;
    void perform() override;

    /**
     * Apply a sessionserver profile payload onto account data.
     * Preserves the auth UUID, prefers an unmasked Yggdrasil name, and strips '*' masks.
     * Returns true when the Mojang-format JSON parsed.
     */
    static bool applyFetchedProfile(AccountData* data, QByteArray response);

   private slots:
    void onRequestDone(QByteArray* response);

   private:
    void requestProfile(const QUrl& url);
    bool tryMojangProfileFallback();

    Net::Request::Ptr m_request;
    NetJob::Ptr m_task;
    /** Altening's session server often 404s; one Mojang retry fills skin URLs. */
    bool m_triedMojangFallback = false;
};
