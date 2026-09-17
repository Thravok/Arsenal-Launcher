#include "TheAlteningAuthParse.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QObject>

namespace TheAlteningAuth {

bool applyLicenseJson(AccountData* data, const QJsonObject& json, QString* errorOut)
{
    if (!json.value(QStringLiteral("hasLicense")).toBool(false)) {
        if (errorOut) {
            *errorOut = QObject::tr("The Altening API key has no active license.");
        }
        return false;
    }

    data->theAlteningLicenseType = json.value(QStringLiteral("licenseType")).toString();
    data->theAlteningLicenseExpires = json.value(QStringLiteral("expires")).toString();
    return true;
}

GenerateApplyResult applyGenerateJson(AccountData* data, const QJsonObject& json)
{
    GenerateApplyResult result;
    const QString token = json.value(QStringLiteral("token")).toString();
    const QString password = json.value(QStringLiteral("password")).toString(QStringLiteral("anything"));
    if (token.isEmpty()) {
        result.error = QObject::tr("The Altening API did not return an alt token.");
        return result;
    }

    data->yggdrasilToken.extra[QStringLiteral("userName")] = token;
    data->theAlteningPendingPassword = password;

    const QString altUsername = json.value(QStringLiteral("username")).toString();
    if (!altUsername.isEmpty()) {
        data->theAlteningAltUsername = altUsername;
    }

    const QString skin = json.value(QStringLiteral("skin")).toString();
    if (!skin.isEmpty()) {
        data->yggdrasilToken.extra[QStringLiteral("alteningSkin")] = skin;
    }

    result.ok = true;
    result.dailyLimit = json.value(QStringLiteral("limit")).toBool(false);
    return result;
}

bool applyAuthenticateJson(AccountData* data, const QByteArray& body, QString* errorOut)
{
    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(body, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorOut) {
            *errorOut = QObject::tr("The Altening authentication returned invalid JSON.");
        }
        return false;
    }
    auto obj = doc.object();
    const QString clientToken = obj.value(QStringLiteral("clientToken")).toString();
    const QString accessToken = obj.value(QStringLiteral("accessToken")).toString();
    if (clientToken.isEmpty() || accessToken.isEmpty()) {
        if (errorOut) {
            *errorOut = QObject::tr("The Altening authentication response was incomplete.");
        }
        return false;
    }

    data->yggdrasilToken.extra[QStringLiteral("clientToken")] = clientToken;
    data->yggdrasilToken.token = accessToken;
    data->yggdrasilToken.validity = Validity::Certain;
    data->yggdrasilToken.issueInstant = QDateTime::currentDateTimeUtc();
    data->yggdrasilToken.notAfter = QDateTime();

    const auto profile = obj.value(QStringLiteral("selectedProfile")).toObject();
    data->minecraftProfile.id = profile.value(QStringLiteral("id")).toString();
    data->minecraftProfile.name = profile.value(QStringLiteral("name")).toString();
    if (data->minecraftProfile.id.isEmpty()) {
        if (errorOut) {
            *errorOut = QObject::tr("The Altening authentication did not return a profile.");
        }
        return false;
    }
    data->minecraftProfile.validity = Validity::Certain;
    data->minecraftEntitlement.canPlayMinecraft = true;
    data->minecraftEntitlement.ownsMinecraft = true;
    data->minecraftEntitlement.validity = Validity::Assumed;
    return true;
}

}  // namespace TheAlteningAuth
