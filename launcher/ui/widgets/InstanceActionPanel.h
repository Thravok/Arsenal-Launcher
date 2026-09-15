// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Arsenal Launcher
 *  Copyright (C) 2026 Arsenal Launcher Contributors
 */

#pragma once

#include <QFrame>
#include <QIcon>
#include <QList>
#include <QVBoxLayout>

class QAction;
class QToolButton;
class QPushButton;
class QLabel;
class QScrollArea;

class InstanceActionPanel : public QFrame
{
    Q_OBJECT

public:
    explicit InstanceActionPanel(QWidget* parent = nullptr);

    QToolButton* iconButton() const { return m_iconButton; }
    QPushButton* nameButton() const { return m_nameButton; }
    QLabel* subtitleLabel() const { return m_subtitleLabel; }
    QToolButton* launchButton() const { return m_launchButton; }
    QPushButton* killButton() const { return m_killButton; }
    QToolButton* accountMenuButton() const { return m_accountMenuButton; }

    void setHeaderText(const QString& name, const QString& subtitle);
    void setAccountDisplay(const QIcon& face, const QString& title, const QString& metaLine, const QString& caption,
                           bool enabled, bool launchBlocked);
    void setSecondaryActions(const QList<QAction*>& actions);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void changeEvent(QEvent* event) override;

private:
    void retranslateUi();
    void clearSecondaryActionButtons();
    void openAccountMenu();

    QToolButton* m_iconButton = nullptr;
    QPushButton* m_nameButton = nullptr;
    QLabel* m_subtitleLabel = nullptr;
    QToolButton* m_launchButton = nullptr;
    QPushButton* m_killButton = nullptr;
    QLabel* m_accountHeading = nullptr;
    QFrame* m_accountPicker = nullptr;
    QLabel* m_accountFace = nullptr;
    QLabel* m_accountTitle = nullptr;
    QLabel* m_accountMeta = nullptr;
    QToolButton* m_accountMenuButton = nullptr;
    QLabel* m_accountCaption = nullptr;
    QWidget* m_actionsContainer = nullptr;
    QVBoxLayout* m_actionsLayout = nullptr;
    QList<QPushButton*> m_actionButtons;
};
