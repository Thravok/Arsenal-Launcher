// SPDX-License-Identifier: GPL-3.0-only
#include "ArsenalUtilitiesInstall.h"

#include "AnarchyUtilityInstallFlow.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "minecraft/mod/ModFolderModel.h"
#include "modplatform/ModIndex.h"
#include "ui/dialogs/CustomMessageBox.h"

#include <QMessageBox>
#include <QObject>

namespace HackClients {

bool installArsenalUtilities(MinecraftInstance* instance, QWidget* parent)
{
    if (!instance)
        return false;

    auto profile = instance->getPackProfile();
    if (!profile)
        return false;

    const auto loaders = profile->getModLoaders();
    if (!loaders || !loaders->testFlag(ModPlatform::ModLoaderType::Fabric)) {
        // Non-Fabric: nothing to do (toggle is ignored silently).
        return true;
    }

    const QString mcVersion = profile->getComponentVersion(QStringLiteral("net.minecraft"));
    if (mcVersion.isEmpty()) {
        if (parent) {
            CustomMessageBox::selectable(parent, QObject::tr("Arsenal NameProtect"),
                                         QObject::tr("Could not determine the Minecraft version for NameProtect install."),
                                         QMessageBox::Warning)
                ->show();
        }
        return false;
    }

    const QString modsDir = instance->modsRoot();
    auto modsModel = instance->loaderModList();
    const QString indexDir = modsModel ? modsModel->indexDir().absolutePath() : QString();

    AnarchyUtilityInstallOptions opts;
    opts.requireReview = false;
    opts.windowTitle = QObject::tr("Arsenal NameProtect");

    const bool ok = resolveAndInstallAnarchyUtilities(modsDir, indexDir, mcVersion,
                                                      { QStringLiteral("arsenal-nameprotect") }, parent, opts);

    if (ok && modsModel)
        modsModel->update();

    return ok;
}

}  // namespace HackClients
