#include "TheAlteningProfileStep.h"

#include <QDebug>
#include <QNetworkReply>
#include <QUrl>

#include "Application.h"
#include "minecraft/auth/Parsers.h"
#include "minecraft/auth/TheAlteningConfig.h"
#include "net/NetJob.h"
#include "net/NetUtils.h"
#include "net/Request.h"

TheAlteningProfileStep::TheAlteningProfileStep(AccountData* data) : AuthStep(data) {}

QString TheAlteningProfileStep::describe()
{
    return tr("Fetching the Minecraft profile.");
}

void TheAlteningProfileStep::requestProfile(const QUrl& url)
{
    auto [request, response] = Net::Request::makeByteArray(url);
    m_request = request;
    m_request->enableAutoRetry(true);

    m_task.reset(new NetJob("TheAlteningProfileStep", APPLICATION->network()));
    m_task->setAskRetry(false);
    m_task->addNetAction(m_request);
    connect(m_task.get(), &Task::finished, this, [this, response] { onRequestDone(response); });
    m_task->start();
}

bool TheAlteningProfileStep::tryMojangProfileFallback()
{
    if (m_triedMojangFallback || m_data->minecraftProfile.id.isEmpty()) {
        return false;
    }
    m_triedMojangFallback = true;
    qDebug() << "TheAlteningProfileStep: falling back to Mojang sessionserver for skin lookup";
    requestProfile(QUrl(QStringLiteral("https://sessionserver.mojang.com/session/minecraft/profile/") + m_data->minecraftProfile.id));
    return true;
}

void TheAlteningProfileStep::perform()
{
    if (m_data->minecraftProfile.id.isEmpty()) {
        emit finished(AccountTaskState::STATE_FAILED_HARD, tr("A UUID is required to get the profile."));
        return;
    }

    requestProfile(QUrl(TheAltening::SessionServerUrl + "/session/minecraft/profile/" + m_data->minecraftProfile.id));
}

void TheAlteningProfileStep::onRequestDone(QByteArray* response)
{
    if (m_request->error() == QNetworkReply::ContentNotFoundError) {
        // Altening's session server often 404s for valid alts. Mojang still has the real skin.
        if (tryMojangProfileFallback()) {
            return;
        }
        emit finished(AccountTaskState::STATE_WORKING, tr("Account has no Minecraft profile."));
        return;
    }
    if (m_request->error() != QNetworkReply::NoError) {
        qWarning() << "TheAlteningProfileStep failed:"
                    << "HTTP" << m_request->replyStatusCode()
                    << "error" << m_request->error()
                    << m_request->errorString();
        if (tryMojangProfileFallback()) {
            return;
        }
        if (Net::isApplicationError(m_request->error()) && !Net::isServerError(m_request->error())) {
            emit finished(AccountTaskState::STATE_FAILED_SOFT, tr("Failed to fetch The Altening profile."));
        } else {
            m_data->networkError = m_request->error();
            emit finished(AccountTaskState::STATE_OFFLINE, tr("Failed to fetch The Altening profile."));
        }
        return;
    }

    // Preserve the real username from Yggdrasil auth. The Altening generate API returns a
    // privacy-masked name (e.g. "BeanEater6942**") which is invalid for joining servers.
    if (applyFetchedProfile(m_data, *response)) {
        emit finished(AccountTaskState::STATE_WORKING, tr("Fetched profile."));
        return;
    }

    if (tryMojangProfileFallback()) {
        return;
    }
    emit finished(AccountTaskState::STATE_WORKING, tr("The Altening profile ready."));
}

bool TheAlteningProfileStep::applyFetchedProfile(AccountData* data, QByteArray response)
{
    if (!data) {
        return false;
    }

    const QString authName = data->minecraftProfile.name;
    const QString profileId = data->minecraftProfile.id;
    const bool authNameUsable = TheAltening::isUsableMinecraftUsername(authName);

    if (Parsers::parseMinecraftProfileMojang(response, data->minecraftProfile)) {
        if (!profileId.isEmpty()) {
            data->minecraftProfile.id = profileId;
        }
        if (authNameUsable) {
            data->minecraftProfile.name = authName;
        } else if (data->minecraftProfile.name.contains(QLatin1Char('*'))) {
            const QString cleaned = TheAltening::unmaskUsername(data->minecraftProfile.name);
            if (!cleaned.isEmpty()) {
                data->minecraftProfile.name = cleaned;
            }
        }
        return true;
    }

    data->minecraftProfile.id = profileId;
    if (authNameUsable) {
        data->minecraftProfile.name = authName;
    } else if (!authName.isEmpty()) {
        data->minecraftProfile.name = TheAltening::unmaskUsername(authName);
    }
    return false;
}
