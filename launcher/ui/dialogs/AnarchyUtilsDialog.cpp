// SPDX-License-Identifier: GPL-3.0-only
#include "AnarchyUtilsDialog.h"

#include "hackclients/AnarchyUtilityCatalog.h"
#include "hackclients/AnarchyUtilityInstallFlow.h"
#include "minecraft/mod/ModFolderModel.h"

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QTextBrowser>
#include <QVBoxLayout>

AnarchyUtilsDialog::AnarchyUtilsDialog(const QString& minecraftVersion, ModFolderModel* modsModel, QWidget* parent)
    : QDialog(parent), m_minecraftVersion(minecraftVersion), m_modsModel(modsModel)
{
    setWindowTitle(tr("Anarchy Utils"));
    resize(520, 480);

    auto* root = new QVBoxLayout(this);

    auto* intro = new QLabel(
        tr("Curated Fabric utilities for anarchy / building workflows. "
           "On install, Arsenal resolves Modrinth dependencies and GitHub releases "
           "(e.g. Arsenal NameProtect) for Minecraft %1.")
            .arg(m_minecraftVersion),
        this);
    intro->setWordWrap(true);
    root->addWidget(intro);

    m_list = new QListWidget(this);
    m_list->setSelectionMode(QAbstractItemView::NoSelection);
    root->addWidget(m_list, 1);

    for (const auto& entry : HackClients::AnarchyUtilityCatalog::all()) {
        auto* item = new QListWidgetItem(entry.name, m_list);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        // Default-check Litematica stack only; leave the rest opt-in.
        const bool defaultOn = entry.id == QLatin1String("litematica") ||
                               entry.id == QLatin1String("litematica-printer");
        item->setCheckState(defaultOn ? Qt::Checked : Qt::Unchecked);
        item->setData(Qt::UserRole, entry.id);
        item->setToolTip(entry.description);
    }

    m_details = new QTextBrowser(this);
    m_details->setOpenExternalLinks(true);
    m_details->setMaximumHeight(140);
    root->addWidget(m_details);

    auto* buttons = new QHBoxLayout();
    m_installButton = new QPushButton(tr("Install selected"), this);
    auto* closeButton = new QPushButton(tr("Close"), this);
    buttons->addStretch();
    buttons->addWidget(m_installButton);
    buttons->addWidget(closeButton);
    root->addLayout(buttons);

    connect(m_list, &QListWidget::itemChanged, this, &AnarchyUtilsDialog::onSelectionChanged);
    connect(m_list, &QListWidget::currentRowChanged, this, &AnarchyUtilsDialog::onSelectionChanged);
    connect(m_installButton, &QPushButton::clicked, this, &AnarchyUtilsDialog::onInstallClicked);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);

    if (m_list->count() > 0)
        m_list->setCurrentRow(0);
    onSelectionChanged();
}

QStringList AnarchyUtilsDialog::selectedUtilityIds() const
{
    QStringList ids;
    for (int i = 0; i < m_list->count(); ++i) {
        const auto* item = m_list->item(i);
        if (item->checkState() == Qt::Checked)
            ids.append(item->data(Qt::UserRole).toString());
    }
    return ids;
}

void AnarchyUtilsDialog::onSelectionChanged()
{
    m_installButton->setEnabled(!selectedUtilityIds().isEmpty());

    const int row = m_list->currentRow();
    if (row < 0 || row >= m_list->count()) {
        const auto ids = selectedUtilityIds();
        if (ids.isEmpty()) {
            m_details->setPlainText(tr("Select at least one utility to install."));
            return;
        }
        m_details->setPlainText(tr("%1 selected. Required dependencies are checked on install.").arg(ids.size()));
        return;
    }

    const QString id = m_list->item(row)->data(Qt::UserRole).toString();
    const auto entry = HackClients::AnarchyUtilityCatalog::byId(id);
    QString html = QStringLiteral("<h3>%1</h3>").arg(entry.name.toHtmlEscaped());
    if (!entry.description.isEmpty())
        html += QStringLiteral("<p>%1</p>").arg(entry.description.toHtmlEscaped());
    if (!entry.modrinthSlug.isEmpty()) {
        html += QStringLiteral("<p><a href=\"https://modrinth.com/mod/%1\">Modrinth</a></p>")
                    .arg(entry.modrinthSlug.toHtmlEscaped());
    } else if (!entry.githubReleasesRepo.isEmpty()) {
        html += QStringLiteral("<p><a href=\"https://github.com/%1/releases\">GitHub Releases</a></p>")
                    .arg(entry.githubReleasesRepo.toHtmlEscaped());
    }
    m_details->setHtml(html);
}

void AnarchyUtilsDialog::onInstallClicked()
{
    const QStringList ids = selectedUtilityIds();
    if (ids.isEmpty()) {
        QMessageBox::information(this, tr("Anarchy Utils"), tr("Select at least one utility."));
        return;
    }
    if (!m_modsModel)
        return;

    const QString modsDir = m_modsModel->dir().absolutePath();
    const QString indexDir = m_modsModel->indexDir().absolutePath();

    HackClients::AnarchyUtilityInstallOptions opts;
    opts.requireReview = true;
    opts.windowTitle = tr("Anarchy Utils");

    if (HackClients::resolveAndInstallAnarchyUtilities(modsDir, indexDir, m_minecraftVersion, ids, this, opts)) {
        m_modsMayHaveChanged = true;
        accept();
    }
}
