// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Arsenal Launcher
 *  Copyright (C) 2026 Arsenal Launcher Contributors
 */

#include "InstanceActionPanel.h"

#include <QAction>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QSizePolicy>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

InstanceActionPanel::InstanceActionPanel(QWidget* parent) : QFrame(parent)
{
    setObjectName(QStringLiteral("instanceActionPanel"));
    setFixedWidth(340);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setMinimumWidth(340);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 14);
    root->setSpacing(10);

    // --- Home (idle) page ---
    m_homePage = new QWidget(this);
    m_homePage->setObjectName(QStringLiteral("instancePanelHome"));
    auto* homeLayout = new QVBoxLayout(m_homePage);
    homeLayout->setContentsMargins(0, 0, 0, 0);
    homeLayout->setSpacing(10);

    m_homeLogo = new QLabel(m_homePage);
    m_homeLogo->setObjectName(QStringLiteral("instancePanelHomeLogo"));
    m_homeLogo->setFixedSize(56, 56);
    m_homeLogo->setScaledContents(true);
    m_homeLogo->setAlignment(Qt::AlignCenter);

    m_homeTitle = new QLabel(m_homePage);
    m_homeTitle->setObjectName(QStringLiteral("instancePanelHomeTitle"));
    m_homeTitle->setAlignment(Qt::AlignHCenter);

    m_homeSubtitle = new QLabel(m_homePage);
    m_homeSubtitle->setObjectName(QStringLiteral("instancePanelHomeSubtitle"));
    m_homeSubtitle->setWordWrap(true);
    m_homeSubtitle->setAlignment(Qt::AlignHCenter);

    m_homeRecentHeading = new QLabel(m_homePage);
    m_homeRecentHeading->setObjectName(QStringLiteral("instanceAccountHeading"));

    m_homeRecentContainer = new QWidget(m_homePage);
    m_homeRecentContainer->setObjectName(QStringLiteral("instancePanelRecentList"));
    m_homeRecentLayout = new QVBoxLayout(m_homeRecentContainer);
    m_homeRecentLayout->setContentsMargins(0, 0, 0, 0);
    m_homeRecentLayout->setSpacing(6);

    m_homeNameProtect = new QLabel(m_homePage);
    m_homeNameProtect->setObjectName(QStringLiteral("instancePanelHomeNameProtect"));
    m_homeNameProtect->setWordWrap(true);

    m_homeAddButton = new QPushButton(m_homePage);
    m_homeAddButton->setObjectName(QStringLiteral("instancePanelHomeAddButton"));
    m_homeAddButton->setCursor(Qt::PointingHandCursor);
    connect(m_homeAddButton, &QPushButton::clicked, this, [this]() {
        if (m_addInstanceAction) {
            m_addInstanceAction->trigger();
        }
    });

    homeLayout->addWidget(m_homeLogo, 0, Qt::AlignHCenter);
    homeLayout->addWidget(m_homeTitle);
    homeLayout->addWidget(m_homeSubtitle);
    homeLayout->addSpacing(6);
    homeLayout->addWidget(m_homeRecentHeading);
    homeLayout->addWidget(m_homeRecentContainer);
    homeLayout->addStretch(1);
    homeLayout->addWidget(m_homeNameProtect);
    homeLayout->addWidget(m_homeAddButton);

    // --- Detail (selected instance) page ---
    m_detailPage = new QWidget(this);
    m_detailPage->setObjectName(QStringLiteral("instancePanelDetail"));
    auto* detailLayout = new QVBoxLayout(m_detailPage);
    detailLayout->setContentsMargins(0, 0, 0, 0);
    detailLayout->setSpacing(10);

    m_heroWell = new QFrame(m_detailPage);
    m_heroWell->setObjectName(QStringLiteral("instancePanelHero"));
    m_heroWell->setFixedHeight(120);
    auto* heroLayout = new QVBoxLayout(m_heroWell);
    heroLayout->setContentsMargins(12, 12, 12, 12);
    heroLayout->setAlignment(Qt::AlignCenter);

    m_iconButton = new QToolButton(m_heroWell);
    m_iconButton->setObjectName(QStringLiteral("instancePanelIconButton"));
    m_iconButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_iconButton->setIconSize(QSize(72, 72));
    m_iconButton->setAutoRaise(true);
    m_iconButton->setCursor(Qt::PointingHandCursor);
    heroLayout->addWidget(m_iconButton, 0, Qt::AlignCenter);

    m_nameButton = new QPushButton(m_detailPage);
    m_nameButton->setObjectName(QStringLiteral("instancePanelName"));
    m_nameButton->setFlat(true);
    m_nameButton->setCursor(Qt::PointingHandCursor);
    m_nameButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    m_chipsContainer = new QWidget(m_detailPage);
    m_chipsContainer->setObjectName(QStringLiteral("instancePanelChips"));
    m_chipsLayout = new QHBoxLayout(m_chipsContainer);
    m_chipsLayout->setContentsMargins(0, 0, 0, 0);
    m_chipsLayout->setSpacing(6);
    m_chipsLayout->addStretch(1);

    m_accountHeading = new QLabel(m_detailPage);
    m_accountHeading->setObjectName(QStringLiteral("instanceAccountHeading"));

    m_accountPicker = new QFrame(m_detailPage);
    m_accountPicker->setObjectName(QStringLiteral("instanceAccountPicker"));
    m_accountPicker->setCursor(Qt::PointingHandCursor);
    m_accountPicker->installEventFilter(this);

    auto* pickerLayout = new QHBoxLayout(m_accountPicker);
    pickerLayout->setContentsMargins(12, 10, 8, 10);
    pickerLayout->setSpacing(10);

    m_accountFace = new QLabel(m_accountPicker);
    m_accountFace->setObjectName(QStringLiteral("instanceAccountFace"));
    m_accountFace->setFixedSize(36, 36);
    m_accountFace->setScaledContents(true);

    auto* textColumn = new QVBoxLayout();
    textColumn->setSpacing(2);
    textColumn->setContentsMargins(0, 0, 0, 0);

    m_accountTitle = new QLabel(m_accountPicker);
    m_accountTitle->setObjectName(QStringLiteral("instanceAccountTitle"));

    m_accountMeta = new QLabel(m_accountPicker);
    m_accountMeta->setObjectName(QStringLiteral("instanceAccountMeta"));

    textColumn->addWidget(m_accountTitle);
    textColumn->addWidget(m_accountMeta);

    m_accountMenuButton = new QToolButton(m_accountPicker);
    m_accountMenuButton->setObjectName(QStringLiteral("instanceAccountMenuButton"));
    m_accountMenuButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_accountMenuButton->setPopupMode(QToolButton::InstantPopup);
    m_accountMenuButton->setAutoRaise(true);
    m_accountMenuButton->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));

    pickerLayout->addWidget(m_accountFace);
    pickerLayout->addLayout(textColumn, 1);
    pickerLayout->addWidget(m_accountMenuButton);

    m_accountCaption = new QLabel(m_detailPage);
    m_accountCaption->setObjectName(QStringLiteral("instanceAccountCaption"));
    m_accountCaption->setWordWrap(true);

    m_footer = new QWidget(m_detailPage);
    m_footer->setObjectName(QStringLiteral("instancePanelFooter"));
    auto* footerLayout = new QHBoxLayout(m_footer);
    footerLayout->setContentsMargins(0, 4, 0, 0);
    footerLayout->setSpacing(6);
    footerLayout->setAlignment(Qt::AlignVCenter);

    constexpr int kFooterButtonHeight = 40;

    m_editButton = new QToolButton(m_footer);
    m_editButton->setObjectName(QStringLiteral("instancePanelEditButton"));
    m_editButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_editButton->setIconSize(QSize(18, 18));
    m_editButton->setFixedSize(kFooterButtonHeight, kFooterButtonHeight);
    m_editButton->setAutoRaise(false);
    m_editButton->setCursor(Qt::PointingHandCursor);
    m_editButton->setFocusPolicy(Qt::NoFocus);

    m_launchButton = new QToolButton(m_footer);
    m_launchButton->setObjectName(QStringLiteral("launchPrimaryButton"));
    m_launchButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_launchButton->setPopupMode(QToolButton::MenuButtonPopup);
    m_launchButton->setFixedHeight(kFooterButtonHeight);
    m_launchButton->setMinimumWidth(118);
    m_launchButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_launchButton->setCursor(Qt::PointingHandCursor);
    m_launchButton->setFocusPolicy(Qt::NoFocus);

    m_killButton = new QPushButton(m_footer);
    m_killButton->setObjectName(QStringLiteral("instancePanelKillButton"));
    m_killButton->setFlat(false);
    m_killButton->setFixedHeight(kFooterButtonHeight);
    m_killButton->setMinimumWidth(118);
    m_killButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_killButton->setCursor(Qt::PointingHandCursor);
    m_killButton->setFocusPolicy(Qt::NoFocus);
    m_killButton->setVisible(false);

    m_moreActionsMenu = new QMenu(this);
    m_moreActionsButton = new QToolButton(m_footer);
    m_moreActionsButton->setObjectName(QStringLiteral("instancePanelMoreButton"));
    m_moreActionsButton->setPopupMode(QToolButton::InstantPopup);
    m_moreActionsButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_moreActionsButton->setAutoRaise(false);
    m_moreActionsButton->setMenu(m_moreActionsMenu);
    m_moreActionsButton->setCursor(Qt::PointingHandCursor);
    m_moreActionsButton->setFixedHeight(kFooterButtonHeight);
    m_moreActionsButton->setFocusPolicy(Qt::NoFocus);

    footerLayout->addWidget(m_editButton, 0, Qt::AlignVCenter);
    footerLayout->addWidget(m_launchButton, 1);
    footerLayout->addWidget(m_killButton, 1);
    footerLayout->addWidget(m_moreActionsButton, 0, Qt::AlignVCenter);

    detailLayout->addWidget(m_heroWell);
    detailLayout->addWidget(m_nameButton);
    detailLayout->addWidget(m_chipsContainer);
    detailLayout->addSpacing(4);
    detailLayout->addWidget(m_accountHeading);
    detailLayout->addWidget(m_accountPicker);
    detailLayout->addWidget(m_accountCaption);
    detailLayout->addStretch(1);
    detailLayout->addWidget(m_footer);

    root->addWidget(m_homePage, 1);
    root->addWidget(m_detailPage, 1);

    retranslateUi();
    updateModeVisibility();
}

void InstanceActionPanel::setDetailMode(bool active)
{
    if (m_detailMode == active) {
        return;
    }
    m_detailMode = active;
    updateModeVisibility();
}

void InstanceActionPanel::updateModeVisibility()
{
    m_homePage->setVisible(!m_detailMode);
    m_detailPage->setVisible(m_detailMode);
}

void InstanceActionPanel::setHeaderText(const QString& name)
{
    m_nameButton->setText(name);
}

void InstanceActionPanel::clearStatusChips()
{
    for (QLabel* chip : m_chipLabels) {
        m_chipsLayout->removeWidget(chip);
        chip->deleteLater();
    }
    m_chipLabels.clear();
}

void InstanceActionPanel::setStatusChips(const QList<InstanceStatusChip>& chips)
{
    clearStatusChips();

    const int stretchIndex = m_chipsLayout->count() - 1;
    int insertAt = qMax(0, stretchIndex);

    for (const InstanceStatusChip& chip : chips) {
        if (chip.text.isEmpty()) {
            continue;
        }
        auto* label = new QLabel(chip.text, m_chipsContainer);
        label->setObjectName(QStringLiteral("instanceStatusChip"));
        QString tone;
        switch (chip.tone) {
            case InstanceStatusChip::Tone::Success:
                tone = QStringLiteral("success");
                break;
            case InstanceStatusChip::Tone::Danger:
                tone = QStringLiteral("danger");
                break;
            case InstanceStatusChip::Tone::Warning:
                tone = QStringLiteral("warning");
                break;
            case InstanceStatusChip::Tone::Neutral:
            default:
                tone = QStringLiteral("neutral");
                break;
        }
        label->setProperty("tone", tone);
        label->style()->unpolish(label);
        label->style()->polish(label);
        m_chipsLayout->insertWidget(insertAt++, label);
        m_chipLabels.append(label);
    }

    m_chipsContainer->setVisible(!m_chipLabels.isEmpty());
}

void InstanceActionPanel::setAccountDisplay(const QIcon& face, const QString& title, const QString& metaLine, const QString& caption,
                                            bool enabled, bool launchBlocked)
{
    const QPixmap facePixmap = face.pixmap(m_accountFace->size(), enabled ? QIcon::Normal : QIcon::Disabled);
    m_accountFace->setPixmap(facePixmap.isNull() ? QPixmap() : facePixmap);
    m_accountTitle->setText(title);
    m_accountMeta->setText(metaLine);
    m_accountMeta->setVisible(!metaLine.isEmpty());
    m_accountCaption->setText(caption);
    m_accountCaption->setVisible(!caption.isEmpty());

    m_accountPicker->setEnabled(enabled);
    m_accountMenuButton->setEnabled(enabled);
    m_accountHeading->setEnabled(enabled);

    m_accountPicker->setProperty("launchBlocked", launchBlocked);
    m_accountCaption->setProperty("launchBlocked", launchBlocked);
    m_accountMeta->setProperty("launchBlocked", launchBlocked);

    const auto polish = [](QWidget* widget) {
        widget->style()->unpolish(widget);
        widget->style()->polish(widget);
        widget->update();
    };
    polish(m_accountPicker);
    polish(m_accountCaption);
    polish(m_accountMeta);
}

void InstanceActionPanel::setHomeBrand(const QIcon& logo, const QString& title, const QString& subtitle)
{
    const QPixmap pixmap = logo.pixmap(m_homeLogo->size());
    m_homeLogo->setPixmap(pixmap.isNull() ? QPixmap() : pixmap);
    m_homeTitle->setText(title);
    m_homeSubtitle->setText(subtitle);
    m_homeSubtitle->setVisible(!subtitle.isEmpty());
}

void InstanceActionPanel::clearRecentLaunchButtons()
{
    for (QPushButton* button : m_recentButtons) {
        m_homeRecentLayout->removeWidget(button);
        button->deleteLater();
    }
    m_recentButtons.clear();
}

void InstanceActionPanel::setHomeRecentLaunches(const QList<RecentLaunchItem>& items)
{
    clearRecentLaunchButtons();

    for (const RecentLaunchItem& item : items) {
        auto* button = new QPushButton(m_homeRecentContainer);
        button->setObjectName(QStringLiteral("instancePanelRecentButton"));
        button->setCursor(Qt::PointingHandCursor);
        button->setIcon(item.icon);
        button->setIconSize(QSize(28, 28));
        if (item.meta.isEmpty()) {
            button->setText(item.name);
        } else {
            button->setText(QStringLiteral("%1\n%2").arg(item.name, item.meta));
        }
        const QString id = item.id;
        connect(button, &QPushButton::clicked, this, [this, id]() { emit recentInstanceRequested(id); });
        m_homeRecentLayout->addWidget(button);
        m_recentButtons.append(button);
    }

    const bool hasRecent = !m_recentButtons.isEmpty();
    m_homeRecentHeading->setVisible(hasRecent);
    m_homeRecentContainer->setVisible(hasRecent);
}

void InstanceActionPanel::setHomeNameProtectSummary(const QString& text)
{
    m_homeNameProtect->setText(text);
    m_homeNameProtect->setVisible(!text.isEmpty());
}

void InstanceActionPanel::setAddInstanceAction(QAction* action)
{
    m_addInstanceAction = action;
    if (action) {
        m_homeAddButton->setText(action->text());
        m_homeAddButton->setIcon(action->icon());
        m_homeAddButton->setToolTip(action->toolTip());
    }
}

void InstanceActionPanel::setSecondaryActions(QAction* editAction, const QList<QAction*>& moreActions)
{
    if (editAction) {
        m_editButton->setDefaultAction(editAction);
        m_editButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
        m_editButton->setVisible(true);
    } else {
        m_editButton->setDefaultAction(nullptr);
        m_editButton->setVisible(false);
    }

    m_moreActionsMenu->clear();
    for (QAction* action : moreActions) {
        if (!action) {
            continue;
        }
        if (action->isSeparator()) {
            m_moreActionsMenu->addSeparator();
            continue;
        }
        m_moreActionsMenu->addAction(action);
    }

    const bool hasMore = !m_moreActionsMenu->isEmpty();
    m_moreActionsButton->setVisible(hasMore);
}

bool InstanceActionPanel::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_accountPicker && event->type() == QEvent::MouseButtonRelease) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton && m_accountPicker->isEnabled()) {
            openAccountMenu();
            return true;
        }
    }
    return QFrame::eventFilter(watched, event);
}

void InstanceActionPanel::openAccountMenu()
{
    if (m_accountMenuButton->menu()) {
        const QPoint menuAnchor = m_accountMenuButton->mapToGlobal(QPoint(0, m_accountMenuButton->height()));
        m_accountMenuButton->menu()->exec(menuAnchor);
    }
}

void InstanceActionPanel::changeEvent(QEvent* event)
{
    QFrame::changeEvent(event);
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
}

void InstanceActionPanel::retranslateUi()
{
    m_launchButton->setText(tr("Launch"));
    m_killButton->setText(tr("Kill"));
    m_accountHeading->setText(tr("Launch account"));
    m_homeRecentHeading->setText(tr("Recent"));
    m_moreActionsButton->setText(tr("More"));
    m_moreActionsButton->setToolTip(tr("More instance actions"));
    if (m_addInstanceAction) {
        m_homeAddButton->setText(m_addInstanceAction->text());
        m_homeAddButton->setToolTip(m_addInstanceAction->toolTip());
    } else {
        m_homeAddButton->setText(tr("Add Instance"));
    }
}
