// SPDX-License-Identifier: GPL-3.0-only

#include "AccountListPage.h"
#include "ui_AccountListPage.h"

#include <QItemSelectionModel>
#include <QMenu>
#include <QMessageBox>

#include "Application.h"
#include "BuildConfig.h"
#include "minecraft/auth/AccountData.h"
#include "minecraft/auth/MinecraftAccount.h"
#include "minecraft/auth/TheAlteningApi.h"
#include "minecraft/auth/TheAlteningConfig.h"
#include "settings/SettingsObject.h"
#include "ui/dialogs/ChooseOfflineNameDialog.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/MSALoginDialog.h"
#include "ui/dialogs/ProgressDialog.h"
#include "ui/dialogs/skins/SkinManageDialog.h"

AccountListPage::AccountListPage(QWidget* parent) : QMainWindow(parent), ui(new Ui::AccountListPage)
{
    ui->setupUi(this);
    ui->listView->setEmptyString(
        tr("Welcome!\n"
           "If you're new here, you can select the \"Add Microsoft\" button to link your Microsoft account."));
    ui->listView->setEmptyMode(VersionListView::String);
    ui->listView->setContextMenuPolicy(Qt::CustomContextMenu);

    m_accounts = APPLICATION->accounts();

    ui->listView->setModel(m_accounts);
    ui->listView->header()->setSectionResizeMode(AccountList::VListColumns::ProfileNameColumn, QHeaderView::Stretch);
    ui->listView->header()->setSectionResizeMode(AccountList::VListColumns::TypeColumn, QHeaderView::ResizeToContents);
    ui->listView->header()->setSectionResizeMode(AccountList::VListColumns::StatusColumn, QHeaderView::ResizeToContents);
    ui->listView->setSelectionMode(QAbstractItemView::SingleSelection);

    QItemSelectionModel* selectionModel = ui->listView->selectionModel();

    connect(selectionModel, &QItemSelectionModel::selectionChanged, this,
            [this]([[maybe_unused]] const QItemSelection& sel, [[maybe_unused]] const QItemSelection& dsel) {
                updateButtonStates();
            });
    connect(ui->listView, &VersionListView::customContextMenuRequested, this, &AccountListPage::ShowContextMenu);
    connect(ui->listView, &VersionListView::activated, this,
            [this](const QModelIndex& index) { m_accounts->setDefaultAccount(m_accounts->at(index.row())); });

    connect(m_accounts, &AccountList::listChanged, this, &AccountListPage::listChanged);
    connect(m_accounts, &AccountList::listActivityChanged, this, &AccountListPage::listChanged);
    connect(m_accounts, &AccountList::defaultAccountChanged, this, &AccountListPage::listChanged);

    updateButtonStates();
    migrateAlteningApiKeyFromAccounts();

    if (~APPLICATION->capabilities() & Application::SupportsMSA) {
        ui->actionAddMicrosoft->setVisible(false);
        ui->actionAddMicrosoft->setToolTip(tr("No Microsoft Authentication client ID was set."));
    }
    ui->actionAddAuthlibInjector->setVisible(false);
}

AccountListPage::~AccountListPage()
{
    delete ui;
}

void AccountListPage::retranslate()
{
    ui->retranslateUi(this);
}

void AccountListPage::openedImpl()
{
    migrateAlteningApiKeyFromAccounts();
    updateButtonStates();
}

void AccountListPage::ShowContextMenu(const QPoint& pos)
{
    auto menu = ui->toolBar->createContextMenu(this, tr("Context menu"));
    menu->exec(ui->listView->mapToGlobal(pos));
    delete menu;
}

void AccountListPage::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QMainWindow::changeEvent(event);
}

QMenu* AccountListPage::createPopupMenu()
{
    QMenu* filteredMenu = QMainWindow::createPopupMenu();
    filteredMenu->removeAction(ui->toolBar->toggleViewAction());
    return filteredMenu;
}

void AccountListPage::listChanged()
{
    updateButtonStates();
}

void AccountListPage::on_actionAddAuthlibInjector_triggered()
{
}

void AccountListPage::migrateAlteningApiKeyFromAccounts()
{
    auto settings = APPLICATION->settings();
    if (!TheAltening::storedApiKey().isEmpty()) {
        return;
    }

    for (int i = 0; i < m_accounts->count(); ++i) {
        auto account = m_accounts->at(i);
        if (!account) {
            continue;
        }
        auto* data = account->accountData();
        if (data->type == AccountType::TheAltening && !data->theAlteningApiKey.isEmpty()) {
            settings->set(TheAltening::ApiKeySettingName, data->theAlteningApiKey);
            return;
        }
    }
}

void AccountListPage::on_actionGenerateAltening_triggered()
{
    migrateAlteningApiKeyFromAccounts();
    const QString apiKey = TheAltening::storedApiKey();
    if (apiKey.isEmpty()) {
        auto reply = QMessageBox::question(
            this,
            tr("The Altening API Key Required"),
            tr("Set your The Altening API key in Settings → The Altening before generating an account.\n\nOpen settings now?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes);
        if (reply == QMessageBox::Yes) {
            APPLICATION->ShowGlobalSettings(this, QStringLiteral("the-altening"));
        }
        updateButtonStates();
        return;
    }

    MinecraftAccountPtr account = MinecraftAccount::createTheAlteningFromApiKey(apiKey);
    auto loginTask = account->login();
    ProgressDialog prog(this);
    if (prog.execWithTask(loginTask.get()) != QDialog::Accepted) {
        CustomMessageBox::selectable(this, tr("The Altening"), tr("Failed to generate a The Altening account."),
                                     QMessageBox::Warning)
            ->exec();
        return;
    }

    m_accounts->addAccount(account);
    if (!m_accounts->defaultAccount()) {
        m_accounts->setDefaultAccount(account);
    }

    const int row = m_accounts->findAccountByProfileId(account->profileId());
    if (row >= 0) {
        const QModelIndex idx = m_accounts->index(row, 0);
        ui->listView->setCurrentIndex(idx);
        ui->listView->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }
    updateButtonStates();
}

void AccountListPage::on_actionAddMicrosoft_triggered()
{
    if (BuildConfig.BUILD_PLATFORM == "osx64") {
        CustomMessageBox::selectable(
            this,
            tr("Microsoft Accounts not available"),
            tr("Microsoft accounts are only usable on macOS 10.13 or newer, with fully updated %1.\n\n"
               "Please update both your operating system and %1.")
                .arg(BuildConfig.LAUNCHER_NAME),
            QMessageBox::Warning)
            ->exec();
        return;
    }
    if (auto account = MSALoginDialog::newAccount(this)) {
        m_accounts->addAccount(account);
        if (m_accounts->count() == 1) {
            m_accounts->setDefaultAccount(account);
        }
    }
}

void AccountListPage::on_actionAddOffline_triggered()
{
    if (!m_accounts->anyAccountIsValid()) {
        QMessageBox::warning(
            this,
            tr("Error"),
            tr("You must add a Microsoft account that owns Minecraft before you can add an offline account."
               "<br><br>"
               "If you have lost your account you can contact Microsoft for support."));
        return;
    }

    ChooseOfflineNameDialog dialog(tr("Please enter your desired username to add your offline account."), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    if (const MinecraftAccountPtr account = MinecraftAccount::createOffline(dialog.getUsername())) {
        account->login()->start();
        m_accounts->addAccount(account);
        if (m_accounts->count() == 1) {
            m_accounts->setDefaultAccount(account);
        }
    }
}

void AccountListPage::on_actionRemove_triggered()
{
    auto response = CustomMessageBox::selectable(this, tr("Remove account?"), tr("Do you really want to delete this account?"),
                                                 QMessageBox::Question, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                        ->exec();
    if (response != QMessageBox::Yes) {
        return;
    }
    QModelIndexList selection = ui->listView->selectionModel()->selectedIndexes();
    if (!selection.isEmpty()) {
        m_accounts->removeAccount(selection.first());
    }
}

void AccountListPage::on_actionRefresh_triggered()
{
    QModelIndexList selection = ui->listView->selectionModel()->selectedIndexes();
    if (!selection.isEmpty()) {
        MinecraftAccountPtr account = selection.first().data(AccountList::PointerRole).value<MinecraftAccountPtr>();
        m_accounts->requestRefresh(account->internalId());
    }
}

void AccountListPage::on_actionSetDefault_triggered()
{
    QModelIndexList selection = ui->listView->selectionModel()->selectedIndexes();
    if (!selection.isEmpty()) {
        MinecraftAccountPtr account = selection.first().data(AccountList::PointerRole).value<MinecraftAccountPtr>();
        m_accounts->setDefaultAccount(account);
    }
}

void AccountListPage::on_actionNoDefault_triggered()
{
    m_accounts->setDefaultAccount(nullptr);
}

void AccountListPage::updateButtonStates()
{
    QModelIndexList selection = ui->listView->selectionModel()->selectedIndexes();
    const bool hasSelection = !selection.empty();
    bool accountIsReady = false;
    bool accountIsOnline = false;
    if (hasSelection) {
        MinecraftAccountPtr account = selection.first().data(AccountList::PointerRole).value<MinecraftAccountPtr>();
        accountIsReady = !account->isActive();
        accountIsOnline = account->accountType() != AccountType::Offline;
    }
    ui->actionRemove->setEnabled(accountIsReady);
    ui->actionSetDefault->setEnabled(accountIsReady);
    ui->actionUploadSkin->setEnabled(accountIsReady && accountIsOnline);
    ui->actionDeleteSkin->setEnabled(accountIsReady && accountIsOnline);
    ui->actionRefresh->setEnabled(accountIsReady && accountIsOnline);

    const bool hasAlteningKey = !TheAltening::storedApiKey().isEmpty();
    ui->actionGenerateAltening->setEnabled(hasAlteningKey);
    ui->actionGenerateAltening->setToolTip(hasAlteningKey
                                               ? tr("Generate a new The Altening alt. Refresh renews the selected alt's session.")
                                               : tr("Set the API key in Settings → The Altening before generating an account."));

    if (m_accounts->defaultAccount().get() == nullptr) {
        ui->actionNoDefault->setEnabled(false);
        ui->actionNoDefault->setChecked(true);
    } else {
        ui->actionNoDefault->setEnabled(true);
        ui->actionNoDefault->setChecked(false);
    }
}

void AccountListPage::on_actionUploadSkin_triggered()
{
    QModelIndexList selection = ui->listView->selectionModel()->selectedIndexes();
    if (selection.isEmpty()) {
        return;
    }
    MinecraftAccountPtr account = selection.first().data(AccountList::PointerRole).value<MinecraftAccountPtr>();
    SkinManageDialog dialog(this, account);
    dialog.exec();
}

void AccountListPage::on_actionDeleteSkin_triggered()
{
    on_actionUploadSkin_triggered();
}
