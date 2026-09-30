// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "hackclients/HackClientTypes.h"
#include "hackclients/MeteorAddonsProvider.h"

#include <QDialog>
#include <QHash>
#include <QList>
#include <QString>

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QLineEdit;
class QListView;
class QPushButton;
class QStandardItemModel;
class QTextBrowser;

class MeteorAddonsDialog : public QDialog {
    Q_OBJECT
   public:
    MeteorAddonsDialog(const QString& minecraftVersion, const QString& modsDir, QWidget* parent = nullptr);

    /** True if any install attempt may have changed files on disk. */
    bool modsMayHaveChanged() const { return m_modsMayHaveChanged; }

   public slots:
    void reject() override;

   private slots:
    void onCatalogReady();
    void onCatalogFailed(QString reason);
    void onFilterChanged();
    void onCurrentChanged(const QModelIndex& current, const QModelIndex& previous);
    void onAddonToggle(const QModelIndex& index);
    void onSelectClicked();
    void onConfirmClicked();

   private:
    enum class SortMode { Stars, Downloads };
    enum Roles {
        AddonIndexRole = Qt::UserRole + 10,
    };

    static QString selectionKey(const HackClients::MeteorAddonEntry& entry);
    SortMode currentSortMode() const;
    void rebuildList();
    void setBusy(bool busy, const QString& status = {});
    void updateDetails();
    void updateSelectButton();
    void updateConfirmButton();
    void toggleAddonAt(int visibleIndex);
    bool isAddonInstalled(const HackClients::MeteorAddonEntry& entry) const;
    HackClients::MeteorAddonEntry addonAt(const QModelIndex& index) const;

    QString m_minecraftVersion;
    QString m_modsDir;
    bool m_modsMayHaveChanged = false;
    bool m_busy = false;

    HackClients::MeteorAddonsProvider* m_provider = nullptr;
    QList<HackClients::MeteorAddonEntry> m_visible;
    QHash<QString, HackClients::MeteorAddonEntry> m_queued;

    QStandardItemModel* m_model = nullptr;
    QListView* m_list = nullptr;
    QLineEdit* m_search = nullptr;
    QComboBox* m_sortBox = nullptr;
    QCheckBox* m_showAllVersions = nullptr;
    QTextBrowser* m_details = nullptr;
    QPushButton* m_selectButton = nullptr;
    QDialogButtonBox* m_buttons = nullptr;
};
