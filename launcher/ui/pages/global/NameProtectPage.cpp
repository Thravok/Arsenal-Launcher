// SPDX-License-Identifier: GPL-3.0-only
#include "NameProtectPage.h"
#include "ui_NameProtectPage.h"

#include "Application.h"
#include "minecraft/NameProtectConfig.h"
#include "minecraft/auth/AccountList.h"
#include "settings/SettingsObject.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMessageBox>

NameProtectPage::NameProtectPage(QWidget* parent) : QWidget(parent), ui(new Ui::NameProtectPage)
{
    ui->setupUi(this);
    loadSettings();
}

NameProtectPage::~NameProtectPage()
{
    delete ui;
}

void NameProtectPage::retranslate()
{
    ui->retranslateUi(this);
}

void NameProtectPage::loadSettings()
{
    auto s = APPLICATION->settings();
    ui->enabledCheck->setChecked(s->get(NameProtect::EnabledSetting).toBool());
    ui->manageCheck->setChecked(s->get(NameProtect::ManageFromLauncherSetting).toBool());
    ui->maskAccountsCheck->setChecked(s->get(NameProtect::MaskAccountsSetting).toBool());

    QString alias = s->get(NameProtect::SelfAliasSetting).toString();
    if (alias.isEmpty())
        alias = QStringLiteral("Hidden");
    ui->aliasEdit->setText(alias);

    QString rules = s->get(NameProtect::RulesSetting).toString().trimmed();
    if (rules.isEmpty())
        rules = QStringLiteral("{\n}\n");
    ui->rulesEdit->setPlainText(rules);
}

void NameProtectPage::applySettings()
{
    auto s = APPLICATION->settings();
    s->set(NameProtect::EnabledSetting, ui->enabledCheck->isChecked());
    s->set(NameProtect::ManageFromLauncherSetting, ui->manageCheck->isChecked());
    s->set(NameProtect::MaskAccountsSetting, ui->maskAccountsCheck->isChecked());

    QString alias = ui->aliasEdit->text().trimmed();
    if (alias.isEmpty())
        alias = QStringLiteral("Hidden");
    s->set(NameProtect::SelfAliasSetting, alias);

    QString rulesText = ui->rulesEdit->toPlainText().trimmed();
    if (rulesText.isEmpty())
        rulesText = QStringLiteral("{}");
    s->set(NameProtect::RulesSetting, rulesText);

    // Refresh account list display when masking toggles.
    if (auto accounts = APPLICATION->accounts())
        accounts->refreshDisplayNames();
}

bool NameProtectPage::apply()
{
    const QString rulesText = ui->rulesEdit->toPlainText().trimmed();
    if (!rulesText.isEmpty()) {
        QJsonParseError err;
        const auto doc = QJsonDocument::fromJson(rulesText.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            QMessageBox::warning(this, tr("Name Protect"),
                                 tr("Extra rules must be a JSON object, e.g. {\"Name\":\"Alias\"}.\n%1")
                                     .arg(err.errorString()));
            return false;
        }
    }
    applySettings();
    return true;
}
