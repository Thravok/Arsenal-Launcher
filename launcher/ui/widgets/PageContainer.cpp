// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
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

#include "PageContainer.h"
#include "BuildConfig.h"
#include "PageContainer_p.h"

#include <QAction>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSortFilterProxyModel>
#include <QStackedLayout>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <utility>

#include "settings/SettingsObject.h"

#include "ui/widgets/IconLabel.h"

#include "Application.h"
#include "DesktopServices.h"

class PageEntryFilterModel : public QSortFilterProxyModel {
   public:
    explicit PageEntryFilterModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {}

   protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override
    {
        const QString pattern = filterRegularExpression().pattern();
        auto* const model = static_cast<PageModel*>(sourceModel());
        auto* const page = model->sidebarPageAt(sourceRow);
        if (!page->shouldDisplay()) {
            return false;
        }
        // Regular contents check, then check page-filter.
        return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    }
};

PageContainer::PageContainer(BasePageProvider* pageProvider, QString defaultId, QWidget* parent)
    : QWidget(parent)
    , m_proxyModel(new PageEntryFilterModel(this))
    , m_model(new PageModel(this))
{
    createUI();
    useSidebarStyle(true);

    int counter = 0;
    auto pages = pageProvider->getPages();
    for (auto* page : pages) {
        auto* widget = dynamic_cast<QWidget*>(page);
        widget->setParent(this);
        page->stackIndex = m_pageStack->addWidget(widget);
        page->listIndex = counter;
        page->setParentContainer(this);
        counter++;
        page->updateExtraInfo = [this](const QString& id, const QString& info) {
            if (m_currentPage && id == m_currentPage->id()) {
                m_header->setText(m_currentPage->displayName() + info);
            }
        };
    }
    m_model->setPages(pages);

    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    m_pageList->setIconSize(QSize(pageIconSize, pageIconSize));
    m_pageList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pageList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_pageList->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    m_pageList->setModel(m_proxyModel);
    connect(m_pageList->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &PageContainer::currentChanged);
    m_pageStack->setStackingMode(QStackedLayout::StackOne);
    if (m_model->hasGroupedPages()) {
        rebuildMorePlatformsMenu();
    }

    m_pageList->setFocus();
    selectPage(std::move(defaultId));
}

bool PageContainer::selectPage(QString pageId)
{
    BasePage* page = m_model->findPageEntryById(pageId);
    if (page && !page->shouldDisplay()) {
        page = nullptr;
    }
    if (!page && m_model->rowCount() > 0) {
        page = m_model->sidebarPageAt(0);
    }
    if (!page) {
        return false;
    }

    if (!page->sidebarGroup().isEmpty()) {
        QSignalBlocker blocker(m_pageList->selectionModel());
        m_pageList->setCurrentIndex(QModelIndex());
        setMoreButtonActivePage(page);
        BasePage* previous = m_currentPage;
        emit selectedPageChanged(previous, page);
        showPage(page);
        return true;
    }

    setMoreButtonActivePage(nullptr);
    const int sidebarRow = m_model->sidebarRowForPage(page);
    if (sidebarRow < 0) {
        return false;
    }
    const QModelIndex index = m_proxyModel->mapFromSource(m_model->index(sidebarRow, 0));
    if (index.isValid()) {
        m_pageList->setCurrentIndex(index);
        return true;
    }
    return false;
}

BasePage* PageContainer::getPage(QString pageId)
{
    return m_model->findPageEntryById(pageId);
}

BasePage* PageContainer::selectedPage() const
{
    return m_currentPage;
}

const QList<BasePage*>& PageContainer::getPages() const
{
    return m_model->pages();
}

void PageContainer::refreshContainer()
{
    m_proxyModel->invalidate();
    rebuildMorePlatformsMenu();
    if (m_currentPage && !m_currentPage->shouldDisplay()) {
        selectPage(QString());
    }
}

void PageContainer::createUI()
{
    m_pageStack = new QStackedLayout;
    m_pageList = new PageView;
    m_header = new QLabel();

    QFont headerLabelFont = m_header->font();
    headerLabelFont.setBold(true);
    const int pointSize = headerLabelFont.pointSize();
    if (pointSize > 0) {
        headerLabelFont.setPointSize(pointSize + 2);
    }
    m_header->setFont(headerLabelFont);

    auto* headerHLayout = new QHBoxLayout;
    const int leftMargin = APPLICATION->style()->pixelMetric(QStyle::PM_LayoutLeftMargin);
    headerHLayout->addSpacerItem(new QSpacerItem(leftMargin, 0, QSizePolicy::Fixed, QSizePolicy::Ignored));
    headerHLayout->addWidget(m_header);
    headerHLayout->setContentsMargins(0, 6, 0, 0);

    m_pageStack->setContentsMargins(0, 0, 0, 0);
    m_pageStack->addWidget(new QWidget(this));

    m_sidebarColumn = new QWidget(this);
    auto* sidebarLayout = new QVBoxLayout(m_sidebarColumn);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);
    sidebarLayout->setSpacing(4);
    sidebarLayout->addWidget(m_pageList, 1);

    m_morePlatformsButton = new QToolButton(m_sidebarColumn);
    m_morePlatformsButton->setObjectName(QStringLiteral("morePlatformsButton"));
    m_morePlatformsButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_morePlatformsButton->setPopupMode(QToolButton::InstantPopup);
    m_morePlatformsButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_morePlatformsButton->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));
    m_morePlatformsButton->setVisible(false);
    sidebarLayout->addWidget(m_morePlatformsButton);

    m_morePlatformsMenu = new QMenu(m_morePlatformsButton);
    m_morePlatformsButton->setMenu(m_morePlatformsMenu);

    m_layout = new QGridLayout;
    m_layout->addLayout(headerHLayout, 0, 1, 1, 1);
    m_layout->addWidget(m_sidebarColumn, 0, 0, 3, 1);
    m_layout->addLayout(m_pageStack, 1, 1, 1, 1);
    m_layout->setColumnStretch(1, 4);
    m_layout->setContentsMargins(0, 0, 0, 0);
    setLayout(m_layout);
}

void PageContainer::retranslate()
{
    if (m_currentPage) {
        m_header->setText(m_currentPage->displayName());
    }

    for (auto* page : m_model->pages()) {
        page->retranslate();
    }

    if (m_morePlatformsButton) {
        if (m_currentPage && !m_currentPage->sidebarGroup().isEmpty()) {
            setMoreButtonActivePage(m_currentPage);
        } else {
            setMoreButtonActivePage(nullptr);
        }
        rebuildMorePlatformsMenu();
    }
}

void PageContainer::rebuildMorePlatformsMenu()
{
    if (!m_morePlatformsMenu || !m_morePlatformsButton) {
        return;
    }

    m_morePlatformsMenu->clear();
    const bool hasGrouped = m_model->hasGroupedPages();
    m_morePlatformsButton->setVisible(hasGrouped);
    if (!hasGrouped) {
        return;
    }

    for (BasePage* page : m_model->groupedPages()) {
        if (!page->shouldDisplay()) {
            continue;
        }
        auto* action = m_morePlatformsMenu->addAction(page->icon(), page->displayName());
        action->setData(page->id());
        connect(action, &QAction::triggered, this, [this, pageId = page->id()]() { selectPage(pageId); });
    }
}

void PageContainer::setMoreButtonActivePage(BasePage* page)
{
    if (!m_morePlatformsButton) {
        return;
    }

    const bool secondaryActive = page != nullptr;
    if (secondaryActive) {
        m_morePlatformsButton->setText(page->displayName());
    } else {
        m_morePlatformsButton->setText(tr("More platforms"));
    }
    m_morePlatformsButton->setProperty("secondaryActive", secondaryActive);
    m_morePlatformsButton->style()->unpolish(m_morePlatformsButton);
    m_morePlatformsButton->style()->polish(m_morePlatformsButton);
    m_morePlatformsButton->update();
}

void PageContainer::addButtons(QWidget* buttons)
{
    m_layout->addWidget(buttons, 2, 1, 1, 2);
}

void PageContainer::addButtons(QLayout* buttons)
{
    m_layout->addLayout(buttons, 2, 1, 1, 2);
}

void PageContainer::useSidebarStyle(bool sidebar)
{
    m_pageList->setProperty("_kde_side_panel_view", sidebar);
}

void PageContainer::showPage(BasePage* page)
{
    if (m_currentPage) {
        m_currentPage->closed();
    }
    m_currentPage = page;
    if (m_currentPage) {
        m_pageStack->setCurrentIndex(m_currentPage->stackIndex);
        m_header->setText(m_currentPage->displayName());
        m_currentPage->opened();
    } else {
        m_pageStack->setCurrentIndex(0);
        m_header->setText(QString());
    }
}

void PageContainer::help()
{
    if (m_currentPage) {
        QString pageId = m_currentPage->helpPage();
        if (pageId.isEmpty()) {
            return;
        }
        DesktopServices::openUrl(QUrl(BuildConfig.HELP_URL.arg(pageId)));
    }
}

void PageContainer::currentChanged(const QModelIndex& current)
{
    if (!current.isValid()) {
        return;
    }

    const int selectedIndex = m_proxyModel->mapToSource(current).row();
    BasePage* selected = m_model->sidebarPageAt(selectedIndex);
    BasePage* previous = m_currentPage;

    setMoreButtonActivePage(nullptr);

    emit selectedPageChanged(previous, selected);

    showPage(selected);
}

bool PageContainer::prepareToClose()
{
    if (!saveAll()) {
        return false;
    }
    if (m_currentPage) {
        m_currentPage->closed();
    }
    return true;
}

bool PageContainer::saveAll()
{
    for (auto* page : m_model->pages()) {
        if (!page->apply()) {
            return false;
        }
    }
    return true;
}

void PageContainer::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslate();
    }
    QWidget::changeEvent(event);
}
