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

namespace {

bool finalizeBaritoneInstall(const QString& modsDir, const QString& destPath, const QString& tempPath, QString* errorOut)
{
    QFile::remove(destPath);
    if (!QFile::rename(tempPath, destPath) && !QFile::copy(tempPath, destPath)) {
        QFile::remove(tempPath);
        if (errorOut)
            *errorOut = QObject::tr("Failed to install Baritone into the mods folder.");
        return false;
    }
    QFile::remove(tempPath);
    replaceOtherBaritoneJars(modsDir, QFileInfo(destPath).fileName());
    return true;
}

}  // namespace

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

    // Meteor Client needs Meteor's Baritone fork (mod id baritone-meteor), not standalone Fabric Baritone.
    const bool useMeteorBaritone = modsFolderHasMeteorClient(m_modsDir);
    if (useMeteorBaritone) {
        const QString destPath =
            FS::PathCombine(m_modsDir, QString("baritone-meteor-%1.jar").arg(m_minecraftVersion));
        const QString tempPath = destPath + QStringLiteral(".part");
        QFile::remove(tempPath);

        if (!downloadMeteorBaritone(tempPath)) {
            QFile::remove(tempPath);
            if (m_abort) {
                emitAborted();
                return;
            }
            emitFailed(tr("Failed to download Meteor Baritone for Minecraft %1 from meteorclient.com.")
                           .arg(m_minecraftVersion));
            return;
        }
        if (m_abort) {
            QFile::remove(tempPath);
            emitAborted();
            return;
        }

        setStatus(tr("Installing Baritone…"));
        QString installError;
        if (!finalizeBaritoneInstall(m_modsDir, destPath, tempPath, &installError)) {
            emitFailed(installError);
            return;
        }
        emitSucceeded();
        return;
    }

    const QString destName = QString("baritone-fabric-%1.jar").arg(m_minecraftVersion);
    const QString destPath = FS::PathCombine(m_modsDir, destName);
    const QString tempPath = destPath + QStringLiteral(".part");
    QFile::remove(tempPath);

    QString error;
    const bool ok = m_mavenVersion.isEmpty()
                        ? BaritoneMaven::downloadBaritoneForMinecraft(
                              APPLICATION->network(), m_minecraftVersion, tempPath, &error,
                              [this](const QString& status) { setStatus(status); })
                        : BaritoneMaven::downloadBaritone(APPLICATION->network(), m_mavenVersion, m_minecraftVersion,
                                                          tempPath, &error,
                                                          [this](const QString& status) { setStatus(status); });

    if (m_abort) {
        QFile::remove(tempPath);
        emitAborted();
        return;
    }
    if (!ok) {
        QFile::remove(tempPath);
        emitFailed(error.isEmpty() ? tr("Failed to download Baritone.") : error);
        return;
    }

    // Only drop previous Baritone JARs after the new file is on disk.
    setStatus(tr("Installing Baritone…"));
    QString installError;
    if (!finalizeBaritoneInstall(m_modsDir, destPath, tempPath, &installError)) {
        emitFailed(installError);
        return;
    }
    emitSucceeded();
}

}  // namespace HackClients
