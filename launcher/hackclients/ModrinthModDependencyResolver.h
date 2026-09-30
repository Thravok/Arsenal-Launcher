// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "HackClientTypes.h"

#include "QObjectPtr.h"
#include "net/NetJob.h"

#include <QList>
#include <QNetworkAccessManager>
#include <QString>
#include <QStringList>
#include <functional>

namespace HackClients {

/**
 * Resolves Modrinth projects for a Minecraft version and walks required
 * dependencies (same idea as Feather / Prism "Download Mods" dep check).
 */
class ModrinthModDependencyResolver {
   public:
    using StatusFn = std::function<void(const QString&)>;

    explicit ModrinthModDependencyResolver(QNetworkAccessManager* network);

    void abort();

    /**
     * Resolve root projects (slug or project id) plus required Modrinth deps.
     * Dependencies are listed before the mods that need them.
     */
    bool resolve(const QString& minecraftVersion,
                 const QString& modsDir,
                 const QStringList& rootProjectIdsOrSlugs,
                 QList<AnarchyUtilityResolvedFile>* out,
                 QString* error,
                 const StatusFn& status = {});

    /** True if filename belongs to this project hint (won't treat litematica-printer as litematica). */
    static bool fileMatchesJarHint(const QString& fileName, const QString& hint);

    static void detectInstalled(const QString& modsDir, const QStringList& jarNameHints,
                                const QString& fileName, bool* alreadyInstalled, bool* maybeInstalled);

   private:
    bool fetchBytes(const QUrl& url, QByteArray* out);
    bool resolveProject(const QString& idOrSlug,
                        const QString& displayFallback,
                        const QStringList& jarNameHints,
                        bool isDependency,
                        bool isRoot,
                        const QString& requiredBy,
                        AnarchyUtilityResolvedFile* out,
                        QStringList* requiredProjectIds,
                        QString* error,
                        const StatusFn& status);
    static QStringList jarHintsFor(const QString& slug, const QString& fileName);

    QNetworkAccessManager* m_network = nullptr;
    QString m_minecraftVersion;
    QString m_modsDir;
    NetJob::Ptr m_job;
    bool m_abort = false;
};

}  // namespace HackClients
