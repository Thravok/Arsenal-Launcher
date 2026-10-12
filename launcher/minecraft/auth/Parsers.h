#pragma once

#include <QString>

#include "AccountData.h"

namespace Parsers {
bool getDateTime(QJsonValue value, QDateTime& out);
bool getString(QJsonValue value, QString& out);
bool getNumber(QJsonValue value, double& out);
bool getNumber(QJsonValue value, int64_t& out);
bool getBool(QJsonValue value, bool& out);

/** Map known Xbox Live XErr codes to a user-facing message; empty if unknown. */
QString xboxXErrMessage(int64_t errorCode);

/** Structural JWT check (three base64url segments that decode). No signature verify. */
bool isStructurallyValidJwt(const QString& token);

bool parseXTokenResponse(QByteArray& data, Token& output, QString name, QString* errorOut = nullptr);
bool parseMojangResponse(QByteArray& data, Token& output);

bool parseMinecraftProfile(QByteArray& data, MinecraftProfile& output);
bool parseMinecraftProfileMojang(QByteArray& data, MinecraftProfile& output);
bool parseMinecraftEntitlements(QByteArray& data, MinecraftEntitlement& output, const QString& expectedRequestId = {});
bool parseRolloutResponse(QByteArray& data, bool& result);
}  // namespace Parsers
