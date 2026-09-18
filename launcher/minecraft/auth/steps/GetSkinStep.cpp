#include "GetSkinStep.h"

#include <QNetworkReply>
#include <QNetworkRequest>

#include "Application.h"
#include "minecraft/auth/TheAlteningConfig.h"

GetSkinStep::GetSkinStep(AccountData* data) : AuthStep(data) {}

QString GetSkinStep::describe()
{
    return tr("Getting skin.");
}

void GetSkinStep::perform()
{
    QString urlString = m_data->minecraftProfile.skin.url;

    // Prefer real Mojang texture URLs from the profile step. For Altening, replace empty /
    // default / body-CDN placeholders with the head CDN when we have a skin id.
    if (m_data->type == AccountType::TheAltening) {
        const auto skinId = m_data->yggdrasilToken.extra.value(QStringLiteral("alteningSkin")).toString();
        const QString resolved = TheAltening::resolveAlteningSkinDownloadUrl(urlString, skinId);
        if (resolved != urlString) {
            m_data->minecraftProfile.skin.url = resolved;
        }
        urlString = resolved;
    }

    if (urlString.isEmpty()) {
        emit finished(AccountTaskState::STATE_WORKING, tr("Got skin"));
        return;
    }
    QUrl url(urlString);

    auto [request, response] = Net::Request::makeByteArray(url);
    m_request = request;
    m_request->enableAutoRetry(true);

    m_task.reset(new NetJob("GetSkinStep", APPLICATION->network()));
    m_task->setAskRetry(false);
    m_task->addNetAction(m_request);

    connect(m_task.get(), &Task::finished, this, [this, response] { onRequestDone(response); });

    m_task->start();
}

void GetSkinStep::onRequestDone(QByteArray* response)
{
    if (m_request->error() == QNetworkReply::NoError)
        m_data->minecraftProfile.skin.data = *response;
    emit finished(AccountTaskState::STATE_WORKING, tr("Got skin"));
}
