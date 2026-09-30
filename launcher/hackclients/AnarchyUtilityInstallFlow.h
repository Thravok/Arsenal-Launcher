// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QString>
#include <QStringList>

class QWidget;

namespace HackClients {

struct AnarchyUtilityInstallOptions {
    /** When true, show ReviewMessageBox before downloading. */
    bool requireReview = true;
    QString windowTitle;
};

/**
 * Resolve curated utilities for @p utilityIds, optionally review, then download into modsDir.
 * Returns true when install succeeded (or there was nothing new to install).
 */
bool resolveAndInstallAnarchyUtilities(const QString& modsDir,
                                       const QString& indexDir,
                                       const QString& minecraftVersion,
                                       const QStringList& utilityIds,
                                       QWidget* parent,
                                       const AnarchyUtilityInstallOptions& options = {});

}  // namespace HackClients
