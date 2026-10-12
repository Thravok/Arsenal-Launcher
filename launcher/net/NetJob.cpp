// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2023 Rachel Powers <508861+Ryex@users.noreply.github.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "NetJob.h"
#include <QNetworkReply>
#include "net/Request.h"
#include "tasks/ConcurrentTask.h"
#if defined(LAUNCHER_APPLICATION)
#include <QApplication>
#include <QDialog>
#include "Application.h"
#include "settings/SettingsObject.h"
#include "ui/dialogs/NetworkJobFailedDialog.h"
#endif

NetJob::NetJob(QString job_name, QNetworkAccessManager* network, int max_concurrent) : ConcurrentTask(job_name), m_network(network)
{
#if defined(LAUNCHER_APPLICATION)
    if (APPLICATION_DYN && max_concurrent < 0)
        max_concurrent = APPLICATION->settings()->get("NumberOfConcurrentDownloads").toInt();
#endif
    if (max_concurrent > 0)
        setMaxConcurrent(max_concurrent);
}

auto NetJob::addNetAction(Net::Request::Ptr action) -> bool
{
    action->setNetwork(m_network);

    addTask(action);

    return true;
}

void NetJob::executeNextSubTask()
{
    // We're finished, check for failures and retry if we can (up to 3 times)
    if (isRunning() && m_queue.isEmpty() && m_doing.isEmpty() && !m_failed.isEmpty() && m_try < 3) {
        auto isRetryableRequest = [](Task* task) {
            auto* request = dynamic_cast<Net::Request*>(task);
            // Only auto-retry network requests; skip non-Request tasks (e.g. account refresh).
            // There is also no point in retrying a 404 Not Found.
            return request && request->replyStatusCode() != 404;
        };
        bool anyRetryable = false;
        for (auto task : m_failed) {
            if (isRetryableRequest(task.get())) {
                anyRetryable = true;
                break;
            }
        }
        if (anyRetryable) {
            m_try += 1;
            m_failed.removeIf([this, &isRetryableRequest](QHash<Task*, Task::Ptr>::iterator task) {
                if (!isRetryableRequest(task->get())) {
                    return false;
                }
                m_done.remove(task->get());
                m_queue.enqueue(*task);
                return true;
            });
        }
    }
    ConcurrentTask::executeNextSubTask();
}

auto NetJob::size() const -> int
{
    return m_queue.size() + m_doing.size() + m_done.size();
}

auto NetJob::canAbort() const -> bool
{
    bool canFullyAbort = true;

    // can abort the downloads on the queue?
    for (auto part : m_queue)
        canFullyAbort &= part->canAbort();

    // can abort the active downloads?
    for (auto part : m_doing)
        canFullyAbort &= part->canAbort();

    return canFullyAbort;
}

auto NetJob::abort() -> bool
{
    // Dismiss an open manual-retry dialog without finishing the job a second time.
    if (m_retryDialog) {
        m_retryDialog->disconnect(this);
        m_retryDialog->close();
        m_retryDialog.clear();
    }

    // fail all downloads on the queue
    for (auto task : m_queue)
        m_failed.insert(task.get(), task);
    m_queue.clear();

    if (m_doing.isEmpty()) {
        // No in-flight downloads. Still finish if we are waiting on the manual retry
        // dialog (task stays Running until that dialog resolves).
        if (isRunning()) {
            emitAborted();
        }
        return true;
    }

    bool fullyAborted = true;

    // abort active downloads
    auto toKill = m_doing.values();
    for (auto part : toKill) {
        fullyAborted &= part->abort();
    }

    if (fullyAborted)
        emitAborted();
    else
        emitFailed(tr("Failed to abort all tasks in the NetJob!"));

    return fullyAborted;
}

auto NetJob::getFailedActions() -> QList<Net::Request*>
{
    QList<Net::Request*> failed;
    for (auto index : m_failed) {
        if (auto* request = dynamic_cast<Net::Request*>(index.get())) {
            failed.push_back(request);
        }
    }
    return failed;
}

auto NetJob::getFailedFiles() -> QList<QString>
{
    QList<QString> failed;
    for (auto* request : getFailedActions()) {
        failed.append(request->url().toString());
    }
    return failed;
}

void NetJob::updateState()
{
    emit progress(m_done.count(), totalSize());
    setStatus(tr("Executing %1 task(s) (%2 out of %3 are done)")
                  .arg(QString::number(m_doing.count()), QString::number(m_done.count()), QString::number(totalSize())));
}

bool NetJob::isOnline()
{
    // check some errors that are ussually associated with the lack of internet
    for (auto job : getFailedActions()) {
        auto err = job->error();
        if (err != QNetworkReply::HostNotFoundError && err != QNetworkReply::NetworkSessionFailedError) {
            return true;
        }
    }
    return false;
};

void NetJob::emitFailed(QString reason)
{
#if defined(LAUNCHER_APPLICATION)

    if (APPLICATION_DYN && m_ask_retry && m_manual_try < APPLICATION->settings()->get("NumberOfManualRetries").toInt() && isOnline()) {
        m_manual_try++;
        auto failed = getFailedActions();
        // Parent to the active window (usually ProgressDialog) so the retry UI is not
        // stuck behind an application-modal "Please wait..." dialog and leave the task open.
        QWidget* activeWindow = QApplication::activeWindow();
        auto dialog = new NetworkJobFailedDialog(objectName(), m_try, m_done.size(), failed.size(), activeWindow);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        m_retryDialog = dialog;

        for (const auto& request : failed) {
            dialog->addFailedRequest(request->url(), request->errorString());
        }

        dialog->open();

        connect(dialog, &QDialog::finished, this, [this, reason = std::move(reason)](int result) {
            m_retryDialog.clear();
            // Abort may have already finished the job while this dialog was open.
            if (!isRunning()) {
                return;
            }
            if (result == QDialog::Accepted) {
                m_try = 0;
                executeNextSubTask();
            } else {
                ConcurrentTask::emitFailed(reason);
            }
        });

        return;
    }
#endif

    ConcurrentTask::emitFailed(reason);
}

void NetJob::setAskRetry(bool askRetry)
{
    m_ask_retry = askRetry;
}
