// SPDX-License-Identifier: GPL-3.0-only
#include "AnarchyUtilityInstallTask.h"

#include "AnarchyUtilityCatalog.h"
#include "Application.h"
#include "FileSystem.h"
#include "ModrinthModDependencyResolver.h"
#include "minecraft/mod/tasks/LocalResourceUpdateTask.h"
#include "modplatform/ModIndex.h"
#include "net/ChecksumValidator.h"
#include "net/Request.h"

#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace HackClients {

AnarchyUtilityResolveTask::AnarchyUtilityResolveTask(QString modsDir, QString minecraftVersion,
                                                     QStringList utilityIds, QObject* parent)
    : Task(parent)
    , m_modsDir(std::move(modsDir))
    , m_minecraftVersion(std::move(minecraftVersion))
    , m_utilityIds(std::move(utilityIds))
{
    setAbortable(true);
}

bool AnarchyUtilityResolveTask::abort()
{
    if (!canAbort())
        return false;
    m_abort = true;
    if (m_resolver)
        m_resolver->abort();
    if (m_job)
        m_job->abort();
    // Do not emitAborted here — nested QEventLoop may still be running.
    return true;
}

bool AnarchyUtilityResolveTask::resolveGitHubRelease(const AnarchyUtilitySpec& spec,
                                                     AnarchyUtilityResolvedFile* out,
                                                     QString* error)
{
    setStatus(tr("Looking up %1 on GitHub…").arg(spec.name));
    // Prefer full release list so we can match arsenal-nameprotect-mc{version}-*.jar
    const QUrl url(
        QStringLiteral("https://api.github.com/repos/%1/releases?per_page=30").arg(spec.githubReleasesRepo));

    m_job.reset(new NetJob(QStringLiteral("GitHub %1").arg(spec.githubReleasesRepo), APPLICATION->network()));
    auto [action, response] = Net::Request::makeByteArray(url);
    m_job->addNetAction(action);

    QEventLoop loop;
    bool ok = false;
    QByteArray data;
    connect(m_job.get(), &NetJob::succeeded, &loop, [&] {
        ok = true;
        data = *response;
    });
    connect(m_job.get(), &Task::finished, &loop, &QEventLoop::quit, Qt::QueuedConnection);
    m_job->start();
    loop.exec();
    m_job.reset();

    if (m_abort)
        return false;
    if (!ok) {
        *error = tr("Failed to query GitHub releases for %1.").arg(spec.name);
        return false;
    }

    QJsonParseError parseError;
    const auto doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        *error = tr("Invalid GitHub release response for %1.").arg(spec.name);
        return false;
    }

    const QString mcNeedle = QStringLiteral("-mc%1").arg(m_minecraftVersion);
    QString fileName;
    QUrl downloadUrl;
    QString tag;

    auto considerAsset = [&](const QJsonObject& asset, const QString& releaseTag, bool requireMcMatch) -> bool {
        const QString name = asset.value(QStringLiteral("name")).toString();
        if (!name.endsWith(QLatin1String(".jar"), Qt::CaseInsensitive))
            return false;
        if (name.contains(QLatin1String("-sources"), Qt::CaseInsensitive) ||
            name.contains(QLatin1String("-dev"), Qt::CaseInsensitive) ||
            name.contains(QLatin1String("-javadoc"), Qt::CaseInsensitive))
            return false;
        if (!spec.jarNameHints.isEmpty()) {
            bool hintOk = false;
            for (const auto& hint : spec.jarNameHints) {
                if (ModrinthModDependencyResolver::fileMatchesJarHint(name, hint)) {
                    hintOk = true;
                    break;
                }
            }
            if (!hintOk)
                return false;
        }
        if (requireMcMatch && !name.contains(mcNeedle, Qt::CaseInsensitive))
            return false;
        fileName = name;
        downloadUrl = QUrl(asset.value(QStringLiteral("browser_download_url")).toString());
        tag = releaseTag;
        return downloadUrl.isValid();
    };

    // Pass 1: exact Minecraft version in asset name
    for (const auto& value : doc.array()) {
        if (!value.isObject())
            continue;
        const auto obj = value.toObject();
        const QString releaseTag = obj.value(QStringLiteral("tag_name")).toString();
        for (const auto& assetVal : obj.value(QStringLiteral("assets")).toArray()) {
            if (!assetVal.isObject())
                continue;
            if (considerAsset(assetVal.toObject(), releaseTag, true))
                break;
        }
        if (!fileName.isEmpty())
            break;
    }

    // Pass 2: any matching jar hint (single-jar releases without -mc in the name)
    if (fileName.isEmpty()) {
        for (const auto& value : doc.array()) {
            if (!value.isObject())
                continue;
            const auto obj = value.toObject();
            const QString releaseTag = obj.value(QStringLiteral("tag_name")).toString();
            for (const auto& assetVal : obj.value(QStringLiteral("assets")).toArray()) {
                if (!assetVal.isObject())
                    continue;
                if (considerAsset(assetVal.toObject(), releaseTag, false))
                    break;
            }
            if (!fileName.isEmpty())
                break;
        }
    }

    if (fileName.isEmpty() || !downloadUrl.isValid()) {
        *error = tr("No JAR for Minecraft %1 found in %2 releases.")
                     .arg(m_minecraftVersion, spec.name);
        return false;
    }

    out->projectId = spec.githubReleasesRepo;
    out->slug = spec.id;
    out->displayName = spec.name;
    out->versionNumber = tag;
    out->fileName = fileName;
    out->downloadUrl = downloadUrl;
    out->jarNameHints = spec.jarNameHints;
    out->isRoot = true;
    out->isDependency = false;
    ModrinthModDependencyResolver::detectInstalled(m_modsDir, out->jarNameHints, out->fileName,
                                                   &out->alreadyInstalled, &out->maybeInstalled);
    return true;
}

void AnarchyUtilityResolveTask::executeTask()
{
    if (m_minecraftVersion.isEmpty()) {
        emitFailed(tr("Cannot resolve utilities: Minecraft version is unknown."));
        return;
    }
    if (m_utilityIds.isEmpty()) {
        emitFailed(tr("No utilities selected."));
        return;
    }

    QStringList modrinthRoots;
    QList<AnarchyUtilitySpec> githubSpecs;
    QHash<QString, QStringList> hintOverrides;

    for (const auto& id : m_utilityIds) {
        const auto spec = AnarchyUtilityCatalog::byId(id);
        if (spec.id.isEmpty()) {
            emitFailed(tr("Unknown utility: %1").arg(id));
            return;
        }
        if (!spec.jarNameHints.isEmpty()) {
            if (!spec.modrinthSlug.isEmpty())
                hintOverrides.insert(spec.modrinthSlug.toLower(), spec.jarNameHints);
            hintOverrides.insert(spec.id.toLower(), spec.jarNameHints);
        }
        if (!spec.githubReleasesRepo.isEmpty() && spec.modrinthSlug.isEmpty()) {
            githubSpecs.append(spec);
        } else if (!spec.modrinthSlug.isEmpty()) {
            modrinthRoots.append(spec.modrinthSlug);
        } else {
            emitFailed(tr("Utility %1 has no download source.").arg(spec.name));
            return;
        }
    }

    if (!modrinthRoots.isEmpty()) {
        ModrinthModDependencyResolver resolver(APPLICATION->network());
        m_resolver = &resolver;

        QString error;
        QList<AnarchyUtilityResolvedFile> modrinthPlan;
        if (!resolver.resolve(
                m_minecraftVersion, m_modsDir, modrinthRoots, &modrinthPlan, &error,
                [this](const QString& status) { setStatus(status); })) {
            m_resolver = nullptr;
            if (m_abort) {
                emitAborted();
                return;
            }
            emitFailed(error.isEmpty() ? tr("Failed to resolve Modrinth dependencies.") : error);
            return;
        }
        m_resolver = nullptr;
        m_plan.append(modrinthPlan);
    }

    for (const auto& spec : githubSpecs) {
        if (m_abort) {
            emitAborted();
            return;
        }
        AnarchyUtilityResolvedFile file;
        QString error;
        if (!resolveGitHubRelease(spec, &file, &error)) {
            if (m_abort) {
                emitAborted();
                return;
            }
            emitFailed(error);
            return;
        }
        m_plan.append(file);
    }

    // Apply catalog jar hints for any plan entry matching a catalog slug/id (including transitive deps).
    const auto catalog = AnarchyUtilityCatalog::all();
    for (auto& file : m_plan) {
        auto it = hintOverrides.constFind(file.slug.toLower());
        if (it == hintOverrides.cend())
            it = hintOverrides.constFind(file.projectId.toLower());
        if (it != hintOverrides.cend() && !it->isEmpty())
            file.jarNameHints = *it;
        else {
            for (const auto& entry : catalog) {
                if (!entry.modrinthSlug.isEmpty() &&
                    (file.slug.compare(entry.modrinthSlug, Qt::CaseInsensitive) == 0 ||
                     file.projectId.compare(entry.modrinthSlug, Qt::CaseInsensitive) == 0) &&
                    !entry.jarNameHints.isEmpty()) {
                    file.jarNameHints = entry.jarNameHints;
                    break;
                }
            }
        }
        // Recompute install detection after hint overrides
        ModrinthModDependencyResolver::detectInstalled(m_modsDir, file.jarNameHints, file.fileName,
                                                       &file.alreadyInstalled, &file.maybeInstalled);
    }

    if (m_plan.isEmpty()) {
        emitFailed(tr("Nothing to install."));
        return;
    }

    setStatus(tr("Checked %1 project(s) (including dependencies).").arg(m_plan.size()));
    emitSucceeded();
}

AnarchyUtilityInstallTask::AnarchyUtilityInstallTask(QString modsDir, QString indexDir,
                                                     QList<AnarchyUtilityResolvedFile> plan,
                                                     QObject* parent)
    : Task(parent)
    , m_modsDir(std::move(modsDir))
    , m_indexDir(std::move(indexDir))
    , m_plan(std::move(plan))
{
    setAbortable(true);
}

bool AnarchyUtilityInstallTask::abort()
{
    if (!canAbort())
        return false;
    m_abort = true;
    if (m_job)
        m_job->abort();
    return true;
}

bool AnarchyUtilityInstallTask::downloadToTempThenReplace(const AnarchyUtilityResolvedFile& file)
{
    FS::ensureFolderPathExists(m_modsDir);
    const QString destPath = FS::PathCombine(m_modsDir, file.fileName);
    const QString tempPath = destPath + QStringLiteral(".part");

    QFile::remove(tempPath);

    m_job.reset(new NetJob(QString("Download %1").arg(file.fileName), APPLICATION->network()));
    auto action = Net::Request::makeFile(file.downloadUrl, tempPath);
    if (!file.hashSha512.isEmpty()) {
        action->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha512, file.hashSha512));
    }
    m_job->addNetAction(action);

    QEventLoop loop;
    bool ok = false;
    connect(m_job.get(), &NetJob::succeeded, &loop, [&] { ok = true; });
    connect(m_job.get(), &Task::finished, &loop, &QEventLoop::quit, Qt::QueuedConnection);
    m_job->start();
    loop.exec();
    m_job.reset();

    if (!ok || m_abort || !QFileInfo::exists(tempPath)) {
        QFile::remove(tempPath);
        return false;
    }

    // Only remove older jars after a successful download.
    removeMatchingJarsExcept(file.jarNameHints, file.fileName);
    QFile::remove(destPath);
    if (!QFile::rename(tempPath, destPath)) {
        // Fallback copy+remove if rename fails across devices
        if (!QFile::copy(tempPath, destPath)) {
            QFile::remove(tempPath);
            return false;
        }
        QFile::remove(tempPath);
    }
    return QFileInfo::exists(destPath);
}

void AnarchyUtilityInstallTask::removeMatchingJarsExcept(const QStringList& jarNameHints,
                                                        const QString& keepFileName)
{
    if (jarNameHints.isEmpty())
        return;
    const QDir mods(m_modsDir);
    for (const auto& entry : mods.entryList(QDir::Files)) {
        if (entry.compare(keepFileName, Qt::CaseInsensitive) == 0)
            continue;
        for (const auto& hint : jarNameHints) {
            if (ModrinthModDependencyResolver::fileMatchesJarHint(entry, hint)) {
                QFile::remove(FS::PathCombine(m_modsDir, entry));
                break;
            }
        }
    }
}

bool AnarchyUtilityInstallTask::writeModrinthMetadata(const AnarchyUtilityResolvedFile& file)
{
    if (m_indexDir.isEmpty() || file.projectId.isEmpty() || file.projectId.contains(QLatin1Char('/'))) {
        // Skip GitHub-only entries (owner/repo as projectId) — no Modrinth index format.
        return true;
    }

    ModPlatform::IndexedPack pack;
    pack.addonId = file.projectId;
    pack.name = file.displayName;
    pack.slug = file.slug;
    pack.provider = ModPlatform::ResourceProvider::MODRINTH;
    pack.websiteUrl = QStringLiteral("https://modrinth.com/mod/%1").arg(file.slug);

    ModPlatform::IndexedVersion version;
    version.addonId = file.projectId;
    version.fileId = file.versionId;
    version.version = file.versionNumber;
    version.versionNumber = file.versionNumber;
    version.fileName = file.fileName;
    version.downloadUrl = file.downloadUrl.toString();
    version.hashType = file.hashSha512.isEmpty() ? QString() : QStringLiteral("sha512");
    version.hash = file.hashSha512;
    version.loaders = ModPlatform::Fabric;

    LocalResourceUpdateTask metaTask(QDir(m_indexDir), pack, version);
    QEventLoop loop;
    bool ok = false;
    connect(&metaTask, &Task::succeeded, &loop, [&] { ok = true; });
    connect(&metaTask, &Task::finished, &loop, &QEventLoop::quit, Qt::QueuedConnection);
    metaTask.start();
    loop.exec();
    return ok;
}

void AnarchyUtilityInstallTask::executeTask()
{
    if (m_plan.isEmpty()) {
        emitFailed(tr("Nothing to install."));
        return;
    }

    FS::ensureFolderPathExists(m_modsDir);

    for (const auto& file : m_plan) {
        if (m_abort) {
            emitAborted();
            return;
        }
        if (file.alreadyInstalled)
            continue;

        setStatus(tr("Downloading %1 %2…").arg(file.displayName, file.versionNumber));
        if (!downloadToTempThenReplace(file)) {
            if (m_abort) {
                emitAborted();
                return;
            }
            emitFailed(tr("Failed to download %1.").arg(file.displayName));
            return;
        }

        setStatus(tr("Updating metadata for %1…").arg(file.displayName));
        if (!writeModrinthMetadata(file)) {
            // Non-fatal: jar is installed even if index write fails.
            qWarning() << "Failed to write Modrinth metadata for" << file.displayName;
        }
    }

    emitSucceeded();
}

}  // namespace HackClients
