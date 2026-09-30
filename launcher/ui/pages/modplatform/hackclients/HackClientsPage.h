// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QHash>
#include <QList>
#include <QWidget>
#include <utility>

class QCheckBox;
class QToolButton;

#include "ui/pages/BasePage.h"
#include <Application.h>

#include "hackclients/BaritoneProvider.h"
#include "hackclients/EpsilonProvider.h"
#include "hackclients/FDPProvider.h"
#include "hackclients/HackClientCatalog.h"
#include "hackclients/ImpactReleasesProvider.h"
#include "hackclients/LambdaProvider.h"
#include "hackclients/LiquidBounceProvider.h"
#include "hackclients/MeteorProvider.h"
#include "hackclients/WurstProvider.h"

namespace Ui {
class HackClientsPage;
}

class NewInstanceDialog;

/** Shared metadata for all Hack Clients category pages in one New Instance dialog. */
class HackClientsPageContext : public QObject {
    Q_OBJECT

   public:
    struct ClientStatus {
        bool ok = false;
        QString error;
    };

    explicit HackClientsPageContext(QObject* parent = nullptr);

    void refresh(HackClients::HackClientCategory category, bool force);

    bool isOk(int clientId) const { return m_status.value(clientId).ok; }
    QString errorFor(int clientId) const { return m_status.value(clientId).error; }
    void setStatus(int clientId, bool ok, QString error = {})
    {
        auto& s = m_status[clientId];
        s.ok = ok;
        s.error = std::move(error);
    }

    HackClients::ImpactReleasesProvider* impactProvider = nullptr;
    HackClients::LiquidBounceProvider* lbProvider = nullptr;
    HackClients::MeteorProvider* meteorProvider = nullptr;
    HackClients::LambdaProvider* lambdaProvider = nullptr;
    HackClients::WurstProvider* wurstProvider = nullptr;
    HackClients::BaritoneProvider* baritoneProvider = nullptr;
    HackClients::FDPProvider* fdpProvider = nullptr;
    HackClients::EpsilonProvider* epsilonProvider = nullptr;

   signals:
    void statusChanged();

   private:
    QHash<int, ClientStatus> m_status;
};

class HackClientsPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit HackClientsPage(NewInstanceDialog* dialog,
                             HackClients::HackClientCategory category,
                             HackClientsPageContext* context,
                             QWidget* parent = nullptr);
    ~HackClientsPage() override;

    QString displayName() const override { return HackClients::hackClientCategoryTitle(m_category); }
    QIcon icon() const override { return APPLICATION->getThemedIcon("loadermods"); }
    QString id() const override { return HackClients::hackClientCategoryPageId(m_category); }
    QString helpPage() const override { return QString(); }
    bool shouldDisplay() const override { return true; }
    void retranslate() override;
    void openedImpl() override;

   private slots:
    void onClientSelectionChanged();
    void onVersionChanged(int index);
    void onRefreshClicked();
    void onCatalogStatusChanged();
    void onCardClicked();

   private:
    void populateClientList();
    void selectFirstClient();
    bool selectClientKind(int kind);
    void suggestCurrent();
    void updateStatus();
    QString statusLineForKind(int kind) const;
    void populateVersionCombo();
    int currentClientKind() const;
    bool clientNeedsVersion(int kind) const;
    bool clientVersionReady(int kind) const;
    QIcon iconForCatalogEntry(const HackClients::HackClientCatalogEntry& entry) const;
    void syncCardSelection();

    NewInstanceDialog* dialog = nullptr;
    Ui::HackClientsPage* ui = nullptr;
    HackClients::HackClientCategory m_category = HackClients::HackClientCategory::Anarchy;
    HackClientsPageContext* m_context = nullptr;

    QCheckBox* m_wurstBaritoneCheck = nullptr;
    QList<QToolButton*> m_clientCards;
    QWidget* m_catalogHost = nullptr;
    int m_lastClientKind = HackClients::ClientNone;
    bool m_updatingSelection = false;
};
