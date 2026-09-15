// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "hackclients/HackClientTypes.h"
#include "hackclients/MeteorAddonsProvider.h"

#include <QDialog>
#include <QList>

class QListWidget;
class QListWidgetItem;
class QLineEdit;
class QTextBrowser;
class QCheckBox;
class QPushButton;

class MeteorAddonsDialog : public QDialog {
    Q_OBJECT
   public:
    MeteorAddonsDialog(const QString& minecraftVersion, const QString& modsDir, QWidget* parent = nullptr);

    HackClients::MeteorAddonEntry selectedAddon() const { return m_selected; }

   private slots:
    void onCatalogReady();
    void onCatalogFailed(QString reason);
    void onFilterChanged();
    void onSelectionChanged();
    void onInstallClicked();

   private:
    void rebuildList();
    void setBusy(bool busy, const QString& status = {});

    QString m_minecraftVersion;
    QString m_modsDir;
    HackClients::MeteorAddonsProvider* m_provider = nullptr;
    HackClients::MeteorAddonEntry m_selected;
    QList<HackClients::MeteorAddonEntry> m_visible;

    QListWidget* m_list = nullptr;
    QLineEdit* m_search = nullptr;
    QCheckBox* m_showAllVersions = nullptr;
    QTextBrowser* m_details = nullptr;
    QPushButton* m_installButton = nullptr;
};
