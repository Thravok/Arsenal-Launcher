#pragma once

#include "minecraft/auth/AccountData.h"

#include <QByteArray>
#include <QJsonObject>
#include <QString>

namespace TheAlteningAuth {

bool applyLicenseJson(AccountData* data, const QJsonObject& json, QString* errorOut = nullptr);

struct GenerateApplyResult {
    bool ok = false;
    bool dailyLimit = false;
    QString error;
};

GenerateApplyResult applyGenerateJson(AccountData* data, const QJsonObject& json);

bool applyAuthenticateJson(AccountData* data, const QByteArray& body, QString* errorOut = nullptr);

}  // namespace TheAlteningAuth
