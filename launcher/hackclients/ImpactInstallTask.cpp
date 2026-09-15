// SPDX-License-Identifier: GPL-3.0-only
#include "ImpactInstallTask.h"

#include "Application.h"
#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"
#include "net/Request.h"
#include "settings/INISettingsObject.h"

#include <QDir>
#include <QDirIterator>
#include <QEventLoop>
#include <QFileInfo>
#include <QTemporaryDir>

namespace HackClients {

static const QUrl IMPACT_INSTALLER_URL(
    "https://github.com/ImpactDevelopment/Installer/releases/download/0.9.5/installer-0.9.5.jar");
static const QString IMPACT_INSTALLER_NAME = "ImpactInstaller-0.9.5.jar";

ImpactInstallTask::ImpactInstallTask(ImpactRelease release) : InstanceCreationTask(), m_release(std::move(release)) {}

ImpactInstallTask::~ImpactInstallTask()
{
    if (m_process) {
        m_process->kill();
        m_process->deleteLater();
        m_process = nullptr;
    }
}

bool ImpactInstallTask::abort()
{
    if (!canAbort())
        return false;
    m_abort = true;
    if (m_job)
        m_job->abort();
    if (m_process && m_process->state() != QProcess::NotRunning)
        m_process->kill();
    return Task::abort();
}

bool ImpactInstallTask::ensureInstallerJar(QString& outPath)
{
    auto entry = APPLICATION->metacache()->resolveEntry("hackclients", IMPACT_INSTALLER_NAME);
    outPath = entry->getFullPath();
    if (QFileInfo::exists(outPath) && QFileInfo(outPath).size() > 100000) {
        return true;
    }

    setStatus(tr("Downloading Impact Installer…"));
    m_job.reset();
    m_job.reset(new NetJob("Impact Installer", APPLICATION->network()));
    auto dl = Net::Request::makeCached(IMPACT_INSTALLER_URL, entry);
    m_job->addNetAction(dl);

    QEventLoop loop;
    bool ok = false;
    connect(m_job.get(), &NetJob::succeeded, &loop, [&] { ok = true; });
    connect(m_job.get(), &Task::finished, &loop, &QEventLoop::quit, Qt::QueuedConnection);
    m_job->start();
    loop.exec();
    m_job.reset();

    if (m_abort)
        return false;
    if (!ok || !QFileInfo::exists(outPath)) {
        setError(tr("Failed to download Impact Installer from GitHub."));
        return false;
    }
    return true;
}

bool ImpactInstallTask::runInstaller(const QString& installerJar, const QString& mmcRoot)
{
    QString javaPath = APPLICATION->settings()->get("JavaPath").toString();
    if (javaPath.isEmpty())
        javaPath = "java";

    setStatus(tr("Running Impact Installer for %1…").arg(m_release.tagName));

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    QStringList args;
    args << "-jar" << installerJar << "--no-gui"
         << "-m"
         << "MultiMC"
         << "-i" << m_release.tagName << "--mmc-dir" << mmcRoot << "--no-ga";

    QEventLoop loop;
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), &loop, &QEventLoop::quit);
    connect(m_process, &QProcess::readyRead, this, [this] {
        m_installerLog.append(QString::fromLocal8Bit(m_process->readAll()));
    });

    m_process->start(javaPath, args);
    if (!m_process->waitForStarted(15000)) {
        setError(tr("Could not start Java to run Impact Installer. Set a Java path in settings."));
        return false;
    }
    loop.exec();

    int code = m_process->exitCode();
    auto status = m_process->exitStatus();
    m_process->deleteLater();
    m_process = nullptr;

    if (m_abort)
        return false;
    if (status != QProcess::NormalExit || code != 0) {
        setError(tr("Impact Installer failed (exit %1):\n%2").arg(code).arg(m_installerLog.right(2000)));
        return false;
    }
    return true;
}

bool ImpactInstallTask::locateInstalledInstance(const QString& mmcRoot, QString& outInstancePath)
{
    // Installer may write to mmcRoot/instances or mmcRoot/Contents/MacOS/instances
    QStringList candidates;
    candidates << FS::PathCombine(mmcRoot, "instances");
    candidates << FS::PathCombine(mmcRoot, "Contents", "MacOS", "instances");
    candidates << FS::PathCombine(mmcRoot, "Contents", "Resources", "instances");

    for (const auto& base : candidates) {
        QDir dir(base);
        if (!dir.exists())
            continue;
        QDirIterator it(base, QDir::Dirs | QDir::NoDotAndDotDot);
        while (it.hasNext()) {
            it.next();
            auto path = it.filePath();
            if (QFileInfo::exists(FS::PathCombine(path, "mmc-pack.json"))) {
                outInstancePath = path;
                return true;
            }
        }
    }
    setError(tr("Impact Installer finished but no instance folder was found.\n%1").arg(m_installerLog.right(2000)));
    return false;
}

bool ImpactInstallTask::copyIntoStaging(const QString& sourceInstancePath)
{
    setStatus(tr("Importing Impact instance…"));
    if (!FS::ensureFolderPathExists(m_stagingPath)) {
        setError(tr("Could not create staging directory."));
        return false;
    }

    FS::copy copyTask(sourceInstancePath, m_stagingPath);
    copyTask.followSymlinks(true);
    if (!copyTask()) {
        setError(tr("Failed to copy Impact instance into staging."));
        return false;
    }

    // Apply user-chosen name / icon from InstanceTask
    const QString configPath = FS::PathCombine(m_stagingPath, "instance.cfg");
    auto settings = std::make_unique<INISettingsObject>(configPath);
    MinecraftInstance inst(m_globalSettings, std::move(settings), m_stagingPath);
    inst.setName(name());
    if (!m_instIcon.isEmpty())
        inst.setIconKey(m_instIcon);
    inst.saveNow();
    return true;
}

bool ImpactInstallTask::createInstance()
{
    if (m_release.tagName.isEmpty()) {
        setError(tr("No Impact version selected."));
        return false;
    }

    QString installerJar;
    if (!ensureInstallerJar(installerJar))
        return false;

    QTemporaryDir tempRoot;
    tempRoot.setAutoRemove(true);
    if (!tempRoot.isValid()) {
        setError(tr("Could not create temporary directory for Impact Installer."));
        return false;
    }

    // Seed a fake MultiMC root so the installer has somewhere to write
    QString mmcRoot = tempRoot.path();
    FS::ensureFolderPathExists(FS::PathCombine(mmcRoot, "instances"));

    if (!runInstaller(installerJar, mmcRoot))
        return false;

    QString installedPath;
    if (!locateInstalledInstance(mmcRoot, installedPath))
        return false;

    return copyIntoStaging(installedPath);
}

}  // namespace HackClients
