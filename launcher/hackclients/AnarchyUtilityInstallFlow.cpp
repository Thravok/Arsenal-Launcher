// SPDX-License-Identifier: GPL-3.0-only
#include "AnarchyUtilityInstallFlow.h"

#include "AnarchyUtilityInstallTask.h"
#include "tasks/Task.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ProgressDialog.h"
#include "ui/dialogs/ReviewMessageBox.h"

#include <QMessageBox>
#include <QObject>

namespace HackClients {

bool resolveAndInstallAnarchyUtilities(const QString& modsDir,
                                       const QString& indexDir,
                                       const QString& minecraftVersion,
                                       const QStringList& utilityIds,
                                       QWidget* parent,
                                       const AnarchyUtilityInstallOptions& options)
{
    if (utilityIds.isEmpty())
        return true;

    const QString title =
        options.windowTitle.isEmpty() ? QObject::tr("Anarchy Utils") : options.windowTitle;

    auto* resolveTask = new AnarchyUtilityResolveTask(modsDir, minecraftVersion, utilityIds, parent);
    ProgressDialog resolveDialog(parent);
    resolveDialog.setSkipButton(true, QObject::tr("Abort"));
    resolveDialog.setWindowTitle(QObject::tr("Checking for dependencies…"));

    QString resolveError;
    QObject::connect(resolveTask, &Task::failed, resolveTask, [&resolveError](QString reason) { resolveError = reason; });
    const int resolveRet = resolveDialog.execWithTask(resolveTask);
    const auto plan = resolveTask->plan();
    resolveTask->deleteLater();

    if (resolveRet == QDialog::Rejected) {
        if (!resolveError.isEmpty() && parent) {
            CustomMessageBox::selectable(parent, title, resolveError, QMessageBox::Warning)->show();
        }
        return false;
    }

    QList<AnarchyUtilityResolvedFile> toInstall;
    if (options.requireReview) {
        auto* confirmDialog = ReviewMessageBox::create(parent, QObject::tr("Confirm mods to download"));
        confirmDialog->retranslateUi(QObject::tr("mods"));
        for (const auto& file : plan) {
            const bool skipExact = file.alreadyInstalled;
            QString tip;
            if (file.maybeInstalled && !file.alreadyInstalled) {
                tip = QObject::tr("A related jar may already be installed; installing will replace or add alongside it.");
            } else if (skipExact) {
                tip = QObject::tr("Mod was disabled as it may be already installed.");
            }
            confirmDialog->appendResource({
                .name = file.displayName,
                .filename = file.fileName,
                .provider = file.projectId.contains(QLatin1Char('/')) ? QStringLiteral("GitHub")
                                                                      : QStringLiteral("Modrinth"),
                .required_by = file.requiredBy,
                .version = file.versionNumber,
                .enabled = !skipExact,
                .deselectKey = !file.projectId.isEmpty() ? file.projectId : file.fileName,
                .tooltip = tip,
            });
        }

        if (confirmDialog->exec() == 0) {
            confirmDialog->deleteLater();
            return false;
        }

        const QStringList deselected = confirmDialog->deselectedResources();
        confirmDialog->deleteLater();

        for (const auto& file : plan) {
            if (file.alreadyInstalled)
                continue;
            const QString key = !file.projectId.isEmpty() ? file.projectId : file.fileName;
            if (deselected.contains(key) || deselected.contains(file.displayName))
                continue;
            toInstall.append(file);
        }
    } else {
        for (const auto& file : plan) {
            if (!file.alreadyInstalled)
                toInstall.append(file);
        }
    }

    if (toInstall.isEmpty())
        return true;

    auto* installTask = new AnarchyUtilityInstallTask(modsDir, indexDir, toInstall, parent);
    ProgressDialog installDialog(parent);
    installDialog.setSkipButton(true, QObject::tr("Abort"));
    installDialog.setWindowTitle(QObject::tr("Downloading mods…"));

    bool ok = false;
    QString installError;
    QObject::connect(installTask, &Task::succeeded, installTask, [&ok]() { ok = true; });
    QObject::connect(installTask, &Task::failed, installTask, [&installError](QString reason) { installError = reason; });
    installDialog.execWithTask(installTask);
    installTask->deleteLater();

    if (!ok && parent && !installError.isEmpty()) {
        CustomMessageBox::selectable(parent, title, installError, QMessageBox::Warning)->show();
    }
    return ok;
}

}  // namespace HackClients
