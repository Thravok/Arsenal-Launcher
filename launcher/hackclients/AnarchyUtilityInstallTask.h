// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "HackClientTypes.h"
#include "tasks/Task.h"

#include "QObjectPtr.h"
#include "net/NetJob.h"

#include <QList>
#include <QString>

namespace HackClients {

/** Resolves curated utilities + Modrinth required dependencies (no downloads). */
class AnarchyUtilityResolveTask final : public Task {
    Q_OBJECT
   public:
    AnarchyUtilityResolveTask(QString modsDir, QString minecraftVersion, QStringList utilityIds,
                              QObject* parent = nullptr);

    bool abort() override;
    QList<AnarchyUtilityResolvedFile> plan() const { return m_plan; }

   protected:
    void executeTask() override;

   private:
    bool resolveGitHubRelease(const AnarchyUtilitySpec& spec, AnarchyUtilityResolvedFile* out, QString* error);

    QString m_modsDir;
    QString m_minecraftVersion;
    QStringList m_utilityIds;
    QList<AnarchyUtilityResolvedFile> m_plan;
    class ModrinthModDependencyResolver* m_resolver = nullptr;
    NetJob::Ptr m_job;
    bool m_abort = false;
};

/** Downloads an already-resolved Modrinth install plan into mods/. */
class AnarchyUtilityInstallTask final : public Task {
    Q_OBJECT
   public:
    AnarchyUtilityInstallTask(QString modsDir, QString indexDir, QList<AnarchyUtilityResolvedFile> plan,
                              QObject* parent = nullptr);

    bool abort() override;

   protected:
    void executeTask() override;

   private:
    bool downloadToTempThenReplace(const AnarchyUtilityResolvedFile& file);
    void removeMatchingJarsExcept(const QStringList& jarNameHints, const QString& keepFileName);
    bool writeModrinthMetadata(const AnarchyUtilityResolvedFile& file);

    QString m_modsDir;
    QString m_indexDir;
    QList<AnarchyUtilityResolvedFile> m_plan;
    NetJob::Ptr m_job;
    bool m_abort = false;
};

}  // namespace HackClients
