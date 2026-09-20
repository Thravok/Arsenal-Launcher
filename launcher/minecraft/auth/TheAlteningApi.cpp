#include "TheAlteningApi.h"
#include "TheAlteningConfig.h"

#include "Application.h"
#include "settings/SettingsObject.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QNetworkRequest>
#include <QUrlQuery>

namespace TheAltening {

QString storedApiKey()
{
    return APPLICATION->settings()->get(ApiKeySettingName).toString().trimmed();
}

}  // namespace TheAltening

TheAlteningApi::TheAlteningApi(QObject* parent) : QObject(parent) {}

QString TheAlteningApi::errorMessageForStatus(int httpStatus)
{
    return TheAltening::errorMessageForStatus(httpStatus);
}

void TheAlteningApi::checkLicense(const QString& apiKey)
{
    get(QStringLiteral("license"), { { QStringLiteral("key"), apiKey } });
}

void TheAlteningApi::generate(const QString& apiKey, bool withInfo)
{
    QList<QPair<QString, QString>> query = { { QStringLiteral("key"), apiKey } };
    if (withInfo) {
        query.append({ QStringLiteral("info"), QStringLiteral("true") });
    }
    get(QStringLiteral("generate"), query);
}

void TheAlteningApi::info(const QString& apiKey, const QString& token)
{
    get(QStringLiteral("info"), { { QStringLiteral("key"), apiKey }, { QStringLiteral("token"), token } });
}

void TheAlteningApi::get(const QString& path, const QList<QPair<QString, QString>>& query)
{
    if (m_reply) {
        m_reply->disconnect(this);
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    QUrl url(TheAltening::ApiBaseUrl + path);
    QUrlQuery urlQuery;
    for (const auto& pair : query) {
        urlQuery.addQueryItem(pair.first, pair.second);
    }
    url.setQuery(urlQuery);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Arsenal-TheAltening"));
    m_reply = APPLICATION->network()->get(request);
    connect(m_reply, &QNetworkReply::finished, this, &TheAlteningApi::onReplyFinished);
}

void TheAlteningApi::onReplyFinished()
{
    auto* reply = m_reply;
    m_reply = nullptr;
    if (!reply) {
        return;
    }
    reply->deleteLater();

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body = reply->readAll();

    if (reply->error() != QNetworkReply::NoError && status == 0) {
        emit failed(tr("Network error talking to The Altening API: %1").arg(reply->errorString()));
        return;
    }

    if (status != 200) {
        emit failed(errorMessageForStatus(status));
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        emit failed(tr("Failed to parse The Altening API response."));
        return;
    }

    emit succeeded(doc.object());
}
