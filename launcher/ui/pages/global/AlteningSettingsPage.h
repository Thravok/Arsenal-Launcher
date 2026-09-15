// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QWidget>

#include "minecraft/auth/TheAlteningApi.h"
#include "ui/pages/BasePage.h"
#include <Application.h>

namespace Ui {
class AlteningSettingsPage;
}

class AlteningSettingsPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit AlteningSettingsPage(QWidget *parent = nullptr);
    ~AlteningSettingsPage() override;

    QString displayName() const override { return tr("The Altening"); }
    QIcon icon() const override { return APPLICATION->getThemedIcon("accounts"); }
    QString id() const override { return QStringLiteral("the-altening"); }
    QString helpPage() const override { return QStringLiteral("Getting-Started#adding-an-account"); }
    bool apply() override;
    void retranslate() override;

   private slots:
    void onVerifySaveClicked();
    void onLicenseSucceeded(QJsonObject json);
    void onLicenseFailed(const QString &reason);
    void onApiKeyEdited(const QString &text);

   private:
    void loadSettings();
    void setBusy(bool busy);
    void updateLicenseLabel(const QString &licenseType, const QString &expires);

    Ui::AlteningSettingsPage *ui;
    TheAlteningApi::Ptr m_api;
    bool m_verifyInProgress = false;
    bool m_lastVerifyOk = false;
};
