#include "TheAlteningGenerateStep.h"
#include "minecraft/auth/TheAlteningAuthParse.h"
#include "minecraft/auth/TheAlteningConfig.h"

#include <QJsonObject>

TheAlteningGenerateStep::TheAlteningGenerateStep(AccountData* data) : AuthStep(data)
{
    m_api.reset(new TheAlteningApi(this));
    connect(m_api.get(), &TheAlteningApi::failed, this, &TheAlteningGenerateStep::onApiFailed);
}

TheAlteningGenerateStep::~TheAlteningGenerateStep() noexcept = default;

QString TheAlteningGenerateStep::describe()
{
    return tr("Generating The Altening alt token.");
}

void TheAlteningGenerateStep::perform()
{
    if (m_data->theAlteningApiKey.isEmpty()) {
        m_data->theAlteningApiKey = TheAltening::storedApiKey();
    }
    if (m_data->theAlteningApiKey.isEmpty()) {
        emit finished(AccountTaskState::STATE_FAILED_HARD, tr("The Altening API key is missing. Set it in Settings → The Altening."));
        return;
    }

    m_awaitingLicense = true;
    disconnect(m_api.get(), &TheAlteningApi::succeeded, this, nullptr);
    connect(m_api.get(), &TheAlteningApi::succeeded, this, &TheAlteningGenerateStep::onLicenseSucceeded);
    m_api->checkLicense(m_data->theAlteningApiKey);
}

void TheAlteningGenerateStep::onLicenseSucceeded(QJsonObject json)
{
    m_awaitingLicense = false;
    QString error;
    if (!TheAlteningAuth::applyLicenseJson(m_data, json, &error)) {
        emit finished(AccountTaskState::STATE_FAILED_HARD, error);
        return;
    }
    startGenerate();
}

void TheAlteningGenerateStep::startGenerate()
{
    disconnect(m_api.get(), &TheAlteningApi::succeeded, this, nullptr);
    connect(m_api.get(), &TheAlteningApi::succeeded, this, &TheAlteningGenerateStep::onGenerateSucceeded);
    m_api->generate(m_data->theAlteningApiKey, true);
}

void TheAlteningGenerateStep::onGenerateSucceeded(QJsonObject json)
{
    const auto result = TheAlteningAuth::applyGenerateJson(m_data, json);
    if (!result.ok) {
        emit finished(AccountTaskState::STATE_FAILED_HARD, result.error);
        return;
    }

    QString message = tr("Generated The Altening alt token.");
    if (result.dailyLimit) {
        message = tr("Daily The Altening generate limit reached; reusing an alt from today.");
    }

    emit finished(AccountTaskState::STATE_WORKING, message);
}

void TheAlteningGenerateStep::onApiFailed(QString reason)
{
    Q_UNUSED(m_awaitingLicense);
    emit finished(AccountTaskState::STATE_FAILED_HARD, reason);
}
