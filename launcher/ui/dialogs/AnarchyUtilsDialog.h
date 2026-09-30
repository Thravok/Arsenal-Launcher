// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QDialog>
#include <QString>
#include <QStringList>

class ModFolderModel;
class QListWidget;
class QTextBrowser;
class QPushButton;

class AnarchyUtilsDialog : public QDialog {
    Q_OBJECT
   public:
    AnarchyUtilsDialog(const QString& minecraftVersion, ModFolderModel* modsModel, QWidget* parent = nullptr);

    QStringList selectedUtilityIds() const;
    /** True if any install attempt may have changed files on disk. */
    bool modsMayHaveChanged() const { return m_modsMayHaveChanged; }

   private slots:
    void onSelectionChanged();
    void onInstallClicked();

   private:
    QString m_minecraftVersion;
    ModFolderModel* m_modsModel = nullptr;
    bool m_modsMayHaveChanged = false;

    QListWidget* m_list = nullptr;
    QTextBrowser* m_details = nullptr;
    QPushButton* m_installButton = nullptr;
};
