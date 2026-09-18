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

/** MHF_Steve / MHF_Alex placeholders from Parsers::parseMinecraftProfileMojang. */
inline bool isMhfDefaultSkinUrl(const QString& url)
{
    return url.contains(QStringLiteral("1a4af718455d4aab528e7a61f86fa25e6a369d1768dcb13f7df319a713eb810b")) ||
           url.contains(QStringLiteral("83cee5ca6afcdb171285aa00e8049c297b2dbeba0efb8ff970a5677a1b644032"));
}

/**
 * URL GetSkinStep should download for an Altening account.
 * Prefers a real Mojang texture; otherwise the head CDN when a skin id is known;
 * clears MHF defaults and body-CDN placeholders that cannot be cropped by getFace().
 */
inline QString resolveAlteningSkinDownloadUrl(const QString& currentUrl, const QString& alteningSkinId)
{
    const bool usableMojangTexture = currentUrl.contains(QStringLiteral("textures.minecraft.net")) && !isMhfDefaultSkinUrl(currentUrl);
    if (usableMojangTexture) {
        return currentUrl;
    }
    if (!alteningSkinId.isEmpty()) {
        return skinCdnHeadUrl(alteningSkinId);
    }
    if (isMhfDefaultSkinUrl(currentUrl) || currentUrl.contains(QStringLiteral("cdn.thealtening.com/skins/body/"))) {
        return {};
    }
    return currentUrl;
}

/** API key from launcher settings (trimmed). Empty if unset. */
QString storedApiKey();

}  // namespace TheAltening
