// SPDX-License-Identifier: GPL-3.0-only
#include "MeteorAddonInstallTask.h"

#include "Application.h"
#include "FileSystem.h"
#include "net/Request.h"

namespace HackClients {

MeteorAddonInstallTask::MeteorAddonInstallTask(QString modsDir, QUrl downloadUrl, QString destFileName,
                                               QObject* parent)
    : Task(parent)
    , m_modsDir(std::move(modsDir))
    , m_downloadUrl(std::move(downloadUrl))
    , m_destFileName(std::move(destFileName))
{
    setAbortable(true);
}

bool MeteorAddonInstallTask::abort()
{
    if (!canAbort())
        return false;
    m_abort = true;
    if (m_job)
        m_job->abort();
    return Task::abort();
}

void MeteorAddonInstallTask::executeTask()
{
    if (m_downloadUrl.isEmpty() || !m_downloadUrl.isValid()) {
        emitFailed(tr("No download URL for this addon."));
        return;
    }

    QString fileName = m_destFileName;
    if (fileName.isEmpty())
        fileName = m_downloadUrl.fileName();
    if (fileName.isEmpty()) {
        emitFailed(tr("Could not determine the addon file name."));
        return;
    }

    FS::ensureFolderPathExists(m_modsDir);
    const QString destPath = FS::PathCombine(m_modsDir, fileName);

    setStatus(tr("Downloading %1…").arg(fileName));
    m_job.reset(new NetJob(QString("Meteor addon %1").arg(fileName), APPLICATION->network()));
    m_job->addNetAction(Net::Request::makeFile(m_downloadUrl, destPath));

    connect(m_job.get(), &NetJob::succeeded, this, [this] {
        if (m_abort) {
            emitAborted();
            return;
        }
        emitSucceeded();
    });
    connect(m_job.get(), &NetJob::failed, this, [this](QString reason) { emitFailed(reason); });
    connect(m_job.get(), &NetJob::aborted, this, [this] { emitAborted(); });

    m_job->start();
}

}  // namespace HackClients
