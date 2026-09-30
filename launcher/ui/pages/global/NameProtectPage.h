// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "ui/pages/BasePage.h"
#include <Application.h>

#include <QWidget>

namespace Ui {
class NameProtectPage;
}

class NameProtectPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit NameProtectPage(QWidget* parent = nullptr);
    ~NameProtectPage() override;

    QString displayName() const override { return tr("Name Protect"); }
    QIcon icon() const override { return APPLICATION->getThemedIcon("accounts"); }
    QString id() const override { return QStringLiteral("name-protect"); }
    QString helpPage() const override { return QString(); }
    bool apply() override;
    void retranslate() override;

   private:
    void loadSettings();
    void applySettings();

    Ui::NameProtectPage* ui;
};
