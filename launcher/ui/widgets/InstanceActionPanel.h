// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Arsenal Launcher
 *  Copyright (C) 2026 Arsenal Launcher Contributors
 */

#pragma once

#include <QFrame>
#include <QIcon>
#include <QList>
#include <QString>

class QAction;
class QToolButton;
class QPushButton;
class QLabel;
class QWidget;
class QHBoxLayout;
class QVBoxLayout;
class QMenu;

struct InstanceStatusChip {
    enum class Tone { Neutral, Success, Danger, Warning };
    QString text;
    Tone tone = Tone::Neutral;
};

struct RecentLaunchItem {
    QString id;
    QString name;
    QIcon icon;
    QString meta;
};

class InstanceActionPanel : public QFrame {
    Q_OBJECT

   public:
    explicit InstanceActionPanel(QWidget* parent = nullptr);

    QToolButton* iconButton() const { return m_iconButton; }
    QPushButton* nameButton() const { return m_nameButton; }
    QToolButton* launchButton() const { return m_launchButton; }
    QPushButton* killButton() const { return m_killButton; }
    QToolButton* accountMenuButton() const { return m_accountMenuButton; }
    QToolButton* editButton() const { return m_editButton; }

    /** Detail mode when an instance is selected; otherwise show the home desk. */
    void setDetailMode(bool active);

    void setHeaderText(const QString& name);
    void setStatusChips(const QList<InstanceStatusChip>& chips);
    void setAccountDisplay(const QIcon& face, const QString& title, const QString& metaLine, const QString& caption, bool enabled,
                           bool launchBlocked);

    void setHomeBrand(const QIcon& logo, const QString& title, const QString& subtitle);
    void setHomeRecentLaunches(const QList<RecentLaunchItem>& items);
    void setHomeNameProtectSummary(const QString& text);
    void setAddInstanceAction(QAction* action);

    /** Edit action becomes the footer gear; remaining actions go under More. */
    void setSecondaryActions(QAction* editAction, const QList<QAction*>& moreActions);

   signals:
    void recentInstanceRequested(const QString& id);

   protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void changeEvent(QEvent* event) override;

   private:
    void retranslateUi();
    void clearStatusChips();
    void clearRecentLaunchButtons();
    void openAccountMenu();
    void updateModeVisibility();

    bool m_detailMode = false;

    QWidget* m_homePage = nullptr;
    QLabel* m_homeLogo = nullptr;
    QLabel* m_homeTitle = nullptr;
    QLabel* m_homeSubtitle = nullptr;
    QLabel* m_homeRecentHeading = nullptr;
    QWidget* m_homeRecentContainer = nullptr;
    QVBoxLayout* m_homeRecentLayout = nullptr;
    QLabel* m_homeNameProtect = nullptr;
    QPushButton* m_homeAddButton = nullptr;
    QAction* m_addInstanceAction = nullptr;
    QList<QPushButton*> m_recentButtons;

    QWidget* m_detailPage = nullptr;
    QFrame* m_heroWell = nullptr;
    QToolButton* m_iconButton = nullptr;
    QPushButton* m_nameButton = nullptr;
    QWidget* m_chipsContainer = nullptr;
    QHBoxLayout* m_chipsLayout = nullptr;
    QList<QLabel*> m_chipLabels;
    QLabel* m_accountHeading = nullptr;
    QFrame* m_accountPicker = nullptr;
    QLabel* m_accountFace = nullptr;
    QLabel* m_accountTitle = nullptr;
    QLabel* m_accountMeta = nullptr;
    QToolButton* m_accountMenuButton = nullptr;
    QLabel* m_accountCaption = nullptr;

    QWidget* m_footer = nullptr;
    QToolButton* m_editButton = nullptr;
    QToolButton* m_launchButton = nullptr;
    QPushButton* m_killButton = nullptr;
    QToolButton* m_moreActionsButton = nullptr;
    QMenu* m_moreActionsMenu = nullptr;
};
