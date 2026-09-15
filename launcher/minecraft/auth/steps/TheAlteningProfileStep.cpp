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
    const QString authName = m_data->minecraftProfile.name;
    const QString profileId = m_data->minecraftProfile.id;
    const bool authNameUsable = !authName.isEmpty() && !authName.contains(QLatin1Char('*'));

    if (Parsers::parseMinecraftProfileMojang(*response, m_data->minecraftProfile)) {
        if (!profileId.isEmpty()) {
            m_data->minecraftProfile.id = profileId;
        }
        // Prefer a clean auth name; otherwise keep whatever the profile response provided
        // (Mojang fallback returns the real username).
        if (authNameUsable) {
            m_data->minecraftProfile.name = authName;
        } else if (m_data->minecraftProfile.name.contains(QLatin1Char('*'))) {
            QString cleaned = m_data->minecraftProfile.name;
            cleaned.remove(QLatin1Char('*'));
            if (!cleaned.isEmpty()) {
                m_data->minecraftProfile.name = cleaned;
            }
        }
        emit finished(AccountTaskState::STATE_WORKING, tr("Fetched profile."));
        return;
    }

    // Restore identity from auth if textures couldn't be parsed, then try Mojang once.
    m_data->minecraftProfile.id = profileId;
    if (authNameUsable) {
        m_data->minecraftProfile.name = authName;
    } else if (!authName.isEmpty()) {
        QString cleaned = authName;
        cleaned.remove(QLatin1Char('*'));
        m_data->minecraftProfile.name = cleaned;
    }
    if (tryMojangProfileFallback()) {
        return;
    }
    emit finished(AccountTaskState::STATE_WORKING, tr("The Altening profile ready."));
}
