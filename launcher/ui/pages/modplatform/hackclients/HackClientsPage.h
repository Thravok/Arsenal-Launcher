// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QWidget>

#include "ui/pages/BasePage.h"
#include <Application.h>

#include "hackclients/BaritoneProvider.h"
#include "hackclients/FDPProvider.h"
#include "hackclients/ImpactReleasesProvider.h"
#include "hackclients/LambdaProvider.h"
#include "hackclients/LiquidBounceProvider.h"
#include "hackclients/MeteorProvider.h"
#include "hackclients/WurstProvider.h"

namespace Ui {
class HackClientsPage;
}

class NewInstanceDialog;

class HackClientsPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit HackClientsPage(NewInstanceDialog* dialog, QWidget* parent = nullptr);
    ~HackClientsPage() override;

    QString displayName() const override { return tr("Hack Clients"); }
    QIcon icon() const override { return APPLICATION->getThemedIcon("loadermods"); }
    QString id() const override { return "hackclients"; }
    QString helpPage() const override { return QString(); }
    bool shouldDisplay() const override { return true; }
    void retranslate() override;
    void openedImpl() override;

   private slots:
    void onClientSelectionChanged();
    void onVersionChanged(int index);
    void onRefreshClicked();
    void onImpactReady();
    void onImpactFailed(QString reason);
    void onLiquidBounceReady();
    void onLiquidBounceFailed(QString reason);
    void onMeteorReady();
    void onMeteorFailed(QString reason);
    void onLambdaReady();
    void onLambdaFailed(QString reason);
    void onWurstReady();
    void onWurstFailed(QString reason);
    void onBaritoneReady();
    void onBaritoneFailed(QString reason);
    void onFDPReady();
    void onFDPFailed(QString reason);

   private:
    enum ClientKind {
        ClientNone = 0,
        ClientLiquidBounce = 1,
        ClientImpact = 2,
        ClientMeteor = 3,
        ClientLambda = 4,
        ClientBaritone = 5,
        ClientFDP = 6,
        ClientWurst = 7
    };

    void suggestCurrent();
    void updateStatus();
    void populateVersionCombo();
    int currentClientKind() const;
    bool clientNeedsVersion(int kind) const;
    bool clientVersionReady(int kind) const;

    NewInstanceDialog* dialog = nullptr;
    Ui::HackClientsPage* ui = nullptr;

    HackClients::ImpactReleasesProvider* m_impactProvider = nullptr;
    HackClients::LiquidBounceProvider* m_lbProvider = nullptr;
    HackClients::MeteorProvider* m_meteorProvider = nullptr;
    HackClients::LambdaProvider* m_lambdaProvider = nullptr;
    HackClients::WurstProvider* m_wurstProvider = nullptr;
    HackClients::BaritoneProvider* m_baritoneProvider = nullptr;
    HackClients::FDPProvider* m_fdpProvider = nullptr;
    bool m_impactOk = false;
    bool m_lbOk = false;
    bool m_meteorOk = false;
    bool m_lambdaOk = false;
    bool m_wurstOk = false;
    bool m_baritoneOk = false;
    bool m_fdpOk = false;
    QString m_impactError;
    QString m_lbError;
    QString m_meteorError;
    QString m_lambdaError;
    QString m_wurstError;
    QString m_baritoneError;
    QString m_fdpError;
};
