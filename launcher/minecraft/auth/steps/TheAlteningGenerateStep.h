#pragma once

#include "minecraft/auth/AuthStep.h"
#include "minecraft/auth/TheAlteningApi.h"

class TheAlteningGenerateStep : public AuthStep {
    Q_OBJECT
   public:
    explicit TheAlteningGenerateStep(AccountData* data);
    virtual ~TheAlteningGenerateStep() noexcept;

    void perform() override;
    QString describe() override;

   private slots:
    void onLicenseSucceeded(QJsonObject json);
    void onGenerateSucceeded(QJsonObject json);
    void onApiFailed(QString reason);

   private:
    void startGenerate();

    TheAlteningApi::Ptr m_api;
    bool m_awaitingLicense = false;
};
