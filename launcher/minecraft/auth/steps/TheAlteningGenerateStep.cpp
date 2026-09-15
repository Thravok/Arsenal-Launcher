#include "TheAlteningGenerateStep.h"
#include "minecraft/auth/TheAlteningConfig.h"

TheAlteningGenerateStep::TheAlteningGenerateStep(AccountData *data) : AuthStep(data)
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
    if (!json.value(QStringLiteral("hasLicense")).toBool(false)) {
        emit finished(AccountTaskState::STATE_FAILED_HARD, tr("The Altening API key has no active license."));
        return;
    }

    m_data->theAlteningLicenseType = json.value(QStringLiteral("licenseType")).toString();
    m_data->theAlteningLicenseExpires = json.value(QStringLiteral("expires")).toString();
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
    const QString token = json.value(QStringLiteral("token")).toString();
    const QString password = json.value(QStringLiteral("password")).toString(QStringLiteral("anything"));
    if (token.isEmpty()) {
        emit finished(AccountTaskState::STATE_FAILED_HARD, tr("The Altening API did not return an alt token."));
        return;
    }

    m_data->yggdrasilToken.extra[QStringLiteral("userName")] = token;
    m_data->theAlteningPendingPassword = password;

    const QString altUsername = json.value(QStringLiteral("username")).toString();
    if (!altUsername.isEmpty()) {
        m_data->theAlteningAltUsername = altUsername;
    }

    const QString skin = json.value(QStringLiteral("skin")).toString();
    if (!skin.isEmpty()) {
        m_data->yggdrasilToken.extra[QStringLiteral("alteningSkin")] = skin;
    }

    QString message = tr("Generated The Altening alt token.");
    if (json.value(QStringLiteral("limit")).toBool(false)) {
        message = tr("Daily The Altening generate limit reached; reusing an alt from today.");
    }

    emit finished(AccountTaskState::STATE_WORKING, message);
}

void TheAlteningGenerateStep::onApiFailed(QString reason)
{
    Q_UNUSED(m_awaitingLicense);
    emit finished(AccountTaskState::STATE_FAILED_HARD, reason);
}
