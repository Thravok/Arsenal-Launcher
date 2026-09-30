// SPDX-License-Identifier: GPL-3.0-only
#include "BaritoneModInstallTask.h"

#include "Application.h"
#include "BaritoneMaven.h"
#include "FileSystem.h"
#include "HackClientInstanceDetect.h"
#include "net/Mode.h"
#include "net/Request.h"

#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QUrl>

namespace HackClients {

BaritoneModInstallTask::BaritoneModInstallTask(QString modsDir, QString minecraftVersion, QString mavenVersion,
                                                 QObject* parent)
    : Task(parent)
    , m_modsDir(std::move(modsDir))
    , m_minecraftVersion(std::move(minecraftVersion))
    , m_mavenVersion(std::move(mavenVersion))
{
    setAbortable(true);
}

bool BaritoneModInstallTask::abort()
{
    if (!canAbort())
        return false;
    m_abort = true;
    if (m_job)
        m_job->abort();
    return Task::abort();
}

bool BaritoneModInstallTask::downloadMeteorBaritone(const QString& destPath)
{
    const QUrl url(QString("https://meteorclient.com/api/downloadBaritone?version=%1")
                       .arg(QString::fromUtf8(QUrl::toPercentEncoding(m_minecraftVersion))));

    setStatus(tr("Downloading Meteor Baritone for Minecraft %1…").arg(m_minecraftVersion));
    FS::ensureFilePathExists(destPath);

    m_job.reset(new NetJob(QString("Download %1").arg(url.fileName()), APPLICATION->network()));
    m_job->addNetAction(Net::Request::makeFile(url, destPath));

    QEventLoop loop;
    bool ok = false;
    connect(m_job.get(), &NetJob::succeeded, &loop, [&] { ok = true; });
    connect(m_job.get(), &Task::finished, &loop, &QEventLoop::quit, Qt::QueuedConnection);
    m_job->start();
    loop.exec();
    m_job.reset();

    return ok && !m_abort && QFileInfo::exists(destPath);
}

void BaritoneModInstallTask::executeTask()
{
    if (m_minecraftVersion.isEmpty()) {
        emitFailed(tr("Cannot install Baritone: Minecraft version is unknown."));
        return;
    }

    FS::ensureFolderPathExists(m_modsDir);

    setStatus(tr("Removing older Baritone jars…"));
    const QDir mods(m_modsDir);
    for (const auto& entry : mods.entryList(QDir::Files)) {
        if (entry.contains("baritone", Qt::CaseInsensitive))
            QFile::remove(FS::PathCombine(m_modsDir, entry));
    }

    // Meteor Client needs Meteor's Baritone fork (mod id baritone-meteor), not standalone Fabric Baritone.
    const bool useMeteorBaritone = modsFolderHasMeteorClient(m_modsDir);
    if (useMeteorBaritone) {
        const QString destPath =
            FS::PathCombine(m_modsDir, QString("baritone-meteor-%1.jar").arg(m_minecraftVersion));
        if (!downloadMeteorBaritone(destPath)) {
            QFile::remove(destPath);
            if (m_abort) {
                emitAborted();
                return;
            }
            emitFailed(tr("Failed to download Meteor Baritone for Minecraft %1 from meteorclient.com.")
                           .arg(m_minecraftVersion));
            return;
        }
        if (m_abort) {
            emitAborted();
            return;
        }
        emitSucceeded();
        return;
    }

    const QString destName = QString("baritone-fabric-%1.jar").arg(m_minecraftVersion);
    QString error;
    const bool ok = m_mavenVersion.isEmpty()
                        ? BaritoneMaven::downloadBaritoneForMinecraft(
                              APPLICATION->network(), m_minecraftVersion, FS::PathCombine(m_modsDir, destName), &error,
                              [this](const QString& status) { setStatus(status); })
                        : BaritoneMaven::downloadBaritone(APPLICATION->network(), m_mavenVersion, m_minecraftVersion,
                                                          FS::PathCombine(m_modsDir, destName), &error,
                                                          [this](const QString& status) { setStatus(status); });

    if (m_abort) {
        emitAborted();
        return;
    }
    if (!ok) {
        emitFailed(error.isEmpty() ? tr("Failed to download Baritone.") : error);
        return;
    }
    emitSucceeded();
}

}  // namespace HackClients
