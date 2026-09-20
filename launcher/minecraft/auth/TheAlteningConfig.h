#pragma once

#include <QObject>
#include <QString>

namespace TheAltening {

inline const QString ApiBaseUrl = QStringLiteral("https://api.thealtening.com/v2/");
inline const QString AuthServerUrl = QStringLiteral("http://authserver.thealtening.com");
inline const QString SessionServerUrl = QStringLiteral("http://sessionserver.thealtening.com");
inline const QString SkinCdnBodyBaseUrl = QStringLiteral("https://cdn.thealtening.com/skins/body/");
inline const QString SkinCdnHeadBaseUrl = QStringLiteral("https://cdn.thealtening.com/skins/head/");
/** Global launcher setting name for the panel API key. */
inline const QString ApiKeySettingName = QStringLiteral("TheAlteningApiKey");
/** Sentinel stored in AuthSession to trigger The Altening authlib-injector setup at launch. */
inline const QString AuthlibInjectorSentinel = QStringLiteral("thealtening://local");

/** Full-body rendered preview (not a Minecraft skin texture). */
inline QString skinCdnBodyUrl(const QString& skinId)
{
    return SkinCdnBodyBaseUrl + skinId + QStringLiteral(".png");
}

/** Head render suitable for account list avatars. */
inline QString skinCdnHeadUrl(const QString& skinId)
{
    return SkinCdnHeadBaseUrl + skinId + QStringLiteral(".png");
}

/** @deprecated Prefer skinCdnHeadUrl for avatars; body renders are not cropable by getFace(). */
inline QString skinCdnUrl(const QString& skinId)
{
    return skinCdnHeadUrl(skinId);
}

/** User-facing error for a non-200 panel API status. */
inline QString errorMessageForStatus(int httpStatus)
{
    switch (httpStatus) {
        case 401:
            return QObject::tr("Invalid or missing The Altening API key.");
        case 403:
            return QObject::tr("Your The Altening plan cannot use this API endpoint. API access requires Basic or Premium (not Starter).");
        case 404:
            return QObject::tr("The Altening API endpoint was not found.");
        case 500:
            return QObject::tr("The Altening API reported an internal server error. Try again later.");
        default:
            return QObject::tr("The Altening API request failed (HTTP %1).").arg(httpStatus);
    }
}

/** API key from launcher settings (trimmed). Empty if unset. */
QString storedApiKey();

}  // namespace TheAltening
