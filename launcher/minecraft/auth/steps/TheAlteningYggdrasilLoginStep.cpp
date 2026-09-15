#include "TheAlteningYggdrasilLoginStep.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUrl>
#include <QUuid>

#include "Application.h"
#include "minecraft/auth/TheAlteningConfig.h"
#include "net/NetJob.h"
#include "net/NetUtils.h"
#include "net/RawHeaderProxy.h"
#include "net/Request.h"

TheAlteningYggdrasilLoginStep::TheAlteningYggdrasilLoginStep(AccountData* data, bool refresh) : AuthStep(data), m_refresh(refresh) {}

QString TheAlteningYggdrasilLoginStep::describe()
{
    return m_refresh ? tr("Refreshing The Altening session.") : tr("Logging in with The Altening account.");
}

void TheAlteningYggdrasilLoginStep::perform()
{
    // The Altening alt tokens (username) stay valid for the day; access tokens do not.
    // Always re-authenticate with the stored alt token instead of Yggdrasil /refresh,
    // which fails after the token was used in-game and then leaves the account "Expired".
    const QString altToken = m_data->yggdrasilToken.extra.value(QStringLiteral("userName")).toString();
    if (altToken.isEmpty()) {
        emit finished(AccountTaskState::STATE_FAILED_HARD,
                      tr("This The Altening account has no alt token. Generate a new one."));
        return;
    }

    QJsonObject agent;
    agent.insert(QStringLiteral("name"), QStringLiteral("Minecraft"));
    agent.insert(QStringLiteral("version"), 1);

    QJsonObject req;
    req.insert(QStringLiteral("agent"), agent);
    req.insert(QStringLiteral("username"), altToken);

    QString password = m_data->theAlteningPendingPassword;
    m_data->theAlteningPendingPassword.clear();
    if (password.isEmpty()) {
        password = QStringLiteral("anything");
    }
    req.insert(QStringLiteral("password"), password);
    req.insert(QStringLiteral("requestUser"), false);

    if (!m_data->yggdrasilToken.extra.contains(QStringLiteral("clientToken"))) {
        m_data->yggdrasilToken.extra[QStringLiteral("clientToken")] = QUuid::createUuid().toString(QUuid::Id128);
    }
    req.insert(QStringLiteral("clientToken"), m_data->yggdrasilToken.extra.value(QStringLiteral("clientToken")).toString());

    QUrl url(TheAltening::AuthServerUrl + QStringLiteral("/authenticate"));
    auto body = QJsonDocument(req).toJson(QJsonDocument::Compact);

    auto headers = QList<Net::HeaderPair>{ { "Content-Type", "application/json" } };
    auto [request, response] = Net::Request::makeByteArray(url, body);
    m_request = request;
    m_request->addHeaderProxy(std::make_unique<Net::RawHeaderProxy>(headers));

    m_task.reset(new NetJob("TheAlteningYggdrasilLoginStep", APPLICATION->network()));
    m_task->setAskRetry(false);
    m_task->addNetAction(m_request);
    connect(m_task.get(), &Task::finished, this, [this, response] { onRequestDone(response); });
    m_task->start();
}

void TheAlteningYggdrasilLoginStep::onRequestDone(QByteArray* response)
{
    if (m_request->error() != QNetworkReply::NoError) {
        qWarning() << "The Altening authentication failed:"
                    << "HTTP" << m_request->replyStatusCode()
                    << "error" << m_request->error()
                    << m_request->errorString()
                    << "body" << QString::fromUtf8(*response);

        QString serverMessage;
        QJsonParseError err;
        const auto doc = QJsonDocument::fromJson(*response, &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            serverMessage = doc.object().value(QStringLiteral("errorMessage")).toString();
            if (serverMessage.isEmpty()) {
                serverMessage = doc.object().value(QStringLiteral("error")).toString();
            }
        }

        const auto networkError = m_request->error();
        if (Net::isApplicationError(networkError) && !Net::isServerError(networkError)) {
            const QString detail = serverMessage.isEmpty() ? m_request->errorString() : serverMessage;
            emit finished(AccountTaskState::STATE_FAILED_HARD,
                          tr("The Altening authentication failed: %1").arg(detail));
            return;
        }

        m_data->networkError = networkError;
        emit finished(AccountTaskState::STATE_OFFLINE,
                      tr("Failed to reach The Altening authentication servers: %1").arg(m_request->errorString()));
        return;
    }

    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(*response, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        emit finished(AccountTaskState::STATE_FAILED_SOFT, tr("The Altening authentication returned invalid JSON."));
        return;
    }
    auto obj = doc.object();
    const QString clientToken = obj.value(QStringLiteral("clientToken")).toString();
    const QString accessToken = obj.value(QStringLiteral("accessToken")).toString();
    if (clientToken.isEmpty() || accessToken.isEmpty()) {
        emit finished(AccountTaskState::STATE_FAILED_SOFT, tr("The Altening authentication response was incomplete."));
        return;
    }

    m_data->yggdrasilToken.extra[QStringLiteral("clientToken")] = clientToken;
    m_data->yggdrasilToken.token = accessToken;
    m_data->yggdrasilToken.validity = Validity::Certain;
    m_data->yggdrasilToken.issueInstant = QDateTime::currentDateTimeUtc();
    m_data->yggdrasilToken.notAfter = QDateTime();

    const auto profile = obj.value(QStringLiteral("selectedProfile")).toObject();
    m_data->minecraftProfile.id = profile.value(QStringLiteral("id")).toString();
    m_data->minecraftProfile.name = profile.value(QStringLiteral("name")).toString();
    if (m_data->minecraftProfile.id.isEmpty()) {
        emit finished(AccountTaskState::STATE_FAILED_SOFT, tr("The Altening authentication did not return a profile."));
        return;
    }
    m_data->minecraftProfile.validity = Validity::Certain;
    m_data->minecraftEntitlement.canPlayMinecraft = true;
    m_data->minecraftEntitlement.ownsMinecraft = true;
    m_data->minecraftEntitlement.validity = Validity::Assumed;

    emit finished(AccountTaskState::STATE_WORKING, tr("Logged in with The Altening."));
}
