// SPDX-License-Identifier: GPL-3.0-only
#include "AlteningSettingsPage.h"
#include "ui_AlteningSettingsPage.h"

#include "Application.h"
#include "minecraft/auth/TheAlteningConfig.h"
#include "settings/SettingsObject.h"

#include <QMessageBox>

AlteningSettingsPage::AlteningSettingsPage(QWidget *parent)
    : QWidget(parent), ui(new Ui::AlteningSettingsPage)
{
    ui->setupUi(this);
    connect(ui->verifySaveButton, &QPushButton::clicked, this, &AlteningSettingsPage::onVerifySaveClicked);
    connect(ui->apiKeyEdit, &QLineEdit::textEdited, this, &AlteningSettingsPage::onApiKeyEdited);
    loadSettings();
}

AlteningSettingsPage::~AlteningSettingsPage()
{
    delete ui;
}

void AlteningSettingsPage::retranslate()
{
    ui->retranslateUi(this);
}

void AlteningSettingsPage::loadSettings()
{
    const QString key = TheAltening::storedApiKey();
    ui->apiKeyEdit->setText(key);
    m_lastVerifyOk = !key.isEmpty();
    if (key.isEmpty()) {
        ui->licenseStatusLabel->setText(tr("No API key saved."));
    } else {
        ui->licenseStatusLabel->setText(tr("API key saved. Use Verify & Save to check the license."));
    }
    ui->verifySaveButton->setEnabled(!key.isEmpty());
}

bool AlteningSettingsPage::apply()
{
    const QString key = ui->apiKeyEdit->text().trimmed();
    if (key.isEmpty()) {
        APPLICATION->settings()->set(TheAltening::ApiKeySettingName, QString());
        ui->licenseStatusLabel->setText(tr("No API key saved."));
        m_lastVerifyOk = false;
        return true;
    }

    // Already verified this exact key in this session.
    if (m_lastVerifyOk && key == TheAltening::storedApiKey()) {
        return true;
    }

    // Closing settings without re-verify: keep previous saved key if the field matches it.
    if (key == TheAltening::storedApiKey()) {
        return true;
    }

    // Key changed but not verified — require Verify & Save first.
    QMessageBox::warning(
        this,
        tr("The Altening"),
        tr("Please click Verify & Save to validate the new API key before leaving this page.")
    );
    return false;
}

void AlteningSettingsPage::onApiKeyEdited(const QString &text)
{
    m_lastVerifyOk = false;
    ui->verifySaveButton->setEnabled(!text.trimmed().isEmpty());
}

void AlteningSettingsPage::setBusy(bool busy)
{
    m_verifyInProgress = busy;
    ui->apiKeyEdit->setEnabled(!busy);
    ui->verifySaveButton->setEnabled(!busy && !ui->apiKeyEdit->text().trimmed().isEmpty());
    ui->progressBar->setVisible(busy);
}

void AlteningSettingsPage::updateLicenseLabel(const QString &licenseType, const QString &expires)
{
    QString text = tr("License active.");
    if (!licenseType.isEmpty()) {
        text = tr("License: %1").arg(licenseType);
        if (!expires.isEmpty()) {
            text += tr(" (expires %1)").arg(expires);
        }
    }
    ui->licenseStatusLabel->setText(text);
}

void AlteningSettingsPage::onVerifySaveClicked()
{
    const QString key = ui->apiKeyEdit->text().trimmed();
    if (key.isEmpty()) {
        return;
    }

    setBusy(true);
    ui->licenseStatusLabel->setText(tr("Checking The Altening license…"));

    m_api.reset(new TheAlteningApi(this));
    connect(m_api.get(), &TheAlteningApi::succeeded, this, &AlteningSettingsPage::onLicenseSucceeded);
    connect(m_api.get(), &TheAlteningApi::failed, this, &AlteningSettingsPage::onLicenseFailed);
    m_api->checkLicense(key);
}

void AlteningSettingsPage::onLicenseSucceeded(QJsonObject json)
{
    setBusy(false);

    if (!json.value(QStringLiteral("hasLicense")).toBool(false)) {
        onLicenseFailed(tr("This API key does not have an active The Altening license."));
        return;
    }

    const QString key = ui->apiKeyEdit->text().trimmed();
    APPLICATION->settings()->set(TheAltening::ApiKeySettingName, key);
    m_lastVerifyOk = true;

    updateLicenseLabel(
        json.value(QStringLiteral("licenseType")).toString(),
        json.value(QStringLiteral("expires")).toString()
    );
}

void AlteningSettingsPage::onLicenseFailed(const QString &reason)
{
    setBusy(false);
    m_lastVerifyOk = false;
    ui->licenseStatusLabel->setText(QStringLiteral("<font color='red'>%1</font>").arg(reason.toHtmlEscaped()));
}
