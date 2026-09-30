// SPDX-License-Identifier: GPL-3.0-only
#include "HackClientsPage.h"
#include "ui_HackClientsPage.h"

#include "Application.h"
#include "icons/IconList.h"
#include "ui/dialogs/NewInstanceDialog.h"

#include "hackclients/BaritoneInstallTask.h"
#include "hackclients/EpsilonInstallTask.h"
#include "hackclients/FDPInstallTask.h"
#include "hackclients/ImpactInstallTask.h"
#include "hackclients/LambdaInstallTask.h"
#include "hackclients/LiquidBounceInstallTask.h"
#include "hackclients/MeteorInstallTask.h"
#include "hackclients/WurstInstallTask.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLayout>
#include <QLayoutItem>
#include <QPushButton>
#include <QScrollArea>
#include <QSet>
#include <QSignalBlocker>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>
#include <type_traits>
#include <utility>

namespace {

class FlowLayout : public QLayout {
   public:
    explicit FlowLayout(QWidget* parent = nullptr, int margin = 0, int hSpacing = 8, int vSpacing = 8)
        : QLayout(parent), m_hSpace(hSpacing), m_vSpace(vSpacing)
    {
        setContentsMargins(margin, margin, margin, margin);
    }
    ~FlowLayout() override { qDeleteAll(m_items); }

    void addItem(QLayoutItem* item) override { m_items.append(item); }
    int count() const override { return m_items.size(); }
    QLayoutItem* itemAt(int index) const override { return m_items.value(index); }
    QLayoutItem* takeAt(int index) override
    {
        return (index >= 0 && index < m_items.size()) ? m_items.takeAt(index) : nullptr;
    }
    Qt::Orientations expandingDirections() const override { return {}; }
    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int width) const override { return doLayout(QRect(0, 0, width, 0), true); }
    void setGeometry(const QRect& rect) override
    {
        QLayout::setGeometry(rect);
        doLayout(rect, false);
    }
    QSize sizeHint() const override { return minimumSize(); }
    QSize minimumSize() const override
    {
        QSize size;
        for (auto* item : m_items)
            size = size.expandedTo(item->minimumSize());
        const auto margins = contentsMargins();
        size += QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
        return size;
    }

   private:
    int doLayout(const QRect& rect, bool testOnly) const
    {
        int x = rect.x();
        int y = rect.y();
        int lineHeight = 0;
        for (auto* item : m_items) {
            const QSize hint = item->sizeHint();
            int nextX = x + hint.width() + m_hSpace;
            const int right = rect.x() + rect.width();
            if (nextX - m_hSpace > right && lineHeight > 0) {
                x = rect.x();
                y = y + lineHeight + m_vSpace;
                nextX = x + hint.width() + m_hSpace;
                lineHeight = 0;
            }
            if (!testOnly)
                item->setGeometry(QRect(QPoint(x, y), hint));
            x = nextX;
            lineHeight = qMax(lineHeight, hint.height());
        }
        return y + lineHeight - rect.y();
    }

    QList<QLayoutItem*> m_items;
    int m_hSpace;
    int m_vSpace;
};

}  // namespace

HackClientsPageContext::HackClientsPageContext(QObject* parent) : QObject(parent)
{
    auto* network = APPLICATION->network();
    impactProvider = new HackClients::ImpactReleasesProvider(network, this);
    lbProvider = new HackClients::LiquidBounceProvider(network, this);
    meteorProvider = new HackClients::MeteorProvider(network, this);
    lambdaProvider = new HackClients::LambdaProvider(network, this);
    baritoneProvider = new HackClients::BaritoneProvider(network, this);
    fdpProvider = new HackClients::FDPProvider(network, this);
    wurstProvider = new HackClients::WurstProvider(network, this);
    epsilonProvider = new HackClients::EpsilonProvider(network, this);

    auto bind = [this](int clientId, auto* provider) {
        connect(provider, &std::remove_pointer_t<decltype(provider)>::refreshed, this, [this, clientId]() {
            setStatus(clientId, true);
            emit statusChanged();
        });
        connect(provider, &std::remove_pointer_t<decltype(provider)>::failed, this, [this, clientId](QString reason) {
            setStatus(clientId, false, std::move(reason));
            emit statusChanged();
        });
    };
    bind(HackClients::ClientImpact, impactProvider);
    bind(HackClients::ClientLiquidBounce, lbProvider);
    bind(HackClients::ClientMeteor, meteorProvider);
    bind(HackClients::ClientLambda, lambdaProvider);
    bind(HackClients::ClientBaritone, baritoneProvider);
    bind(HackClients::ClientFDP, fdpProvider);
    bind(HackClients::ClientWurst, wurstProvider);
    bind(HackClients::ClientEpsilon, epsilonProvider);
}

void HackClientsPageContext::refresh(HackClients::HackClientCategory category, bool force)
{
    QSet<int> ids;
    for (const auto& entry : HackClients::hackClientsForCategory(category))
        ids.insert(entry.clientId);

    auto maybeRefresh = [this, force](int clientId, const auto& refreshFn) {
        if (force)
            setStatus(clientId, false);
        refreshFn(force);
    };

    using namespace HackClients;
    if (ids.contains(ClientImpact))
        maybeRefresh(ClientImpact, [this](bool f) { impactProvider->refresh(f); });
    if (ids.contains(ClientLiquidBounce))
        maybeRefresh(ClientLiquidBounce, [this](bool f) { lbProvider->refresh(f); });
    if (ids.contains(ClientMeteor))
        maybeRefresh(ClientMeteor, [this](bool f) { meteorProvider->refresh(f); });
    if (ids.contains(ClientLambda))
        maybeRefresh(ClientLambda, [this](bool f) { lambdaProvider->refresh(f); });
    if (ids.contains(ClientBaritone))
        maybeRefresh(ClientBaritone, [this](bool f) { baritoneProvider->refresh(f); });
    if (ids.contains(ClientFDP))
        maybeRefresh(ClientFDP, [this](bool f) { fdpProvider->refresh(f); });
    if (ids.contains(ClientWurst))
        maybeRefresh(ClientWurst, [this](bool f) { wurstProvider->refresh(f); });
    if (ids.contains(ClientEpsilon))
        maybeRefresh(ClientEpsilon, [this](bool f) { epsilonProvider->refresh(f); });

    emit statusChanged();
}

HackClientsPage::HackClientsPage(NewInstanceDialog* dialog,
                                 HackClients::HackClientCategory category,
                                 HackClientsPageContext* context,
                                 QWidget* parent)
    : QWidget(parent), dialog(dialog), ui(new Ui::HackClientsPage), m_category(category), m_context(context)
{
    ui->setupUi(this);

    connect(ui->versionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &HackClientsPage::onVersionChanged);
    connect(ui->refreshButton, &QPushButton::clicked, this, &HackClientsPage::onRefreshClicked);
    connect(m_context, &HackClientsPageContext::statusChanged, this, &HackClientsPage::onCatalogStatusChanged);

    ui->refreshButton->setIcon(APPLICATION->getThemedIcon(QStringLiteral("refresh")));
    ui->refreshButton->setIconSize(QSize(18, 18));
    ui->descriptionBrowser->setObjectName(QStringLiteral("hackClientDescription"));
    ui->descriptionBrowser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->statusLabel->setMinimumWidth(0);
    ui->versionCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->versionCombo->setMinimumContentsLength(12);
    ui->versionCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);

    ui->clientCatalog->setWidgetResizable(true);
    m_catalogHost = new QWidget(ui->clientCatalog);
    auto* catalogLayout = new QVBoxLayout(m_catalogHost);
    catalogLayout->setContentsMargins(6, 6, 10, 14);
    catalogLayout->setSpacing(8);
    catalogLayout->addStretch(1);
    ui->clientCatalog->setWidget(m_catalogHost);
    ui->contentLayout->setStretch(0, 1);
    ui->contentLayout->setStretch(1, 0);

    m_wurstBaritoneCheck = new QCheckBox(tr("Also install Baritone (pathfinding)"), this);
    m_wurstBaritoneCheck->setToolTip(tr("Adds standalone Fabric Baritone to the new instance's mods folder."));
    m_wurstBaritoneCheck->setChecked(true);
    m_wurstBaritoneCheck->setVisible(false);
    ui->detailLayout->addWidget(m_wurstBaritoneCheck);
    connect(m_wurstBaritoneCheck, &QCheckBox::toggled, this, [this](bool) { suggestCurrent(); });

    populateClientList();
    selectFirstClient();
    onClientSelectionChanged();
}

HackClientsPage::~HackClientsPage()
{
    delete ui;
}

QIcon HackClientsPage::iconForCatalogEntry(const HackClients::HackClientCatalogEntry& entry) const
{
    if (entry.iconKey.isEmpty() || entry.iconKey == QLatin1String("loadermods"))
        return APPLICATION->getThemedIcon(QStringLiteral("loadermods"));
    return APPLICATION->icons()->getIcon(entry.iconKey);
}

void HackClientsPage::populateClientList()
{
    m_updatingSelection = true;
    m_clientCards.clear();

    auto* catalogLayout = qobject_cast<QVBoxLayout*>(m_catalogHost->layout());
    if (!catalogLayout)
        return;

    while (catalogLayout->count() > 0) {
        QLayoutItem* item = catalogLayout->takeAt(0);
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }

    auto* row = new QWidget(m_catalogHost);
    row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    auto* flow = new FlowLayout(row, 4, 12, 12);
    for (const auto& entry : HackClients::hackClientsForCategory(m_category)) {
        auto* card = new QToolButton(row);
        card->setObjectName(QStringLiteral("hackClientCard"));
        card->setCheckable(true);
        card->setAutoExclusive(false);
        card->setFocusPolicy(Qt::NoFocus);
        card->setAttribute(Qt::WA_MacShowFocusRect, false);
        card->setCursor(Qt::PointingHandCursor);
        card->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        card->setIconSize(QSize(44, 44));
        card->setIcon(iconForCatalogEntry(entry));
        card->setText(entry.displayName);
        card->setProperty("clientId", entry.clientId);
        card->setFixedSize(128, 122);
        connect(card, &QToolButton::clicked, this, &HackClientsPage::onCardClicked);
        flow->addWidget(card);
        m_clientCards.append(card);
    }
    catalogLayout->addWidget(row);

    catalogLayout->addStretch(1);
    m_updatingSelection = false;
    syncCardSelection();
}

void HackClientsPage::selectFirstClient()
{
    for (auto* card : m_clientCards) {
        const int kind = card->property("clientId").toInt();
        if (kind != HackClients::ClientNone) {
            m_lastClientKind = kind;
            syncCardSelection();
            return;
        }
    }
}

bool HackClientsPage::selectClientKind(int kind)
{
    if (kind == HackClients::ClientNone)
        return false;
    for (auto* card : m_clientCards) {
        if (card->property("clientId").toInt() == kind) {
            m_lastClientKind = kind;
            syncCardSelection();
            return true;
        }
    }
    return false;
}

void HackClientsPage::onCardClicked()
{
    auto* card = qobject_cast<QToolButton*>(sender());
    if (!card)
        return;
    const int kind = card->property("clientId").toInt();
    if (kind == HackClients::ClientNone)
        return;
    m_lastClientKind = kind;
    syncCardSelection();
    onClientSelectionChanged();
}

void HackClientsPage::syncCardSelection()
{
    const int selected = m_lastClientKind;
    for (auto* card : m_clientCards) {
        QSignalBlocker blocker(card);
        card->setChecked(card->property("clientId").toInt() == selected && selected != HackClients::ClientNone);
    }
}

void HackClientsPage::retranslate()
{
    ui->retranslateUi(this);
    const int keep = m_lastClientKind;
    populateClientList();
    if (!selectClientKind(keep))
        selectFirstClient();
}

int HackClientsPage::currentClientKind() const
{
    return m_lastClientKind;
}

bool HackClientsPage::clientNeedsVersion(int kind) const
{
    return kind == HackClients::ClientImpact || kind == HackClients::ClientMeteor || kind == HackClients::ClientLambda || kind == HackClients::ClientBaritone ||
           kind == HackClients::ClientFDP || kind == HackClients::ClientWurst || kind == HackClients::ClientEpsilon;
}

bool HackClientsPage::clientVersionReady(int kind) const
{
    return kind != HackClients::ClientNone && m_context->isOk(kind);
}

void HackClientsPage::openedImpl()
{
    m_context->refresh(m_category, false);
}

void HackClientsPage::onRefreshClicked()
{
    m_context->refresh(m_category, true);
}

void HackClientsPage::onCatalogStatusChanged()
{
    if (!isOpened)
        return;
    updateStatus();
    onClientSelectionChanged();
}

void HackClientsPage::updateStatus()
{
    QStringList parts;
    for (const auto& entry : HackClients::hackClientsForCategory(m_category)) {
        const QString line = statusLineForKind(entry.clientId);
        if (!line.isEmpty())
            parts << line;
    }

    // Use newlines so a long multi-client status never forces the New Instance dialog wider.
    ui->statusLabel->setText(parts.join(QStringLiteral("\n")));
}

QString HackClientsPage::statusLineForKind(int kind) const
{
    const bool ok = m_context->isOk(kind);
    const QString error = m_context->errorFor(kind);
    const bool loading = !ok && error.isEmpty();

    auto named = [&](const QString& name, const QString& readyDetail = {}) {
        if (loading)
            return tr("%1: loading…").arg(name);
        if (ok)
            return readyDetail.isEmpty() ? tr("%1: ready").arg(name) : tr("%1: ready (%2)").arg(name, readyDetail);
        return tr("%1: %2").arg(name, error);
    };

    switch (kind) {
        case HackClients::ClientLiquidBounce:
            return named(tr("LiquidBounce"));
        case HackClients::ClientMeteor:
            return named(tr("Meteor"), tr("%n MC version(s)", "", m_context->meteorProvider->builds().size()));
        case HackClients::ClientLambda:
            return named(tr("Lambda"),
                         tr("%n MC version(s)", "", m_context->lambdaProvider->latestStablePerMinecraft().size()));
        case HackClients::ClientImpact:
            return named(tr("Impact"),
                         tr("%n MC version(s)", "", m_context->impactProvider->latestStablePerMinecraft().size()));
        case HackClients::ClientBaritone:
            return named(tr("Baritone"), tr("%n MC version(s)", "", m_context->baritoneProvider->releases().size()));
        case HackClients::ClientFDP:
            return named(tr("FDPClient"), tr("%n release(s)", "", m_context->fdpProvider->releases().size()));
        case HackClients::ClientWurst:
            return named(tr("Wurst"),
                         tr("%n MC version(s)", "", m_context->wurstProvider->latestStablePerMinecraft().size()));
        case HackClients::ClientEpsilon:
            return named(tr("Epsilon"),
                         tr("%n MC version(s)", "", m_context->epsilonProvider->latestStablePerMinecraft().size()));
        default:
            return {};
    }
}

void HackClientsPage::populateVersionCombo()
{
    auto kind = currentClientKind();
    QString previous = ui->versionCombo->currentData().toString();

    ui->versionCombo->blockSignals(true);
    ui->versionCombo->clear();

    if (kind == HackClients::ClientImpact) {
        for (const auto& rel : m_context->impactProvider->latestStablePerMinecraft()) {
            ui->versionCombo->addItem(
                QString("%1 (Impact %2)").arg(rel.minecraftVersion, rel.impactVersion), rel.tagName);
        }
        int idx = ui->versionCombo->findData(previous);
        if (idx < 0)
            idx = 0;
        if (ui->versionCombo->count() > 0)
            ui->versionCombo->setCurrentIndex(idx);
    } else if (kind == HackClients::ClientMeteor) {
        for (const auto& build : m_context->meteorProvider->builds()) {
            ui->versionCombo->addItem(
                QString("%1 (build %2)").arg(build.minecraftVersion).arg(build.buildNumber),
                build.minecraftVersion);
        }
        int idx = ui->versionCombo->findData(previous);
        if (idx < 0)
            idx = 0;
        if (ui->versionCombo->count() > 0)
            ui->versionCombo->setCurrentIndex(idx);
    } else if (kind == HackClients::ClientLambda) {
        for (const auto& rel : m_context->lambdaProvider->latestStablePerMinecraft()) {
            ui->versionCombo->addItem(
                QString("%1 (Lambda %2)").arg(rel.minecraftVersion, rel.lambdaVersion), rel.tagName);
        }
        int idx = ui->versionCombo->findData(previous);
        if (idx < 0)
            idx = 0;
        if (ui->versionCombo->count() > 0)
            ui->versionCombo->setCurrentIndex(idx);
    } else if (kind == HackClients::ClientBaritone) {
        for (const auto& rel : m_context->baritoneProvider->releases()) {
            ui->versionCombo->addItem(rel.minecraftVersion, rel.minecraftVersion);
        }
        int idx = ui->versionCombo->findData(previous);
        if (idx < 0)
            idx = 0;
        if (ui->versionCombo->count() > 0)
            ui->versionCombo->setCurrentIndex(idx);
    } else if (kind == HackClients::ClientFDP) {
        for (const auto& rel : m_context->fdpProvider->releases()) {
            ui->versionCombo->addItem(rel.instanceVersionLabel(), rel.tagName);
        }
        int idx = ui->versionCombo->findData(previous);
        if (idx < 0)
            idx = 0;
        if (ui->versionCombo->count() > 0)
            ui->versionCombo->setCurrentIndex(idx);
    } else if (kind == HackClients::ClientWurst) {
        for (const auto& rel : m_context->wurstProvider->latestStablePerMinecraft()) {
            ui->versionCombo->addItem(
                QString("%1 (Wurst %2)").arg(rel.minecraftVersion, rel.wurstVersion), rel.tagName);
        }
        int idx = ui->versionCombo->findData(previous);
        if (idx < 0)
            idx = 0;
        if (ui->versionCombo->count() > 0)
            ui->versionCombo->setCurrentIndex(idx);
    } else if (kind == HackClients::ClientEpsilon) {
        for (const auto& rel : m_context->epsilonProvider->latestStablePerMinecraft()) {
            ui->versionCombo->addItem(
                QString("%1 (Epsilon %2)").arg(rel.minecraftVersion, rel.epsilonVersion), rel.selectionKey());
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
    if (m_updatingSelection)
        return;

    auto kind = currentClientKind();
    if (kind == HackClients::ClientNone) {
        if (!selectClientKind(m_lastClientKind))
            selectFirstClient();
        kind = currentClientKind();
        if (kind == HackClients::ClientNone) {
            dialog->setSuggestedPack();
            ui->descriptionBrowser->clear();
            return;
        }
    }
    m_lastClientKind = kind;

    bool needsVersion = clientNeedsVersion(kind);
    ui->versionLabel->setVisible(needsVersion);
    ui->versionCombo->setVisible(needsVersion);
    if (m_wurstBaritoneCheck)
        m_wurstBaritoneCheck->setVisible(kind == HackClients::ClientWurst);

    populateVersionCombo();

    if (kind == HackClients::ClientLiquidBounce) {
        if (m_context->isOk(HackClients::ClientLiquidBounce)) {
            auto b = m_context->lbProvider->latestRelease();
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
                    .arg(m_context->errorFor(HackClients::ClientLiquidBounce).isEmpty() ? tr("Loading metadata…") : m_context->errorFor(HackClients::ClientLiquidBounce)));
        }
    } else if (kind == HackClients::ClientMeteor) {
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
    } else if (kind == HackClients::ClientLambda) {
        ui->descriptionBrowser->setHtml(tr(
            "<h3>Lambda</h3>"
            "<p>Open-source Fabric utility mod (Kotlin rewrite). Creates a Fabric instance and installs "
            "Lambda plus Fabric API, Fabric Language Kotlin, and Baritone API Fabric as documented upstream.</p>"
            "<p>Select a Minecraft version below. Releases are listed from "
            "<a href=\"https://github.com/lambda-client/lambda/releases\">GitHub</a>.</p>"
            "<p>Using cheat clients on public servers can get your account banned.</p>"));
    } else if (kind == HackClients::ClientImpact) {
        ui->descriptionBrowser->setHtml(tr(
            "<h3>Impact</h3>"
            "<p>Classic anarchy client installed via the official Impact Installer "
            "(MultiMC instance mode).</p>"
            "<p>Select a Minecraft version below. Stable releases are listed from "
            "<a href=\"http://impactclient.net\">impactclient.net</a>.</p>"
            "<p>Using cheat clients on public servers can get your account banned.</p>"));
    } else if (kind == HackClients::ClientBaritone) {
        ui->descriptionBrowser->setHtml(tr(
            "<h3>Baritone</h3>"
            "<p>Pathfinding mod for Fabric. Creates a Fabric instance with the recommended "
            "Fabric Loader and downloads Baritone for the selected Minecraft version.</p>"
            "<p>Builds are fetched from "
            "<a href=\"https://maven.2b2t.vc/releases/com/github/rfresh2/baritone-fabric/\">"
            "rfresh2's Baritone Maven</a> (same builds Lambda and many Fabric clients use).</p>"
            "<p>You can also install Baritone into an existing Fabric instance from that instance's "
            "<b>Mods</b> tab. On Meteor instances that installs Meteor's <code>baritone-meteor</code> "
            "fork instead of standalone Fabric Baritone.</p>"));
    } else if (kind == HackClients::ClientFDP) {
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
    } else if (kind == HackClients::ClientWurst) {
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
    } else if (kind == HackClients::ClientEpsilon) {
        ui->descriptionBrowser->setHtml(tr(
            "<h3>Epsilon</h3>"
            "<p>Open-source Fabric utility client (GPL-3.0). Creates a Fabric instance and installs "
            "the Epsilon Fabric JAR plus Fabric API from Modrinth.</p>"
            "<p>Select a Minecraft version below. Public releases are listed from "
            "<a href=\"https://github.com/NekoyaHouse/Epsilon/releases\">GitHub</a>. "
            "Upstream development is currently paused; use a recent Java runtime as required by the build.</p>"
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

    auto kind = currentClientKind();
    if (kind == HackClients::ClientNone) {
        dialog->setSuggestedPack();
        return;
    }
    if (kind == HackClients::ClientLiquidBounce) {
        if (!m_context->isOk(HackClients::ClientLiquidBounce)) {
            dialog->setSuggestedPack();
            return;
        }
        auto build = m_context->lbProvider->latestRelease();
        auto* task = new HackClients::LiquidBounceInstallTask(build);
        dialog->setSuggestedPack(QStringLiteral("LiquidBounce"), build.lbVersion, task);
        dialog->setSuggestedIcon(QStringLiteral("liquidbounce"));
        dialog->setSuggestedGroup(QStringLiteral("LiquidBounce"));
    } else if (kind == HackClients::ClientMeteor) {
        if (!m_context->isOk(HackClients::ClientMeteor) || ui->versionCombo->currentIndex() < 0) {
            dialog->setSuggestedPack();
            return;
        }
        QString mcVersion = ui->versionCombo->currentData().toString();
        auto build = m_context->meteorProvider->buildForMinecraft(mcVersion);
        if (build.minecraftVersion.isEmpty()) {
            dialog->setSuggestedPack();
            return;
        }
        auto* task = new HackClients::MeteorInstallTask(build);
        dialog->setSuggestedPack(QStringLiteral("Meteor"), build.instanceVersionLabel(), task);
        dialog->setSuggestedIcon(QStringLiteral("meteor"));
        dialog->setSuggestedGroup(QStringLiteral("Meteor"));
    } else if (kind == HackClients::ClientLambda) {
        if (!m_context->isOk(HackClients::ClientLambda) || ui->versionCombo->currentIndex() < 0) {
            dialog->setSuggestedPack();
            return;
        }
        QString tag = ui->versionCombo->currentData().toString();
        auto release = m_context->lambdaProvider->releaseForTag(tag);
        if (release.tagName.isEmpty()) {
            dialog->setSuggestedPack();
            return;
        }
        auto* task = new HackClients::LambdaInstallTask(release);
        dialog->setSuggestedPack(QStringLiteral("Lambda"), release.instanceVersionLabel(), task);
        dialog->setSuggestedIcon(QStringLiteral("lambda"));
        dialog->setSuggestedGroup(QStringLiteral("Lambda"));
    } else if (kind == HackClients::ClientImpact) {
        if (!m_context->isOk(HackClients::ClientImpact) || ui->versionCombo->currentIndex() < 0) {
            dialog->setSuggestedPack();
            return;
        }
        QString tag = ui->versionCombo->currentData().toString();
        HackClients::ImpactRelease release;
        for (const auto& r : m_context->impactProvider->releases(false)) {
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
    } else if (kind == HackClients::ClientBaritone) {
        if (!m_context->isOk(HackClients::ClientBaritone) || ui->versionCombo->currentIndex() < 0) {
            dialog->setSuggestedPack();
            return;
        }
        QString mcVersion = ui->versionCombo->currentData().toString();
        auto release = m_context->baritoneProvider->releaseForMinecraft(mcVersion);
        if (release.minecraftVersion.isEmpty()) {
            dialog->setSuggestedPack();
            return;
        }
        auto* task = new HackClients::BaritoneInstallTask(release);
        dialog->setSuggestedPack(QStringLiteral("Baritone"), release.minecraftVersion, task);
        dialog->setSuggestedIcon(QStringLiteral("loadermods"));
        dialog->setSuggestedGroup(QStringLiteral("Baritone"));
    } else if (kind == HackClients::ClientFDP) {
        if (!m_context->isOk(HackClients::ClientFDP) || ui->versionCombo->currentIndex() < 0) {
            dialog->setSuggestedPack();
            return;
        }
        QString tag = ui->versionCombo->currentData().toString();
        auto release = m_context->fdpProvider->releaseForTag(tag);
        if (release.tagName.isEmpty()) {
            dialog->setSuggestedPack();
            return;
        }
        auto* task = new HackClients::FDPInstallTask(release);
        dialog->setSuggestedPack(QStringLiteral("FDPClient"), release.instanceVersionLabel(), task);
        dialog->setSuggestedIcon(QStringLiteral("fdp"));
        dialog->setSuggestedGroup(QStringLiteral("FDPClient"));
    } else if (kind == HackClients::ClientWurst) {
        if (!m_context->isOk(HackClients::ClientWurst) || ui->versionCombo->currentIndex() < 0) {
            dialog->setSuggestedPack();
            return;
        }
        QString tag = ui->versionCombo->currentData().toString();
        auto release = m_context->wurstProvider->releaseForTag(tag);
        if (release.tagName.isEmpty()) {
            dialog->setSuggestedPack();
            return;
        }
        auto* task = new HackClients::WurstInstallTask(release, m_wurstBaritoneCheck && m_wurstBaritoneCheck->isChecked());
        dialog->setSuggestedPack(QStringLiteral("Wurst"), release.instanceVersionLabel(), task);
        dialog->setSuggestedIcon(QStringLiteral("wurst"));
        dialog->setSuggestedGroup(QStringLiteral("Wurst"));
    } else if (kind == HackClients::ClientEpsilon) {
        if (!m_context->isOk(HackClients::ClientEpsilon) || ui->versionCombo->currentIndex() < 0) {
            dialog->setSuggestedPack();
            return;
        }
        QString key = ui->versionCombo->currentData().toString();
        auto release = m_context->epsilonProvider->releaseForSelectionKey(key);
        if (release.tagName.isEmpty()) {
            dialog->setSuggestedPack();
            return;
        }
        auto* task = new HackClients::EpsilonInstallTask(release);
        dialog->setSuggestedPack(QStringLiteral("Epsilon"), release.instanceVersionLabel(), task);
        dialog->setSuggestedIcon(QStringLiteral("epsilon"));
        dialog->setSuggestedGroup(QStringLiteral("Epsilon"));
    } else {
        dialog->setSuggestedPack();
    }
}
