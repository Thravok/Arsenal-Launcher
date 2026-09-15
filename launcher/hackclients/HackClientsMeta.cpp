// SPDX-License-Identifier: GPL-3.0-only
#include "HackClientsMeta.h"

#include "Application.h"
#include "meta/Index.h"
#include "meta/VersionList.h"
#include "net/Mode.h"
#include "tasks/Task.h"

#include <QEventLoop>

namespace HackClients {

QString resolveRecommendedFabricLoader()
{
    auto index = APPLICATION->metadataIndex();
    if (!index)
        return {};

    if (!index->isLoaded()) {
        if (auto task = index->loadTask(Net::Mode::Online)) {
            QEventLoop loop;
            QObject::connect(task.get(), &Task::finished, &loop, &QEventLoop::quit);
            task->start();
            loop.exec();
        }
    }

    auto vlist = index->get("net.fabricmc.fabric-loader");
    if (!vlist)
        return {};

    vlist->waitToLoad();

    if (auto recommended = vlist->getRecommended())
        return recommended->descriptor();
    if (vlist->count() > 0)
        return vlist->at(0)->descriptor();
    return {};
}

}  // namespace HackClients
