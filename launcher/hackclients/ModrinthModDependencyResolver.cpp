// SPDX-License-Identifier: GPL-3.0-only
#include "ModrinthModDependencyResolver.h"

#include "FileSystem.h"
#include "net/Request.h"

#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQueue>
#include <QSet>
#include <QUrlQuery>
#include <algorithm>

namespace HackClients {

namespace {

constexpr int kMaxDependencyDepth = 24;

QStringList parseRequiredProjectIds(const QJsonObject& versionObj)
{
    QStringList ids;
    for (const auto& depVal : versionObj.value(QStringLiteral("dependencies")).toArray()) {
        if (!depVal.isObject())
            continue;
        const auto dep = depVal.toObject();
        if (dep.value(QStringLiteral("dependency_type")).toString() != QLatin1String("required"))
            continue;
        const QString projectId = dep.value(QStringLiteral("project_id")).toString().trimmed();
        if (!projectId.isEmpty())
            ids.append(projectId);
    }
    return ids;
}

bool pickPrimaryFile(const QJsonObject& versionObj, QString* fileName, QUrl* url, QString* sha512)
{
    QJsonObject primaryFile;
    for (const auto& fileVal : versionObj.value(QStringLiteral("files")).toArray()) {
        if (!fileVal.isObject())
            continue;
        const auto file = fileVal.toObject();
        if (file.value(QStringLiteral("primary")).toBool(false)) {
            primaryFile = file;
            break;
        }
        if (primaryFile.isEmpty())
            primaryFile = file;
    }
    if (primaryFile.isEmpty())
        return false;
    *fileName = primaryFile.value(QStringLiteral("filename")).toString();
    *url = QUrl(primaryFile.value(QStringLiteral("url")).toString());
    const auto hashes = primaryFile.value(QStringLiteral("hashes")).toObject();
    *sha512 = hashes.value(QStringLiteral("sha512")).toString();
    return !fileName->isEmpty() && url->isValid();
}

QJsonObject pickBestVersion(const QJsonArray& versions)
{
    for (const auto& value : versions) {
        if (!value.isObject())
            continue;
        const auto obj = value.toObject();
        if (obj.value(QStringLiteral("version_type")).toString() == QLatin1String("release"))
            return obj;
    }
    if (!versions.isEmpty() && versions.first().isObject())
        return versions.first().toObject();
    return {};
}

}  // namespace

ModrinthModDependencyResolver::ModrinthModDependencyResolver(QNetworkAccessManager* network)
    : m_network(network)
{}

void ModrinthModDependencyResolver::abort()
{
    m_abort = true;
    if (m_job)
        m_job->abort();
}

bool ModrinthModDependencyResolver::fetchBytes(const QUrl& url, QByteArray* out)
{
    m_job.reset(new NetJob(QStringLiteral("Modrinth %1").arg(url.path()), m_network));
    auto [action, response] = Net::Request::makeByteArray(url);
    m_job->addNetAction(action);

    QEventLoop loop;
    bool ok = false;
    QObject::connect(m_job.get(), &NetJob::succeeded, &loop, [&] {
        ok = true;
        *out = *response;
    });
    QObject::connect(m_job.get(), &Task::finished, &loop, &QEventLoop::quit, Qt::QueuedConnection);
    m_job->start();
    loop.exec();
    m_job.reset();
    return ok && !m_abort;
}

QStringList ModrinthModDependencyResolver::jarHintsFor(const QString& slug, const QString& fileName)
{
    QStringList hints;
    if (!slug.isEmpty())
        hints.append(slug);

    // Prefer slug only. Do not invent a bare first token like "fabric" from "fabric-api-…".
    // If the filename starts with "slug-loader-", also add that longer prefix for replace.
    const QString base = QFileInfo(fileName).completeBaseName().toLower();
    const QString slugLower = slug.toLower();
    if (!slugLower.isEmpty() && base.startsWith(slugLower + QLatin1Char('-'))) {
        static const QStringList loaders = { QStringLiteral("fabric"), QStringLiteral("forge"),
                                             QStringLiteral("neoforge"), QStringLiteral("quilt"),
                                             QStringLiteral("rift"), QStringLiteral("liteloader") };
        for (const auto& loader : loaders) {
            const QString prefix = slugLower + QLatin1Char('-') + loader;
            if (base.startsWith(prefix + QLatin1Char('-')) || base == prefix) {
                if (!hints.contains(prefix, Qt::CaseInsensitive))
                    hints.append(prefix);
                break;
            }
        }
    }
    return hints;
}

bool ModrinthModDependencyResolver::fileMatchesJarHint(const QString& fileName, const QString& hint)
{
    if (hint.isEmpty() || fileName.isEmpty())
        return false;

    const QString lower = fileName.toLower();
    const QString h = hint.toLower();
    if (lower == h + QLatin1String(".jar"))
        return true;
    if (!lower.startsWith(h + QLatin1Char('-')))
        return false;

    // "litematica-" must not match "litematica-printer-…"; allow loader or version next.
    const QString rest = lower.mid(h.size() + 1);
    static const QStringList loaders = { QStringLiteral("fabric"), QStringLiteral("forge"),
                                         QStringLiteral("neoforge"), QStringLiteral("quilt"),
                                         QStringLiteral("rift"), QStringLiteral("liteloader") };
    for (const auto& loader : loaders) {
        if (rest.startsWith(loader + QLatin1Char('-')) || rest.startsWith(loader + QLatin1Char('.')) ||
            rest == loader + QLatin1String(".jar"))
            return true;
    }
    return !rest.isEmpty() && rest.at(0).isDigit();
}

void ModrinthModDependencyResolver::detectInstalled(const QString& modsDir, const QStringList& jarNameHints,
                                                    const QString& fileName, bool* alreadyInstalled,
                                                    bool* maybeInstalled)
{
    *alreadyInstalled = false;
    *maybeInstalled = false;
    const QDir mods(modsDir);
    if (!fileName.isEmpty() && mods.exists(fileName)) {
        *alreadyInstalled = true;
        return;
    }
    for (const auto& entry : mods.entryList(QDir::Files)) {
        for (const auto& hint : jarNameHints) {
            if (fileMatchesJarHint(entry, hint)) {
                *maybeInstalled = true;
                return;
            }
        }
    }
}

bool ModrinthModDependencyResolver::resolveProject(const QString& idOrSlug,
                                                   const QString& displayFallback,
                                                   const QStringList& jarNameHints,
                                                   bool isDependency,
                                                   bool isRoot,
                                                   const QString& requiredBy,
                                                   AnarchyUtilityResolvedFile* out,
                                                   QStringList* requiredProjectIds,
                                                   QString* error,
                                                   const StatusFn& status)
{
    if (status)
        status(QObject::tr("Looking up %1 on Modrinth…").arg(displayFallback.isEmpty() ? idOrSlug : displayFallback));

    QByteArray projectData;
    const QUrl projectUrl(QStringLiteral("https://api.modrinth.com/v2/project/%1").arg(idOrSlug));
    if (!fetchBytes(projectUrl, &projectData)) {
        *error = QObject::tr("Failed to query Modrinth project %1.").arg(idOrSlug);
        return false;
    }
    QJsonParseError parseError;
    const auto projectDoc = QJsonDocument::fromJson(projectData, &parseError);
    if (parseError.error != QJsonParseError::NoError || !projectDoc.isObject()) {
        *error = QObject::tr("Invalid Modrinth project response for %1.").arg(idOrSlug);
        return false;
    }
    const QJsonObject project = projectDoc.object();
    const QString slug = project.value(QStringLiteral("slug")).toString();
    const QString title = project.value(QStringLiteral("title")).toString();
    const QString projectId = project.value(QStringLiteral("id")).toString();
    const QString displayName = !title.isEmpty() ? title : (!displayFallback.isEmpty() ? displayFallback : slug);

    QUrl versionsUrl(QStringLiteral("https://api.modrinth.com/v2/project/%1/version").arg(projectId.isEmpty() ? idOrSlug : projectId));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("loaders"), QStringLiteral("[\"fabric\"]"));
    query.addQueryItem(QStringLiteral("game_versions"),
                       QStringLiteral("[\"%1\"]").arg(m_minecraftVersion));
    versionsUrl.setQuery(query);

    if (status)
        status(QObject::tr("Checking %1 for Minecraft %2…").arg(displayName, m_minecraftVersion));

    QByteArray versionsData;
    if (!fetchBytes(versionsUrl, &versionsData)) {
        *error = QObject::tr("Failed to query Modrinth versions for %1.").arg(displayName);
        return false;
    }
    const auto versionsDoc = QJsonDocument::fromJson(versionsData, &parseError);
    if (parseError.error != QJsonParseError::NoError || !versionsDoc.isArray()) {
        *error = QObject::tr("Invalid Modrinth versions response for %1.").arg(displayName);
        return false;
    }
    const QJsonArray versions = versionsDoc.array();
    if (versions.isEmpty()) {
        *error = QObject::tr("No Fabric release of %1 was found for Minecraft %2.")
                     .arg(displayName, m_minecraftVersion);
        return false;
    }

    const QJsonObject chosen = pickBestVersion(versions);
    QString fileName;
    QUrl downloadUrl;
    QString sha512;
    if (!pickPrimaryFile(chosen, &fileName, &downloadUrl, &sha512)) {
        *error = QObject::tr("Modrinth version for %1 has no downloadable file.").arg(displayName);
        return false;
    }

    QStringList hints = jarNameHints;
    if (hints.isEmpty())
        hints = jarHintsFor(slug, fileName);

    out->projectId = projectId;
    out->versionId = chosen.value(QStringLiteral("id")).toString();
    out->slug = slug.isEmpty() ? idOrSlug : slug;
    out->displayName = displayName;
    out->versionNumber = chosen.value(QStringLiteral("version_number")).toString();
    out->fileName = fileName;
    out->downloadUrl = downloadUrl;
    out->hashSha512 = sha512;
    out->jarNameHints = hints;
    out->isDependency = isDependency;
    out->isRoot = isRoot;
    detectInstalled(m_modsDir, hints, fileName, &out->alreadyInstalled, &out->maybeInstalled);
    if (!requiredBy.isEmpty())
        out->requiredBy = { requiredBy };

    *requiredProjectIds = parseRequiredProjectIds(chosen);
    return true;
}

bool ModrinthModDependencyResolver::resolve(const QString& minecraftVersion,
                                            const QString& modsDir,
                                            const QStringList& rootProjectIdsOrSlugs,
                                            QList<AnarchyUtilityResolvedFile>* out,
                                            QString* error,
                                            const StatusFn& status)
{
    m_abort = false;
    m_minecraftVersion = minecraftVersion;
    m_modsDir = modsDir;
    out->clear();

    if (m_minecraftVersion.isEmpty()) {
        *error = QObject::tr("Minecraft version is unknown.");
        return false;
    }
    if (rootProjectIdsOrSlugs.isEmpty()) {
        *error = QObject::tr("No projects to resolve.");
        return false;
    }

    struct QueueItem {
        QString idOrSlug;
        QString displayFallback;
        QStringList jarHints;
        bool isDependency = false;
        bool isRoot = false;
        QString requiredBy;
        int depth = 0;
    };

    QQueue<QueueItem> queue;
    for (const auto& root : rootProjectIdsOrSlugs) {
        queue.enqueue(QueueItem{ root, root, {}, false, true, {}, 0 });
    }

    QSet<QString> seenKeys;
    QHash<QString, QStringList> extraRequiredBy;
    QList<AnarchyUtilityResolvedFile> resolved;

    auto rememberRequiredBy = [&](const QString& key, const QString& parentName) {
        if (parentName.isEmpty())
            return;
        auto& list = extraRequiredBy[key.toLower()];
        if (!list.contains(parentName))
            list.append(parentName);
    };

    while (!queue.isEmpty()) {
        if (m_abort) {
            *error = QObject::tr("Aborted.");
            return false;
        }

        const QueueItem item = queue.dequeue();
        const QString seeKey = item.idOrSlug.toLower();
        if (seenKeys.contains(seeKey))
            continue;
        seenKeys.insert(seeKey);

        if (item.depth > kMaxDependencyDepth) {
            *error = QObject::tr("Dependency chain too deep while resolving Modrinth projects.");
            return false;
        }

        AnarchyUtilityResolvedFile file;
        QStringList requiredIds;
        if (!resolveProject(item.idOrSlug, item.displayFallback, item.jarHints, item.isDependency,
                            item.isRoot, item.requiredBy, &file, &requiredIds, error, status)) {
            return false;
        }

        if (!file.projectId.isEmpty())
            seenKeys.insert(file.projectId.toLower());
        if (!file.slug.isEmpty())
            seenKeys.insert(file.slug.toLower());

        for (const auto& parent : extraRequiredBy.value(seeKey)) {
            if (!file.requiredBy.contains(parent))
                file.requiredBy.append(parent);
        }
        for (const auto& parent : extraRequiredBy.value(file.projectId.toLower())) {
            if (!file.requiredBy.contains(parent))
                file.requiredBy.append(parent);
        }
        for (const auto& parent : extraRequiredBy.value(file.slug.toLower())) {
            if (!file.requiredBy.contains(parent))
                file.requiredBy.append(parent);
        }
        // Keep isRoot; only mark dependency if it was queued as one (don't flip roots).
        if (!file.isRoot && !file.requiredBy.isEmpty())
            file.isDependency = true;

        resolved.append(file);

        for (const auto& depId : requiredIds) {
            const QString depKey = depId.toLower();
            if (seenKeys.contains(depKey)) {
                rememberRequiredBy(depKey, file.displayName);
                for (auto& existing : resolved) {
                    if (existing.projectId.compare(depId, Qt::CaseInsensitive) == 0 ||
                        existing.slug.compare(depId, Qt::CaseInsensitive) == 0) {
                        if (!existing.requiredBy.contains(file.displayName))
                            existing.requiredBy.append(file.displayName);
                        if (!existing.isRoot)
                            existing.isDependency = true;
                    }
                }
                continue;
            }
            rememberRequiredBy(depKey, file.displayName);
            queue.enqueue(QueueItem{ depId, depId, {}, true, false, file.displayName, item.depth + 1 });
        }
    }

    std::stable_sort(resolved.begin(), resolved.end(), [](const AnarchyUtilityResolvedFile& a,
                                                          const AnarchyUtilityResolvedFile& b) {
        const bool aDepOnly = a.isDependency && !a.isRoot;
        const bool bDepOnly = b.isDependency && !b.isRoot;
        if (aDepOnly != bDepOnly)
            return aDepOnly && !bDepOnly;
        return a.displayName.localeAwareCompare(b.displayName) < 0;
    });

    *out = resolved;
    return true;
}

}  // namespace HackClients
