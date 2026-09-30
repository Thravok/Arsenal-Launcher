// SPDX-License-Identifier: GPL-3.0-only
#include "MeteorAddonsDialog.h"

#include "Application.h"
#include "FileSystem.h"
#include "settings/SettingsObject.h"
#include "hackclients/MeteorAddonInstallTask.h"
#include "tasks/ConcurrentTask.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ProgressDialog.h"
#include "ui/dialogs/ReviewMessageBox.h"
#include "ui/widgets/ProjectItem.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QStandardItemModel>
#include <QTextBrowser>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>

namespace {
QString jarFileNameFor(const HackClients::MeteorAddonEntry& entry)
{
    const QString url = entry.pickJarDownloadUrl();
    if (url.isEmpty())
        return {};
    return QUrl(url).fileName();
}
}  // namespace

MeteorAddonsDialog::MeteorAddonsDialog(const QString& minecraftVersion, const QString& modsDir, QWidget* parent)
    : QDialog(parent), m_minecraftVersion(minecraftVersion), m_modsDir(modsDir)
{
    setWindowTitle(tr("Meteor Addons"));
    setWindowIcon(QIcon::fromTheme("new"));

    // If created with an embedded page as parent, reparent onto the top-level window so
    // the dialog is a real window (macOS sheets / modality are unreliable otherwise).
    if (parent && parent->window() && parent != parent->window())
        setParent(parent->window(), windowFlags());

    if (auto* anchor = parentWidget() ? parentWidget() : parent) {
        resize(static_cast<int>(std::max(0.5 * anchor->width(), 640.0)),
               static_cast<int>(std::max(0.75 * anchor->height(), 480.0)));
    } else {
        resize(720, 540);
    }

    auto* root = new QVBoxLayout(this);
#ifndef Q_OS_MACOS
    root->setContentsMargins(0, 0, 0, 0);
#endif

    auto* content = new QWidget(this);
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(12, 12, 12, 8);

    auto* warning = new QLabel(
        tr("Third-party Meteor addons from the community catalog (meteoraddons.com). "
           "Only install addons you trust; jars run with full game access."),
        content);
    warning->setWordWrap(true);
    contentLayout->addWidget(warning);

    auto* filterRow = new QHBoxLayout();
    m_search = new QLineEdit(content);
    m_search->setPlaceholderText(tr("Search for addons…"));
    filterRow->addWidget(m_search, 1);

    m_sortBox = new QComboBox(content);
    m_sortBox->addItem(tr("Sort by stars"), static_cast<int>(SortMode::Stars));
    m_sortBox->addItem(tr("Sort by downloads"), static_cast<int>(SortMode::Downloads));
    filterRow->addWidget(m_sortBox);
    contentLayout->addLayout(filterRow);

    m_showAllVersions = new QCheckBox(tr("Show addons for other Minecraft versions"), content);
    contentLayout->addWidget(m_showAllVersions);

    auto* splitter = new QSplitter(Qt::Horizontal, content);
    splitter->setChildrenCollapsible(false);

    m_model = new QStandardItemModel(this);
    m_list = new QListView(splitter);
    m_list->setModel(m_model);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setAlternatingRowColors(false);
    m_list->setIconSize(QSize(56, 56));
    m_list->setSpacing(4);
    m_list->setUniformItemSizes(true);
    m_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);

    auto* delegate = new ProjectItemDelegate(m_list);
    m_list->setItemDelegate(delegate);

    m_details = new QTextBrowser(splitter);
    m_details->setOpenExternalLinks(true);

    splitter->addWidget(m_list);
    splitter->addWidget(m_details);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 4);
    contentLayout->addWidget(splitter, 1);

    auto* selectRow = new QHBoxLayout();
    selectRow->addStretch();
    m_selectButton = new QPushButton(tr("Select addon for download"), content);
    m_selectButton->setEnabled(false);
    selectRow->addWidget(m_selectButton);
    contentLayout->addLayout(selectRow);

    root->addWidget(content, 1);

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
#ifndef Q_OS_MACOS
    m_buttons->setContentsMargins(0, 0, 6, 6);
#endif
    auto* okButton = m_buttons->button(QDialogButtonBox::Ok);
    okButton->setText(tr("Review and confirm"));
    okButton->setEnabled(false);
    okButton->setDefault(true);
    okButton->setAutoDefault(true);
    okButton->setShortcut(tr("Ctrl+Return"));

    auto* cancelButton = m_buttons->button(QDialogButtonBox::Cancel);
    cancelButton->setDefault(false);
    cancelButton->setAutoDefault(false);

    auto* refreshButton = m_buttons->addButton(tr("Refresh catalog"), QDialogButtonBox::ActionRole);

    root->addWidget(m_buttons);

    connect(m_search, &QLineEdit::textChanged, this, &MeteorAddonsDialog::onFilterChanged);
    connect(m_sortBox, &QComboBox::currentIndexChanged, this, &MeteorAddonsDialog::onFilterChanged);
    connect(m_showAllVersions, &QCheckBox::toggled, this, &MeteorAddonsDialog::onFilterChanged);
    connect(m_list->selectionModel(), &QItemSelectionModel::currentChanged, this, &MeteorAddonsDialog::onCurrentChanged);
    connect(m_list, &QAbstractItemView::doubleClicked, this, &MeteorAddonsDialog::onAddonToggle);
    connect(delegate, &ProjectItemDelegate::checkboxClicked, this, &MeteorAddonsDialog::onAddonToggle);
    connect(m_selectButton, &QPushButton::clicked, this, &MeteorAddonsDialog::onSelectClicked);
    connect(okButton, &QPushButton::clicked, this, &MeteorAddonsDialog::onConfirmClicked);
    connect(cancelButton, &QPushButton::clicked, this, &MeteorAddonsDialog::reject);
    connect(refreshButton, &QPushButton::clicked, this, [this] {
        setBusy(true, tr("Refreshing catalog…"));
        m_provider->refresh(true);
    });

    m_provider = new HackClients::MeteorAddonsProvider(APPLICATION->network(), this);
    connect(m_provider, &HackClients::MeteorAddonsProvider::refreshed, this, &MeteorAddonsDialog::onCatalogReady);
    connect(m_provider, &HackClients::MeteorAddonsProvider::failed, this, &MeteorAddonsDialog::onCatalogFailed);

    setBusy(true, tr("Loading Meteor addons catalog…"));
    m_provider->refresh(false);
}

QString MeteorAddonsDialog::selectionKey(const HackClients::MeteorAddonEntry& entry)
{
    if (!entry.repoOwner.isEmpty() && !entry.repoName.isEmpty())
        return QStringLiteral("%1/%2").arg(entry.repoOwner, entry.repoName);
    if (!entry.name.isEmpty())
        return entry.name;
    return entry.pickJarDownloadUrl();
}

MeteorAddonsDialog::SortMode MeteorAddonsDialog::currentSortMode() const
{
    return static_cast<SortMode>(m_sortBox->currentData().toInt());
}

bool MeteorAddonsDialog::isAddonInstalled(const HackClients::MeteorAddonEntry& entry) const
{
    const QString fileName = jarFileNameFor(entry);
    if (fileName.isEmpty())
        return false;
    return QFileInfo::exists(FS::PathCombine(m_modsDir, fileName));
}

HackClients::MeteorAddonEntry MeteorAddonsDialog::addonAt(const QModelIndex& index) const
{
    if (!index.isValid())
        return {};
    const int row = index.data(AddonIndexRole).toInt();
    if (row < 0 || row >= m_visible.size())
        return {};
    return m_visible.at(row);
}

void MeteorAddonsDialog::setBusy(bool busy, const QString& status)
{
    m_busy = busy;
    m_list->setEnabled(!busy);
    m_search->setEnabled(!busy);
    m_sortBox->setEnabled(!busy);
    m_showAllVersions->setEnabled(!busy);
    m_selectButton->setEnabled(!busy && m_list->currentIndex().isValid());
    updateConfirmButton();
    if (!status.isEmpty())
        m_details->setPlainText(status);
}

void MeteorAddonsDialog::onCatalogReady()
{
    rebuildList();
    setBusy(false);
    if (m_visible.isEmpty())
        m_details->setPlainText(tr("No installable addons found for this filter."));
    else
        updateDetails();
}

void MeteorAddonsDialog::onCatalogFailed(QString reason)
{
    setBusy(false);
    m_details->setPlainText(reason);
}

void MeteorAddonsDialog::rebuildList()
{
    const QString query = m_search->text().trimmed();
    const bool allVersions = m_showAllVersions->isChecked();
    const SortMode sortMode = currentSortMode();
    const QModelIndex previous = m_list->currentIndex();
    const int previousAddon = previous.isValid() ? previous.data(AddonIndexRole).toInt() : -1;

    m_visible = m_provider->addonsForMinecraft(m_minecraftVersion, allVersions);
    if (!query.isEmpty()) {
        QList<HackClients::MeteorAddonEntry> filtered;
        for (const auto& entry : m_visible) {
            if (entry.name.contains(query, Qt::CaseInsensitive) || entry.description.contains(query, Qt::CaseInsensitive) ||
                entry.repoOwner.contains(query, Qt::CaseInsensitive) || entry.repoName.contains(query, Qt::CaseInsensitive))
                filtered.append(entry);
        }
        m_visible = filtered;
    }

    std::sort(m_visible.begin(), m_visible.end(),
              [sortMode](const HackClients::MeteorAddonEntry& a, const HackClients::MeteorAddonEntry& b) {
                  if (sortMode == SortMode::Downloads) {
                      if (a.downloads != b.downloads)
                          return a.downloads > b.downloads;
                  } else if (a.stars != b.stars) {
                      return a.stars > b.stars;
                  }
                  if (a.verified != b.verified)
                      return a.verified > b.verified;
                  return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
              });

    m_model->clear();
    int restoreRow = -1;
    for (int i = 0; i < m_visible.size(); ++i) {
        const auto& entry = m_visible.at(i);
        auto* item = new QStandardItem();
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
        item->setData(entry.name, UserDataTypes::TITLE);

        QString description = entry.description;
        if (description.isEmpty())
            description = tr("No description provided.");
        QStringList meta;
        if (entry.verified)
            meta << tr("verified");
        meta << tr("★ %1").arg(entry.stars);
        meta << tr("↓ %1").arg(entry.downloads);
        if (!entry.minecraftVersion.isEmpty() && entry.minecraftVersion != m_minecraftVersion)
            meta << tr("MC %1").arg(entry.minecraftVersion);
        description = QStringLiteral("%1\n%2").arg(description, meta.join(QStringLiteral(" · ")));

        item->setData(description, UserDataTypes::DESCRIPTION);
        item->setData(isAddonInstalled(entry), UserDataTypes::INSTALLED);
        item->setData(i, AddonIndexRole);
        item->setCheckable(true);
        item->setCheckState(m_queued.contains(selectionKey(entry)) ? Qt::Checked : Qt::Unchecked);
        item->setIcon(QIcon::fromTheme(QStringLiteral("loadermods")));
        m_model->appendRow(item);

        if (i == previousAddon)
            restoreRow = i;
    }

    if (m_model->rowCount() > 0) {
        const int row = restoreRow >= 0 ? restoreRow : 0;
        m_list->setCurrentIndex(m_model->index(row, 0));
    } else {
        updateDetails();
        updateSelectButton();
    }
    updateConfirmButton();
}

void MeteorAddonsDialog::onFilterChanged()
{
    if (m_provider->isLoaded())
        rebuildList();
}

void MeteorAddonsDialog::onCurrentChanged(const QModelIndex& current, [[maybe_unused]] const QModelIndex& previous)
{
    Q_UNUSED(current);
    updateDetails();
    updateSelectButton();
}

void MeteorAddonsDialog::updateDetails()
{
    const auto entry = addonAt(m_list->currentIndex());
    if (entry.name.isEmpty()) {
        if (!m_busy)
            m_details->clear();
        return;
    }

    const QString url = entry.pickJarDownloadUrl();
    QString html = QStringLiteral("<h3>%1</h3>").arg(entry.name.toHtmlEscaped());
    if (!entry.description.isEmpty())
        html += QStringLiteral("<p>%1</p>").arg(entry.description.toHtmlEscaped());
    if (!entry.minecraftVersion.isEmpty()) {
        html += QStringLiteral("<p><b>%1</b> %2</p>")
                    .arg(tr("Listed MC version:"), entry.minecraftVersion.toHtmlEscaped());
    }
    if (!entry.authors.isEmpty())
        html += QStringLiteral("<p><b>%1</b> %2</p>").arg(tr("Authors:"), entry.authors.join(", ").toHtmlEscaped());
    html += QStringLiteral("<p><b>%1</b> %2 · <b>%3</b> %4</p>")
                .arg(tr("Stars:"), QString::number(entry.stars), tr("Downloads:"), QString::number(entry.downloads));
    if (!entry.githubUrl.isEmpty())
        html += QStringLiteral("<p><a href=\"%1\">GitHub</a></p>").arg(entry.githubUrl.toHtmlEscaped());
    if (url.isEmpty())
        html += QStringLiteral("<p><i>%1</i></p>").arg(tr("No direct JAR download in catalog."));
    else
        html += QStringLiteral("<p><i>%1</i></p>").arg(url.toHtmlEscaped());
    if (isAddonInstalled(entry))
        html += QStringLiteral("<p><b>%1</b></p>").arg(tr("Already present in this instance's mods folder."));

    m_details->setHtml(html);
}

void MeteorAddonsDialog::updateSelectButton()
{
    const auto entry = addonAt(m_list->currentIndex());
    if (m_busy || entry.name.isEmpty() || entry.pickJarDownloadUrl().isEmpty()) {
        m_selectButton->setEnabled(false);
        m_selectButton->setText(tr("Cannot select invalid addon :("));
        return;
    }

    m_selectButton->setEnabled(true);
    if (m_queued.contains(selectionKey(entry)))
        m_selectButton->setText(tr("Deselect addon for download"));
    else
        m_selectButton->setText(tr("Select addon for download"));
}

void MeteorAddonsDialog::updateConfirmButton()
{
    auto* okButton = m_buttons->button(QDialogButtonBox::Ok);
    okButton->setEnabled(!m_busy && !m_queued.isEmpty());
}

void MeteorAddonsDialog::toggleAddonAt(int visibleIndex)
{
    if (visibleIndex < 0 || visibleIndex >= m_visible.size())
        return;

    const auto& entry = m_visible.at(visibleIndex);
    const QString url = entry.pickJarDownloadUrl();
    if (url.isEmpty())
        return;

    const QString key = selectionKey(entry);
    if (m_queued.contains(key)) {
        m_queued.remove(key);
    } else {
        if (!entry.supportsMinecraft(m_minecraftVersion)) {
            auto answer = QMessageBox::question(
                this, tr("Version mismatch"),
                tr("This addon is listed for Minecraft %1, but your instance is %2.\n\nSelect it anyway?")
                    .arg(entry.minecraftVersion.isEmpty() ? tr("unknown") : entry.minecraftVersion, m_minecraftVersion),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (answer != QMessageBox::Yes)
                return;
        }
        m_queued.insert(key, entry);
    }

    auto* item = m_model->item(visibleIndex);
    if (item)
        item->setCheckState(m_queued.contains(key) ? Qt::Checked : Qt::Unchecked);

    updateSelectButton();
    updateConfirmButton();
    m_list->viewport()->update();
}

void MeteorAddonsDialog::onAddonToggle(const QModelIndex& index)
{
    if (!index.isValid() || m_busy)
        return;
    toggleAddonAt(index.data(AddonIndexRole).toInt());
}

void MeteorAddonsDialog::onSelectClicked()
{
    onAddonToggle(m_list->currentIndex());
}

void MeteorAddonsDialog::reject()
{
    if (!m_queued.isEmpty()) {
        auto reply = CustomMessageBox::selectable(this, tr("Confirmation Needed"),
                                                  tr("You have %1 selected resources.\n"
                                                     "Are you sure you want to close this dialog?")
                                                      .arg(m_queued.size()),
                                                  QMessageBox::Question, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                         ->exec();
        if (reply != QMessageBox::Yes)
            return;
    }
    QDialog::reject();
}

void MeteorAddonsDialog::onConfirmClicked()
{
    if (m_queued.isEmpty())
        return;

    auto* confirmDialog = ReviewMessageBox::create(this, tr("Confirm addons to download"));
    confirmDialog->retranslateUi(tr("addons"));

    QList<HackClients::MeteorAddonEntry> selected = m_queued.values();
    std::sort(selected.begin(), selected.end(), [](const HackClients::MeteorAddonEntry& a, const HackClients::MeteorAddonEntry& b) {
        return QString::compare(a.name, b.name, Qt::CaseInsensitive) < 0;
    });

    for (const auto& entry : selected) {
        const QString fileName = jarFileNameFor(entry);
        confirmDialog->appendResource({
            .name = entry.name,
            .filename = fileName,
            .provider = QStringLiteral("Meteor Addons"),
            .required_by = {},
            .version = entry.minecraftVersion,
            .enabled = !isAddonInstalled(entry),
            .deselectKey = selectionKey(entry),
        });
    }

    if (confirmDialog->exec() == 0) {
        confirmDialog->deleteLater();
        return;
    }

    const QStringList deselected = confirmDialog->deselectedResources();
    confirmDialog->deleteLater();

    QList<HackClients::MeteorAddonEntry> toInstall;
    for (const auto& entry : selected) {
        const QString key = selectionKey(entry);
        if (deselected.contains(key) || deselected.contains(entry.name))
            continue;
        if (entry.pickJarDownloadUrl().isEmpty())
            continue;
        toInstall.append(entry);
    }

    if (toInstall.isEmpty()) {
        QMessageBox::information(this, tr("Meteor Addons"), tr("Nothing new to install."));
        return;
    }

    ConcurrentTask tasks(tr("Download Meteor Addons"), APPLICATION->settings()->get("NumberOfConcurrentDownloads").toInt());
    for (const auto& entry : toInstall) {
        const QString url = entry.pickJarDownloadUrl();
        auto task = makeShared<HackClients::MeteorAddonInstallTask>(m_modsDir, QUrl(url), jarFileNameFor(entry));
        tasks.addTask(task);
    }

    connect(&tasks, &Task::failed, this, [this](const QString& reason) {
        CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->show();
    });
    connect(&tasks, &Task::succeeded, this, [this, &tasks]() {
        QStringList warnings = tasks.warnings();
        if (warnings.count())
            CustomMessageBox::selectable(this, tr("Warnings"), warnings.join('\n'), QMessageBox::Warning)->show();
    });

    m_modsMayHaveChanged = true;
    ProgressDialog loadDialog(this);
    loadDialog.setSkipButton(true, tr("Abort"));
    loadDialog.setWindowTitle(tr("Downloading addons…"));
    const int result = loadDialog.execWithTask(&tasks);

    if (result == QDialog::Accepted)
        accept();
}
