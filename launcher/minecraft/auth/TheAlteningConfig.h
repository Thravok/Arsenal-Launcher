#pragma once

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
inline QString skinCdnBodyUrl(const QString &skinId)
{
    return SkinCdnBodyBaseUrl + skinId + QStringLiteral(".png");
}

/** Head render suitable for account list avatars. */
inline QString skinCdnHeadUrl(const QString &skinId)
{
    return SkinCdnHeadBaseUrl + skinId + QStringLiteral(".png");
}

/** @deprecated Prefer skinCdnHeadUrl for avatars; body renders are not cropable by getFace(). */
inline QString skinCdnUrl(const QString &skinId)
{
    return skinCdnHeadUrl(skinId);
}

/** API key from launcher settings (trimmed). Empty if unset. */
QString storedApiKey();

} // namespace TheAltening
