#include "InstanceCreationTask.h"

#include <QDebug>
#include <QFile>

InstanceCreationTask::InstanceCreationTask() = default;

void InstanceCreationTask::executeTask()
{
    setAbortable(true);

    if (updateInstance()) {
        emitSucceeded();
        return;
    }

    if (m_abort) {
        emitAborted();
        return;
    }

    if (!createInstance()) {
        if (m_abort)
            return;

        qWarning() << "Instance creation failed!";
        if (!m_error_message.isEmpty())
            qWarning() << "Reason: " << m_error_message;
        emitFailed(tr("Error while creating new instance."));
        return;
    }

    if (shouldOverride()) {
        setAbortable(false);
        setStatus(tr("Removing old conflicting files..."));
        qDebug() << "Removing old files";

        for (auto path : m_files_to_remove) {
            if (!QFile::exists(path))
                continue;
            qDebug() << "Removing" << path;
            if (!QFile::remove(path)) {
                qCritical() << "Couldn't remove the old conflicting files.";
                emitFailed(tr("Failed to remove old conflicting files."));
                return;
            }
        }
    }

    emitSucceeded();
}
