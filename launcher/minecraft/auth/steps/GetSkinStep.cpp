#include "GetSkinStep.h"

#include <QNetworkReply>
#include <QNetworkRequest>

#include "Application.h"
#include "minecraft/auth/TheAlteningConfig.h"

namespace {

bool isMhfDefaultSkinUrl(const QString& url)
{
    // MHF_Steve / MHF_Alex placeholders from Parsers::parseMinecraftProfileMojang
    return url.contains(QStringLiteral("1a4af718455d4aab528e7a61f86fa25e6a369d1768dcb13f7df319a713eb810b"))
           || url.contains(QStringLiteral("83cee5ca6afcdb171285aa00e8049c297b2dbeba0efb8ff970a5677a1b644032"));
}

}  // namespace

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
        const bool usableMojangTexture =
            urlString.contains(QStringLiteral("textures.minecraft.net")) && !isMhfDefaultSkinUrl(urlString);
        if (!usableMojangTexture) {
            const auto skinId = m_data->yggdrasilToken.extra.value(QStringLiteral("alteningSkin")).toString();
            if (!skinId.isEmpty()) {
                urlString = TheAltening::skinCdnHeadUrl(skinId);
                m_data->minecraftProfile.skin.url = urlString;
            } else if (isMhfDefaultSkinUrl(urlString) || urlString.contains(QStringLiteral("cdn.thealtening.com/skins/body/"))) {
                urlString.clear();
                m_data->minecraftProfile.skin.url.clear();
            }
        }
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
