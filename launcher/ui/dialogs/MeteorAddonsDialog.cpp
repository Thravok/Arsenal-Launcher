// SPDX-License-Identifier: GPL-3.0-only
#include "MeteorAddonsDialog.h"

#include "Application.h"
#include "hackclients/MeteorAddonInstallTask.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ProgressDialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QTextBrowser>
#include <QVBoxLayout>

MeteorAddonsDialog::MeteorAddonsDialog(const QString& minecraftVersion, const QString& modsDir, QWidget* parent)
    : QDialog(parent), m_minecraftVersion(minecraftVersion), m_modsDir(modsDir)
{
    setWindowTitle(tr("Meteor Addons"));
    resize(640, 480);

    auto* root = new QVBoxLayout(this);

    auto* warning = new QLabel(
        tr("Third-party Meteor addons from the community catalog (meteoraddons.com). "
           "Only install addons you trust; jars run with full game access."),
        this);
    warning->setWordWrap(true);
    root->addWidget(warning);

    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(tr("Search addons…"));
    root->addWidget(m_search);

    m_showAllVersions = new QCheckBox(tr("Show addons for other Minecraft versions"), this);
    root->addWidget(m_showAllVersions);

    m_list = new QListWidget(this);
    root->addWidget(m_list, 1);

    m_details = new QTextBrowser(this);
    m_details->setOpenExternalLinks(true);
    m_details->setMaximumHeight(140);
    root->addWidget(m_details);

    auto* buttons = new QHBoxLayout();
    auto* refreshButton = new QPushButton(tr("Refresh catalog"), this);
    m_installButton = new QPushButton(tr("Install"), this);
    auto* closeButton = new QPushButton(tr("Close"), this);
    m_installButton->setEnabled(false);
    buttons->addWidget(refreshButton);
    buttons->addStretch();
    buttons->addWidget(m_installButton);
    buttons->addWidget(closeButton);
    root->addLayout(buttons);

    connect(m_search, &QLineEdit::textChanged, this, &MeteorAddonsDialog::onFilterChanged);
    connect(m_showAllVersions, &QCheckBox::toggled, this, &MeteorAddonsDialog::onFilterChanged);
    connect(m_list, &QListWidget::currentRowChanged, this, &MeteorAddonsDialog::onSelectionChanged);
    connect(refreshButton, &QPushButton::clicked, this, [this] {
        setBusy(true, tr("Refreshing catalog…"));
        m_provider->refresh(true);
    });
    connect(m_installButton, &QPushButton::clicked, this, &MeteorAddonsDialog::onInstallClicked);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);

    m_provider = new HackClients::MeteorAddonsProvider(APPLICATION->network(), this);
    connect(m_provider, &HackClients::MeteorAddonsProvider::refreshed, this, &MeteorAddonsDialog::onCatalogReady);
    connect(m_provider, &HackClients::MeteorAddonsProvider::failed, this, &MeteorAddonsDialog::onCatalogFailed);

    setBusy(true, tr("Loading Meteor addons catalog…"));
    m_provider->refresh(false);
}

void MeteorAddonsDialog::setBusy(bool busy, const QString& status)
{
    m_list->setEnabled(!busy);
    m_installButton->setEnabled(!busy && m_list->currentRow() >= 0);
    if (!status.isEmpty())
        m_details->setPlainText(status);
}

void MeteorAddonsDialog::onCatalogReady()
{
    rebuildList();
    setBusy(false);
    if (m_visible.isEmpty()) {
        m_details->setPlainText(tr("No installable addons found for this filter."));
    }
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

    m_visible = m_provider->addonsForMinecraft(m_minecraftVersion, allVersions);
    if (!query.isEmpty()) {
        QList<HackClients::MeteorAddonEntry> filtered;
        for (const auto& entry : m_visible) {
            if (entry.name.contains(query, Qt::CaseInsensitive) ||
                entry.description.contains(query, Qt::CaseInsensitive) ||
                entry.repoOwner.contains(query, Qt::CaseInsensitive))
                filtered.append(entry);
        }
        m_visible = filtered;
    }

    m_list->clear();
    for (int i = 0; i < m_visible.size(); ++i) {
        const auto& entry = m_visible.at(i);
        QString label = entry.name;
        if (entry.verified)
            label = tr("%1 (verified)").arg(label);
        if (!entry.minecraftVersion.isEmpty() && entry.minecraftVersion != m_minecraftVersion)
            label += tr(" — MC %1").arg(entry.minecraftVersion);
        auto* item = new QListWidgetItem(label, m_list);
        item->setData(Qt::UserRole, i);
    }

    if (m_list->count() > 0)
        m_list->setCurrentRow(0);
    else
        onSelectionChanged();
}

void MeteorAddonsDialog::onFilterChanged()
{
    if (m_provider->isLoaded())
        rebuildList();
}

void MeteorAddonsDialog::onSelectionChanged()
{
    const int row = m_list->currentRow();
    if (row < 0 || row >= m_visible.size()) {
        m_selected = {};
        m_installButton->setEnabled(false);
        m_details->clear();
        return;
    }

    m_selected = m_visible.at(row);
    const QString url = m_selected.pickJarDownloadUrl();

    QString html = QString("<h3>%1</h3>").arg(m_selected.name.toHtmlEscaped());
    if (!m_selected.description.isEmpty())
        html += QString("<p>%1</p>").arg(m_selected.description.toHtmlEscaped());
    if (!m_selected.minecraftVersion.isEmpty())
        html += QString("<p><b>%1</b> %2</p>").arg(tr("Listed MC version:"), m_selected.minecraftVersion.toHtmlEscaped());
    if (!m_selected.authors.isEmpty())
        html += QString("<p><b>%1</b> %2</p>").arg(tr("Authors:"), m_selected.authors.join(", ").toHtmlEscaped());
    if (!m_selected.githubUrl.isEmpty())
        html += QString("<p><a href=\"%1\">GitHub</a></p>").arg(m_selected.githubUrl.toHtmlEscaped());
    if (url.isEmpty())
        html += QString("<p><i>%1</i></p>").arg(tr("No direct JAR download in catalog."));
    else
        html += QString("<p><i>%1</i></p>").arg(url.toHtmlEscaped());

    m_details->setHtml(html);
    m_installButton->setEnabled(!url.isEmpty());
}

void MeteorAddonsDialog::onInstallClicked()
{
    const QString url = m_selected.pickJarDownloadUrl();
    if (url.isEmpty()) {
        QMessageBox::information(this, tr("Meteor Addons"), tr("This addon has no direct download URL."));
        return;
    }

    if (!m_selected.supportsMinecraft(m_minecraftVersion)) {
        auto answer = QMessageBox::question(
            this, tr("Version mismatch"),
            tr("This addon is listed for Minecraft %1, but your instance is %2.\n\nInstall anyway?")
                .arg(m_selected.minecraftVersion.isEmpty() ? tr("unknown") : m_selected.minecraftVersion,
                     m_minecraftVersion),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return;
    }

    auto* task = new HackClients::MeteorAddonInstallTask(m_modsDir, QUrl(url), QString(), this);

    ProgressDialog loadDialog(this);
    loadDialog.setSkipButton(true, tr("Abort"));
    connect(task, &Task::failed, this, [this, task](QString reason) {
        CustomMessageBox::selectable(this, tr("Meteor Addons"), reason, QMessageBox::Critical)->show();
        task->deleteLater();
    });
    connect(task, &Task::succeeded, this, [this, task]() {
        accept();
        task->deleteLater();
    });
    connect(task, &Task::aborted, task, &Task::deleteLater);
    loadDialog.execWithTask(task);
}
