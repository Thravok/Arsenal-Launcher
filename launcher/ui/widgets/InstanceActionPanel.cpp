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
#include <QMouseEvent>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

InstanceActionPanel::InstanceActionPanel(QWidget* parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("instanceActionPanel"));
    setFixedWidth(300);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    m_iconButton = new QToolButton(this);
    m_iconButton->setObjectName(QStringLiteral("instancePanelIconButton"));
    m_iconButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_iconButton->setIconSize(QSize(64, 64));
    m_iconButton->setAutoRaise(true);
    m_iconButton->setCursor(Qt::PointingHandCursor);

    m_nameButton = new QPushButton(this);
    m_nameButton->setObjectName(QStringLiteral("instancePanelName"));
    m_nameButton->setFlat(true);
    m_nameButton->setCursor(Qt::PointingHandCursor);
    m_nameButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    m_subtitleLabel = new QLabel(this);
    m_subtitleLabel->setObjectName(QStringLiteral("instancePanelSubtitle"));
    m_subtitleLabel->setWordWrap(true);

    root->addWidget(m_iconButton, 0, Qt::AlignHCenter);
    root->addWidget(m_nameButton);
    root->addWidget(m_subtitleLabel);

    auto* launchRow = new QHBoxLayout();
    launchRow->setSpacing(8);

    m_launchButton = new QToolButton(this);
    m_launchButton->setObjectName(QStringLiteral("launchPrimaryButton"));
    m_launchButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_launchButton->setPopupMode(QToolButton::MenuButtonPopup);
    m_launchButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_killButton = new QPushButton(this);
    m_killButton->setObjectName(QStringLiteral("instancePanelKillButton"));
    m_killButton->setFlat(true);

    launchRow->addWidget(m_launchButton, 1);
    launchRow->addWidget(m_killButton);
    root->addLayout(launchRow);

    m_accountHeading = new QLabel(this);
    m_accountHeading->setObjectName(QStringLiteral("instanceAccountHeading"));

    m_accountPicker = new QFrame(this);
    m_accountPicker->setObjectName(QStringLiteral("instanceAccountPicker"));
    m_accountPicker->setCursor(Qt::PointingHandCursor);
    m_accountPicker->installEventFilter(this);

    auto* pickerLayout = new QHBoxLayout(m_accountPicker);
    pickerLayout->setContentsMargins(10, 8, 6, 8);
    pickerLayout->setSpacing(10);

    m_accountFace = new QLabel(m_accountPicker);
    m_accountFace->setObjectName(QStringLiteral("instanceAccountFace"));
    m_accountFace->setFixedSize(32, 32);
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

    m_accountCaption = new QLabel(this);
    m_accountCaption->setObjectName(QStringLiteral("instanceAccountCaption"));
    m_accountCaption->setWordWrap(true);

    root->addWidget(m_accountHeading);
    root->addWidget(m_accountPicker);
    root->addWidget(m_accountCaption);

    auto* scroll = new QScrollArea(this);
    scroll->setObjectName(QStringLiteral("instancePanelActionsScroll"));
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_actionsContainer = new QWidget(scroll);
    m_actionsContainer->setObjectName(QStringLiteral("instancePanelActions"));
    m_actionsContainer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    m_actionsLayout = new QVBoxLayout(m_actionsContainer);
    m_actionsLayout->setContentsMargins(0, 0, 0, 0);
    m_actionsLayout->setSpacing(2);
    scroll->setWidget(m_actionsContainer);
    root->addWidget(scroll, 1);

    retranslateUi();
}

void InstanceActionPanel::setHeaderText(const QString& name, const QString& subtitle)
{
    m_nameButton->setText(name);
    m_subtitleLabel->setText(subtitle);
    m_subtitleLabel->setVisible(!subtitle.isEmpty());
}

void InstanceActionPanel::setAccountDisplay(const QIcon& face, const QString& title, const QString& metaLine,
                                            const QString& caption, bool enabled, bool launchBlocked)
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

void InstanceActionPanel::clearSecondaryActionButtons()
{
    for (QPushButton* button : m_actionButtons) {
        m_actionsLayout->removeWidget(button);
        button->deleteLater();
    }
    m_actionButtons.clear();
}

void InstanceActionPanel::setSecondaryActions(const QList<QAction*>& actions)
{
    clearSecondaryActionButtons();

    for (QAction* action : actions) {
        if (!action || action->isSeparator()) {
            continue;
        }
        auto* button = new QPushButton(m_actionsContainer);
        button->setObjectName(QStringLiteral("instancePanelLinkButton"));
        button->setFlat(true);
        button->setText(action->text());
        button->setToolTip(action->toolTip());
        button->setEnabled(action->isEnabled());
        connect(action, &QAction::changed, button, [button, action]() {
            button->setText(action->text());
            button->setToolTip(action->toolTip());
            button->setEnabled(action->isEnabled());
        });
        connect(button, &QPushButton::clicked, action, &QAction::trigger);
        m_actionsLayout->addWidget(button);
        m_actionButtons.append(button);
    }
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
}
