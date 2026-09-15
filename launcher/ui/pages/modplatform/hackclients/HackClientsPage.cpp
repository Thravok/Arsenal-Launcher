// SPDX-License-Identifier: GPL-3.0-only
#include "HackClientsPage.h"
#include "ui_HackClientsPage.h"

#include "Application.h"
#include "icons/IconList.h"
#include "ui/dialogs/NewInstanceDialog.h"

#include "hackclients/BaritoneInstallTask.h"
#include "hackclients/FDPInstallTask.h"
#include "hackclients/ImpactInstallTask.h"
#include "hackclients/LambdaInstallTask.h"
#include "hackclients/LiquidBounceInstallTask.h"
#include "hackclients/MeteorInstallTask.h"
#include "hackclients/WurstInstallTask.h"

#include <QCheckBox>
#include <QComboBox>
#include <QListWidgetItem>

HackClientsPage::HackClientsPage(NewInstanceDialog* dialog, QWidget* parent)
    : QWidget(parent), dialog(dialog), ui(new Ui::HackClientsPage)
{
    ui->setupUi(this);

    auto* lbItem = new QListWidgetItem(tr("LiquidBounce"), ui->clientList);
    lbItem->setData(Qt::UserRole, ClientLiquidBounce);
    lbItem->setIcon(APPLICATION->icons()->getIcon(QStringLiteral("liquidbounce")));
    auto* meteorItem = new QListWidgetItem(tr("Meteor"), ui->clientList);
    meteorItem->setData(Qt::UserRole, ClientMeteor);
    meteorItem->setIcon(APPLICATION->icons()->getIcon(QStringLiteral("meteor")));
    auto* lambdaItem = new QListWidgetItem(tr("Lambda"), ui->clientList);
    lambdaItem->setData(Qt::UserRole, ClientLambda);
    lambdaItem->setIcon(APPLICATION->icons()->getIcon(QStringLiteral("lambda")));
    auto* impactItem = new QListWidgetItem(tr("Impact"), ui->clientList);
    impactItem->setData(Qt::UserRole, ClientImpact);
    impactItem->setIcon(APPLICATION->icons()->getIcon(QStringLiteral("impact")));
    auto* baritoneItem = new QListWidgetItem(tr("Baritone"), ui->clientList);
    baritoneItem->setData(Qt::UserRole, ClientBaritone);
    baritoneItem->setIcon(APPLICATION->getThemedIcon(QStringLiteral("loadermods")));
    auto* fdpItem = new QListWidgetItem(tr("FDPClient"), ui->clientList);
    fdpItem->setData(Qt::UserRole, ClientFDP);
    fdpItem->setIcon(APPLICATION->icons()->getIcon(QStringLiteral("fdp")));
    auto* wurstItem = new QListWidgetItem(tr("Wurst"), ui->clientList);
    wurstItem->setData(Qt::UserRole, ClientWurst);
    wurstItem->setIcon(APPLICATION->icons()->getIcon(QStringLiteral("wurst")));

    connect(ui->clientList, &QListWidget::currentItemChanged, this, &HackClientsPage::onClientSelectionChanged);
    connect(ui->versionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &HackClientsPage::onVersionChanged);
    connect(ui->refreshButton, &QPushButton::clicked, this, &HackClientsPage::onRefreshClicked);

    ui->refreshButton->setIcon(APPLICATION->getThemedIcon(QStringLiteral("refresh")));
    ui->refreshButton->setIconSize(QSize(18, 18));
    ui->clientList->setIconSize(QSize(32, 32));
    ui->clientList->setSpacing(4);
    ui->clientList->setMaximumWidth(200);
    ui->descriptionBrowser->setObjectName(QStringLiteral("hackClientDescription"));
    ui->descriptionBrowser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // Prevent long status / version strings from expanding the New Instance dialog.
    ui->statusLabel->setMinimumWidth(0);
    ui->versionCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->versionCombo->setMinimumContentsLength(12);
    ui->versionCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);

    m_wurstBaritoneCheck = new QCheckBox(tr("Also install Baritone (pathfinding)"), this);
    m_wurstBaritoneCheck->setToolTip(tr("Adds standalone Fabric Baritone to the new instance's mods folder."));
    m_wurstBaritoneCheck->setChecked(true);
    m_wurstBaritoneCheck->setVisible(false);
    ui->detailLayout->addWidget(m_wurstBaritoneCheck);

    m_impactProvider = new HackClients::ImpactReleasesProvider(APPLICATION->network(), this);
    m_lbProvider = new HackClients::LiquidBounceProvider(APPLICATION->network(), this);
    m_meteorProvider = new HackClients::MeteorProvider(APPLICATION->network(), this);
    m_lambdaProvider = new HackClients::LambdaProvider(APPLICATION->network(), this);
    m_baritoneProvider = new HackClients::BaritoneProvider(APPLICATION->network(), this);
    m_fdpProvider = new HackClients::FDPProvider(APPLICATION->network(), this);
    m_wurstProvider = new HackClients::WurstProvider(APPLICATION->network(), this);
    connect(m_impactProvider, &HackClients::ImpactReleasesProvider::refreshed, this, &HackClientsPage::onImpactReady);
    connect(m_impactProvider, &HackClients::ImpactReleasesProvider::failed, this, &HackClientsPage::onImpactFailed);
    connect(m_lbProvider, &HackClients::LiquidBounceProvider::refreshed, this, &HackClientsPage::onLiquidBounceReady);
    connect(m_lbProvider, &HackClients::LiquidBounceProvider::failed, this, &HackClientsPage::onLiquidBounceFailed);
    connect(m_meteorProvider, &HackClients::MeteorProvider::refreshed, this, &HackClientsPage::onMeteorReady);
    connect(m_meteorProvider, &HackClients::MeteorProvider::failed, this, &HackClientsPage::onMeteorFailed);
    connect(m_lambdaProvider, &HackClients::LambdaProvider::refreshed, this, &HackClientsPage::onLambdaReady);
    connect(m_lambdaProvider, &HackClients::LambdaProvider::failed, this, &HackClientsPage::onLambdaFailed);
    connect(m_baritoneProvider, &HackClients::BaritoneProvider::refreshed, this, &HackClientsPage::onBaritoneReady);
    connect(m_baritoneProvider, &HackClients::BaritoneProvider::failed, this, &HackClientsPage::onBaritoneFailed);
    connect(m_fdpProvider, &HackClients::FDPProvider::refreshed, this, &HackClientsPage::onFDPReady);
    connect(m_fdpProvider, &HackClients::FDPProvider::failed, this, &HackClientsPage::onFDPFailed);
    connect(m_wurstProvider, &HackClients::WurstProvider::refreshed, this, &HackClientsPage::onWurstReady);
    connect(m_wurstProvider, &HackClients::WurstProvider::failed, this, &HackClientsPage::onWurstFailed);

    ui->clientList->setCurrentRow(0);
    onClientSelectionChanged();
}

HackClientsPage::~HackClientsPage()
{
    delete ui;
}

void HackClientsPage::retranslate()
{
    ui->retranslateUi(this);
}

int HackClientsPage::currentClientKind() const
{
    auto item = ui->clientList->currentItem();
    return item ? item->data(Qt::UserRole).toInt() : ClientNone;
}

bool HackClientsPage::clientNeedsVersion(int kind) const
{
    return kind == ClientImpact || kind == ClientMeteor || kind == ClientLambda || kind == ClientBaritone ||
           kind == ClientFDP || kind == ClientWurst;
}

bool HackClientsPage::clientVersionReady(int kind) const
{
    if (kind == ClientImpact)
        return m_impactOk;
    if (kind == ClientMeteor)
        return m_meteorOk;
    if (kind == ClientLambda)
        return m_lambdaOk;
    if (kind == ClientBaritone)
        return m_baritoneOk;
    if (kind == ClientFDP)
        return m_fdpOk;
    if (kind == ClientWurst)
        return m_wurstOk;
    return false;
}

void HackClientsPage::openedImpl()
{
    updateStatus();
    m_impactProvider->refresh(false);
    m_lbProvider->refresh(false);
    m_meteorProvider->refresh(false);
    m_lambdaProvider->refresh(false);
    m_baritoneProvider->refresh(false);
    m_fdpProvider->refresh(false);
    m_wurstProvider->refresh(false);
    // Refresh description + OK enablement for the already-selected client (often LiquidBounce).
    onClientSelectionChanged();
}

void HackClientsPage::onRefreshClicked()
{
    m_impactOk = false;
    m_lbOk = false;
    m_meteorOk = false;
    m_lambdaOk = false;
    m_baritoneOk = false;
    m_fdpOk = false;
    m_wurstOk = false;
    m_impactError.clear();
    m_lbError.clear();
    m_meteorError.clear();
    m_lambdaError.clear();
    m_baritoneError.clear();
    m_fdpError.clear();
    m_wurstError.clear();
    updateStatus();
    m_impactProvider->refresh(true);
    m_lbProvider->refresh(true);
    m_meteorProvider->refresh(true);
    m_lambdaProvider->refresh(true);
    m_baritoneProvider->refresh(true);
    m_fdpProvider->refresh(true);
    m_wurstProvider->refresh(true);
}

void HackClientsPage::updateStatus()
{
    QStringList parts;
    if (!m_lbOk && m_lbError.isEmpty())
        parts << tr("LiquidBounce: loading…");
    else if (m_lbOk)
        parts << tr("LiquidBounce: ready");
    else
        parts << tr("LiquidBounce: %1").arg(m_lbError);

    if (!m_meteorOk && m_meteorError.isEmpty())
        parts << tr("Meteor: loading…");
    else if (m_meteorOk)
        parts << tr("Meteor: ready (%1 MC versions)").arg(m_meteorProvider->builds().size());
    else
        parts << tr("Meteor: %1").arg(m_meteorError);

    if (!m_lambdaOk && m_lambdaError.isEmpty())
        parts << tr("Lambda: loading…");
    else if (m_lambdaOk)
        parts << tr("Lambda: ready (%1 MC versions)").arg(m_lambdaProvider->latestStablePerMinecraft().size());
    else
        parts << tr("Lambda: %1").arg(m_lambdaError);

    if (!m_impactOk && m_impactError.isEmpty())
        parts << tr("Impact: loading…");
    else if (m_impactOk)
        parts << tr("Impact: ready (%1 MC versions)").arg(m_impactProvider->latestStablePerMinecraft().size());
    else
        parts << tr("Impact: %1").arg(m_impactError);

    if (!m_baritoneOk && m_baritoneError.isEmpty())
        parts << tr("Baritone: loading…");
    else if (m_baritoneOk)
        parts << tr("Baritone: ready (%1 MC versions)").arg(m_baritoneProvider->releases().size());
    else
        parts << tr("Baritone: %1").arg(m_baritoneError);

    if (!m_fdpOk && m_fdpError.isEmpty())
        parts << tr("FDPClient: loading…");
    else if (m_fdpOk)
        parts << tr("FDPClient: ready (%1 releases)").arg(m_fdpProvider->releases().size());
    else
        parts << tr("FDPClient: %1").arg(m_fdpError);

    if (!m_wurstOk && m_wurstError.isEmpty())
        parts << tr("Wurst: loading…");
    else if (m_wurstOk)
        parts << tr("Wurst: ready (%1 MC versions)").arg(m_wurstProvider->latestStablePerMinecraft().size());
    else
        parts << tr("Wurst: %1").arg(m_wurstError);

    // Use newlines so a long multi-client status never forces the New Instance dialog wider.
    ui->statusLabel->setText(parts.join(QStringLiteral("\n")));
}

void HackClientsPage::onImpactReady()
{
    m_impactOk = true;
    m_impactError.clear();
    updateStatus();
    onClientSelectionChanged();
}

void HackClientsPage::onImpactFailed(QString reason)
{
    m_impactOk = false;
    m_impactError = reason;
    updateStatus();
    onClientSelectionChanged();
}

void HackClientsPage::onLiquidBounceReady()
{
    m_lbOk = true;
    m_lbError.clear();
    updateStatus();
    onClientSelectionChanged();
}

void HackClientsPage::onLiquidBounceFailed(QString reason)
{
    m_lbOk = false;
    m_lbError = reason;
    updateStatus();
    onClientSelectionChanged();
}

void HackClientsPage::onMeteorReady()
{
    m_meteorOk = true;
    m_meteorError.clear();
    updateStatus();
    onClientSelectionChanged();
}

void HackClientsPage::onMeteorFailed(QString reason)
{
    m_meteorOk = false;
    m_meteorError = reason;
    updateStatus();
    onClientSelectionChanged();
}

void HackClientsPage::onLambdaReady()
{
    m_lambdaOk = true;
    m_lambdaError.clear();
    updateStatus();
    onClientSelectionChanged();
}

void HackClientsPage::onLambdaFailed(QString reason)
{
    m_lambdaOk = false;
    m_lambdaError = reason;
    updateStatus();
    onClientSelectionChanged();
}

void HackClientsPage::onBaritoneReady()
{
    m_baritoneOk = true;
    m_baritoneError.clear();
    updateStatus();
    onClientSelectionChanged();
}

void HackClientsPage::onBaritoneFailed(QString reason)
{
    m_baritoneOk = false;
    m_baritoneError = reason;
    updateStatus();
    onClientSelectionChanged();
}

void HackClientsPage::onFDPReady()
{
    m_fdpOk = true;
    m_fdpError.clear();
    updateStatus();
    onClientSelectionChanged();
}

void HackClientsPage::onFDPFailed(QString reason)
{
    m_fdpOk = false;
    m_fdpError = reason;
    updateStatus();
    onClientSelectionChanged();
}

void HackClientsPage::onWurstReady()
{
    m_wurstOk = true;
    m_wurstError.clear();
    updateStatus();
    onClientSelectionChanged();
}

void HackClientsPage::onWurstFailed(QString reason)
{
    m_wurstOk = false;
    m_wurstError = reason;
    updateStatus();
    onClientSelectionChanged();
}

void HackClientsPage::populateVersionCombo()
{
    auto kind = currentClientKind();
    QString previous = ui->versionCombo->currentData().toString();

    ui->versionCombo->blockSignals(true);
    ui->versionCombo->clear();

    if (kind == ClientImpact) {
        for (const auto& rel : m_impactProvider->latestStablePerMinecraft()) {
            ui->versionCombo->addItem(
                QString("%1 (Impact %2)").arg(rel.minecraftVersion, rel.impactVersion), rel.tagName);
        }
        int idx = ui->versionCombo->findData(previous);
        if (idx < 0)
            idx = 0;
        if (ui->versionCombo->count() > 0)
            ui->versionCombo->setCurrentIndex(idx);
    } else if (kind == ClientMeteor) {
        for (const auto& build : m_meteorProvider->builds()) {
            ui->versionCombo->addItem(
                QString("%1 (build %2)").arg(build.minecraftVersion).arg(build.buildNumber),
                build.minecraftVersion);
        }
        int idx = ui->versionCombo->findData(previous);
        if (idx < 0)
            idx = 0;
        if (ui->versionCombo->count() > 0)
            ui->versionCombo->setCurrentIndex(idx);
    } else if (kind == ClientLambda) {
        for (const auto& rel : m_lambdaProvider->latestStablePerMinecraft()) {
            ui->versionCombo->addItem(
                QString("%1 (Lambda %2)").arg(rel.minecraftVersion, rel.lambdaVersion), rel.tagName);
        }
        int idx = ui->versionCombo->findData(previous);
        if (idx < 0)
            idx = 0;
        if (ui->versionCombo->count() > 0)
            ui->versionCombo->setCurrentIndex(idx);
    } else if (kind == ClientBaritone) {
        for (const auto& rel : m_baritoneProvider->releases()) {
            ui->versionCombo->addItem(rel.minecraftVersion, rel.minecraftVersion);
        }
        int idx = ui->versionCombo->findData(previous);
        if (idx < 0)
            idx = 0;
        if (ui->versionCombo->count() > 0)
            ui->versionCombo->setCurrentIndex(idx);
    } else if (kind == ClientFDP) {
        for (const auto& rel : m_fdpProvider->releases()) {
            ui->versionCombo->addItem(rel.instanceVersionLabel(), rel.tagName);
        }
        int idx = ui->versionCombo->findData(previous);
        if (idx < 0)
            idx = 0;
        if (ui->versionCombo->count() > 0)
            ui->versionCombo->setCurrentIndex(idx);
    } else if (kind == ClientWurst) {
        for (const auto& rel : m_wurstProvider->latestStablePerMinecraft()) {
            ui->versionCombo->addItem(
                QString("%1 (Wurst %2)").arg(rel.minecraftVersion, rel.wurstVersion), rel.tagName);
        }
        int idx = ui->versionCombo->findData(previous);
        if (idx < 0)
            idx = 0;
        if (ui->versionCombo->count() > 0)
            ui->versionCombo->setCurrentIndex(idx);
    }

    ui->versionCombo->blockSignals(false);

    bool needsVersion = clientNeedsVersion(kind);
    ui->versionCombo->setEnabled(needsVersion && clientVersionReady(kind) && ui->versionCombo->count() > 0);
}

void HackClientsPage::onClientSelectionChanged()
{
    auto kind = currentClientKind();
    bool needsVersion = clientNeedsVersion(kind);
    ui->versionLabel->setVisible(needsVersion);
    ui->versionCombo->setVisible(needsVersion);
    if (m_wurstBaritoneCheck)
        m_wurstBaritoneCheck->setVisible(kind == ClientWurst);

    populateVersionCombo();

    if (kind == ClientLiquidBounce) {
        if (m_lbOk) {
            auto b = m_lbProvider->latestRelease();
            ui->descriptionBrowser->setHtml(tr(
                "<h3>LiquidBounce %1</h3>"
                "<p>Fabric client for Minecraft <b>%2</b>.</p>"
                "<ul>"
                "<li>Fabric Loader: %3</li>"
                "<li>Fabric API: %4</li>"
                "<li>Fabric Kotlin: %5</li>"
                "<li>Recommended Java: %6</li>"
                "</ul>"
                "<p>Downloads from <a href=\"https://liquidbounce.net/download\">liquidbounce.net</a>. "
                "Using cheat clients on public servers can get your account banned.</p>")
                                                  .arg(b.lbVersion, b.mcVersion, b.fabricLoaderVersion, b.fabricApiVersion,
                                                       b.kotlinModVersion, b.jreVersion > 0 ? QString::number(b.jreVersion) : tr("unknown")));
        } else {
            ui->descriptionBrowser->setHtml(
                tr("<h3>LiquidBounce</h3><p>%1</p>"
                   "<p><a href=\"https://github.com/CCBlueX/LiquidBounce\">GitHub</a> · "
                   "<a href=\"https://liquidbounce.net\">Website</a></p>")
                    .arg(m_lbError.isEmpty() ? tr("Loading metadata…") : m_lbError));
        }
    } else if (kind == ClientMeteor) {
        ui->descriptionBrowser->setHtml(tr(
            "<h3>Meteor Client</h3>"
            "<p>Fabric utility mod for anarchy servers. Creates a Fabric instance and installs "
            "the Meteor JAR (plus Meteor's Baritone fork when available).</p>"
            "<p>Select a Minecraft version below. Builds are listed from "
            "<a href=\"https://meteorclient.com\">meteorclient.com</a>.</p>"
            "<p><a href=\"https://github.com/MeteorDevelopment/meteor-client\">GitHub</a> · "
            "<a href=\"https://meteorclient.com/faq/installation\">Install guide</a></p>"
            "<p>After creating a Meteor instance, use <b>Meteor Addons</b> on the instance Mods tab to install community addons.</p>"
            "<p>Using cheat clients on public servers can get your account banned.</p>"));
    } else if (kind == ClientLambda) {
        ui->descriptionBrowser->setHtml(tr(
            "<h3>Lambda</h3>"
            "<p>Open-source Fabric utility mod (Kotlin rewrite). Creates a Fabric instance and installs "
            "Lambda plus Fabric API, Fabric Language Kotlin, and Baritone API Fabric as documented upstream.</p>"
            "<p>Select a Minecraft version below. Releases are listed from "
            "<a href=\"https://github.com/lambda-client/lambda/releases\">GitHub</a>.</p>"
            "<p>Using cheat clients on public servers can get your account banned.</p>"));
    } else if (kind == ClientImpact) {
        ui->descriptionBrowser->setHtml(tr(
            "<h3>Impact</h3>"
            "<p>Classic anarchy client installed via the official Impact Installer "
            "(MultiMC instance mode).</p>"
            "<p>Select a Minecraft version below. Stable releases are listed from "
            "<a href=\"http://impactclient.net\">impactclient.net</a>.</p>"
            "<p>Using cheat clients on public servers can get your account banned.</p>"));
    } else if (kind == ClientBaritone) {
        ui->descriptionBrowser->setHtml(tr(
            "<h3>Baritone</h3>"
            "<p>Pathfinding mod for Fabric. Creates a Fabric instance with the recommended "
            "Fabric Loader and downloads Baritone for the selected Minecraft version.</p>"
            "<p>Builds are fetched from "
            "<a href=\"https://maven.2b2t.vc/releases/com/github/rfresh2/baritone-fabric/\">"
            "rfresh2's Baritone Maven</a> (same builds Lambda and many Fabric clients use).</p>"
            "<p>You can also install Baritone into an existing Fabric instance from that instance's "
            "<b>Mods</b> tab.</p>"));
    } else if (kind == ClientFDP) {
        ui->descriptionBrowser->setHtml(tr(
            "<h3>FDPClient</h3>"
            "<p>Forge mixin client for Minecraft <b>1.8.9</b> (LiquidBounce legacy fork). "
            "Creates a Forge instance and installs the release JAR into <code>mods/</code>.</p>"
            "<p>Select a release below. Builds are listed from "
            "<a href=\"https://github.com/SkidderMC/FDPClient/releases\">GitHub</a>. "
            "Java 8 is recommended.</p>"
            "<p><a href=\"https://fdpinfo.github.io\">Website</a> · "
            "<a href=\"https://github.com/SkidderMC/FDPClient\">GitHub</a></p>"
            "<p>Using cheat clients on public servers can get your account banned.</p>"));
    } else if (kind == ClientWurst) {
        ui->descriptionBrowser->setHtml(tr(
            "<h3>Wurst Client</h3>"
            "<p>Fabric hacked client (v7). Creates a Fabric instance and installs "
            "the Wurst JAR plus Fabric API as documented upstream.</p>"
            "<p>Select a Minecraft version below. JARs are listed from "
            "<a href=\"https://github.com/Wurst-Imperium/Wurst-MCX2/releases\">GitHub</a>; "
            "Fabric Loader and Fabric API versions come from the matching "
            "<a href=\"https://github.com/Wurst-Imperium/Wurst7\">Wurst7</a> tag.</p>"
            "<p><a href=\"https://www.wurstclient.net/\">Website</a> · "
            "<a href=\"https://www.wurstclient.net/tutorials/how-to-install/\">Install guide</a></p>"
            "<p>Optional Baritone can be added from the checkbox below (same standalone Fabric build as the Mods tab).</p>"
            "<p>Using cheat clients on public servers can get your account banned.</p>"));
    } else {
        ui->descriptionBrowser->clear();
    }

    suggestCurrent();
}

void HackClientsPage::onVersionChanged(int)
{
    suggestCurrent();
}

void HackClientsPage::suggestCurrent()
{
    if (!isOpened)
        return;

    auto item = ui->clientList->currentItem();
    if (!item) {
        dialog->setSuggestedPack();
        return;
    }

    auto kind = item->data(Qt::UserRole).toInt();
    if (kind == ClientLiquidBounce) {
        if (!m_lbOk) {
            dialog->setSuggestedPack();
            return;
        }
        auto build = m_lbProvider->latestRelease();
        auto* task = new HackClients::LiquidBounceInstallTask(build);
        dialog->setSuggestedPack(QStringLiteral("LiquidBounce"), build.lbVersion, task);
        dialog->setSuggestedIcon(QStringLiteral("liquidbounce"));
        dialog->setSuggestedGroup(QStringLiteral("LiquidBounce"));
    } else if (kind == ClientMeteor) {
        if (!m_meteorOk || ui->versionCombo->currentIndex() < 0) {
            dialog->setSuggestedPack();
            return;
        }
        QString mcVersion = ui->versionCombo->currentData().toString();
        auto build = m_meteorProvider->buildForMinecraft(mcVersion);
        if (build.minecraftVersion.isEmpty()) {
            dialog->setSuggestedPack();
            return;
        }
        auto* task = new HackClients::MeteorInstallTask(build);
        dialog->setSuggestedPack(QStringLiteral("Meteor"), build.instanceVersionLabel(), task);
        dialog->setSuggestedIcon(QStringLiteral("meteor"));
        dialog->setSuggestedGroup(QStringLiteral("Meteor"));
    } else if (kind == ClientLambda) {
        if (!m_lambdaOk || ui->versionCombo->currentIndex() < 0) {
            dialog->setSuggestedPack();
            return;
        }
        QString tag = ui->versionCombo->currentData().toString();
        auto release = m_lambdaProvider->releaseForTag(tag);
        if (release.tagName.isEmpty()) {
            dialog->setSuggestedPack();
            return;
        }
        auto* task = new HackClients::LambdaInstallTask(release);
        dialog->setSuggestedPack(QStringLiteral("Lambda"), release.instanceVersionLabel(), task);
        dialog->setSuggestedIcon(QStringLiteral("lambda"));
        dialog->setSuggestedGroup(QStringLiteral("Lambda"));
    } else if (kind == ClientImpact) {
        if (!m_impactOk || ui->versionCombo->currentIndex() < 0) {
            dialog->setSuggestedPack();
            return;
        }
        QString tag = ui->versionCombo->currentData().toString();
        HackClients::ImpactRelease release;
        for (const auto& r : m_impactProvider->releases(false)) {
            if (r.tagName == tag) {
                release = r;
                break;
            }
        }
        if (release.tagName.isEmpty()) {
            dialog->setSuggestedPack();
            return;
        }
        auto* task = new HackClients::ImpactInstallTask(release);
        dialog->setSuggestedPack(QStringLiteral("Impact"), release.instanceVersionLabel(), task);
        dialog->setSuggestedIcon(QStringLiteral("impact"));
        dialog->setSuggestedGroup(QStringLiteral("Impact"));
    } else if (kind == ClientBaritone) {
        if (!m_baritoneOk || ui->versionCombo->currentIndex() < 0) {
            dialog->setSuggestedPack();
            return;
        }
        QString mcVersion = ui->versionCombo->currentData().toString();
        auto release = m_baritoneProvider->releaseForMinecraft(mcVersion);
        if (release.minecraftVersion.isEmpty()) {
            dialog->setSuggestedPack();
            return;
        }
        auto* task = new HackClients::BaritoneInstallTask(release);
        dialog->setSuggestedPack(QStringLiteral("Baritone"), release.minecraftVersion, task);
        dialog->setSuggestedIcon(QStringLiteral("loadermods"));
        dialog->setSuggestedGroup(QStringLiteral("Baritone"));
    } else if (kind == ClientFDP) {
        if (!m_fdpOk || ui->versionCombo->currentIndex() < 0) {
            dialog->setSuggestedPack();
            return;
        }
        QString tag = ui->versionCombo->currentData().toString();
        auto release = m_fdpProvider->releaseForTag(tag);
        if (release.tagName.isEmpty()) {
            dialog->setSuggestedPack();
            return;
        }
        auto* task = new HackClients::FDPInstallTask(release);
        dialog->setSuggestedPack(QStringLiteral("FDPClient"), release.instanceVersionLabel(), task);
        dialog->setSuggestedIcon(QStringLiteral("fdp"));
        dialog->setSuggestedGroup(QStringLiteral("FDPClient"));
    } else if (kind == ClientWurst) {
        if (!m_wurstOk || ui->versionCombo->currentIndex() < 0) {
            dialog->setSuggestedPack();
            return;
        }
        QString tag = ui->versionCombo->currentData().toString();
        auto release = m_wurstProvider->releaseForTag(tag);
        if (release.tagName.isEmpty()) {
            dialog->setSuggestedPack();
            return;
        }
        auto* task = new HackClients::WurstInstallTask(release, m_wurstBaritoneCheck && m_wurstBaritoneCheck->isChecked());
        dialog->setSuggestedPack(QStringLiteral("Wurst"), release.instanceVersionLabel(), task);
        dialog->setSuggestedIcon(QStringLiteral("wurst"));
        dialog->setSuggestedGroup(QStringLiteral("Wurst"));
    } else {
        dialog->setSuggestedPack();
    }
}
