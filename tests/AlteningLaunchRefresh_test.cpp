#include <QDateTime>
#include <QTest>

#include "minecraft/auth/AccountData.h"
#include "minecraft/auth/MinecraftAccount.h"
#include "ui/LaunchAccountUtils.h"

class AlteningLaunchRefreshTest : public QObject {
    Q_OBJECT

   private:
    static MinecraftAccountPtr alteningWithAltToken()
    {
        auto account = MinecraftAccount::createTheAlteningFromApiKey(QStringLiteral("test-key"));
        account->accountData()->yggdrasilToken.extra[QStringLiteral("userName")] = QStringLiteral("user@alt.com");
        account->accountData()->yggdrasilToken.token = QStringLiteral("stale-access-token");
        account->accountData()->yggdrasilToken.validity = Validity::Certain;
        account->accountData()->yggdrasilToken.issueInstant = QDateTime::currentDateTimeUtc();
        account->accountData()->validity_ = Validity::Certain;
        account->accountData()->accountState = AccountState::Online;
        return account;
    }

   private slots:
    void shouldRefresh_alteningRelaunchesWithFreshAuth()
    {
        auto account = alteningWithAltToken();
        QVERIFY(account->shouldRefresh());
    }

    void shouldRefresh_alteningWithoutAltTokenDoesNotRefresh()
    {
        auto account = MinecraftAccount::createTheAlteningFromApiKey(QStringLiteral("test-key"));
        account->accountData()->validity_ = Validity::Certain;
        account->accountData()->yggdrasilToken.issueInstant = QDateTime::currentDateTimeUtc();
        QVERIFY(!account->shouldRefresh());
    }

    void shouldRefresh_msaRecentlyIssuedDoesNotRefresh()
    {
        auto account = MinecraftAccount::createBlankMSA();
        account->accountData()->validity_ = Validity::Certain;
        account->accountData()->yggdrasilToken.issueInstant = QDateTime::currentDateTimeUtc();
        account->accountData()->yggdrasilToken.notAfter = QDateTime();
        QVERIFY(!account->shouldRefresh());
    }

    void blocksLaunch_expiredAlteningWithAltTokenIsRecoverable()
    {
        auto account = alteningWithAltToken();
        account->accountData()->accountState = AccountState::Expired;
        QVERIFY(!LaunchAccountUtils::blocksLaunch(account));
    }

    void blocksLaunch_expiredAlteningWithoutAltTokenIsBlocked()
    {
        auto account = MinecraftAccount::createTheAlteningFromApiKey(QStringLiteral("test-key"));
        account->accountData()->accountState = AccountState::Expired;
        QVERIFY(LaunchAccountUtils::blocksLaunch(account));
    }

    void blocksLaunch_expiredMsaIsBlocked()
    {
        auto account = MinecraftAccount::createBlankMSA();
        account->accountData()->accountState = AccountState::Expired;
        QVERIFY(LaunchAccountUtils::blocksLaunch(account));
    }

    void blocksLaunch_onlineAlteningIsNotBlocked()
    {
        auto account = alteningWithAltToken();
        QVERIFY(!LaunchAccountUtils::blocksLaunch(account));
    }

    void blocksLaunch_goneIsBlocked()
    {
        auto account = alteningWithAltToken();
        account->accountData()->accountState = AccountState::Gone;
        QVERIFY(LaunchAccountUtils::blocksLaunch(account));
    }
};

QTEST_GUILESS_MAIN(AlteningLaunchRefreshTest)

#include "AlteningLaunchRefresh_test.moc"
