#include <QDateTime>
#include <QTest>

#include <Usable.h>
#include <minecraft/auth/MinecraftAccount.h>
#include <ui/LaunchAccountUtils.h>

class LaunchAccountTest : public QObject {
    Q_OBJECT
   private slots:
    void test_offlineUuidMatchesJavaNameUuid()
    {
        QCOMPARE(MinecraftAccount::uuidFromUsername(QStringLiteral("Notch")).toString(QUuid::Id128),
                 QStringLiteral("b50ad385829d3141a2167e7d7539ba7f"));
        QCOMPARE(MinecraftAccount::uuidFromUsername(QStringLiteral("Steve")).toString(QUuid::Id128),
                 QStringLiteral("5627dd98e6be3c21b8a8e92344183641"));
        QCOMPARE(MinecraftAccount::uuidFromUsername(QStringLiteral("Player")).toString(QUuid::Id128),
                 QStringLiteral("a01e3843e5213998958af459800e4d11"));

        auto account = MinecraftAccount::createOffline(QStringLiteral("Notch"));
        QCOMPARE(account->accountType(), AccountType::Offline);
        QCOMPARE(account->typeString(), QStringLiteral("offline"));
        QCOMPARE(account->profileName(), QStringLiteral("Notch"));
        QCOMPARE(account->profileId(), QStringLiteral("b50ad385829d3141a2167e7d7539ba7f"));
        QVERIFY(!account->ownsMinecraft());
    }

    void test_alteningReportsMojangUserType()
    {
        auto account = MinecraftAccount::createTheAlteningFromApiKey(QStringLiteral("  key  "));
        QCOMPARE(account->accountType(), AccountType::TheAltening);
        QCOMPARE(account->typeString(), QStringLiteral("mojang"));
        QCOMPARE(account->accountData()->theAlteningApiKey, QStringLiteral("key"));
        QVERIFY(account->ownsMinecraft());
    }

    void test_msaTypeString()
    {
        auto account = MinecraftAccount::createBlankMSA();
        QCOMPARE(account->typeString(), QStringLiteral("msa"));
    }

    void test_displayNameWarnsOnBrokenAccounts()
    {
        auto account = MinecraftAccount::createOffline(QStringLiteral("Steve"));
        QCOMPARE(account->displayName(), QStringLiteral("Steve"));

        account->accountData()->accountState = AccountState::Online;
        QCOMPARE(account->displayName(), QStringLiteral("Steve"));

        account->accountData()->accountState = AccountState::Expired;
        QCOMPARE(account->displayName(), QStringLiteral("⚠ Steve"));

        account->accountData()->accountState = AccountState::Disabled;
        QCOMPARE(account->displayName(), QStringLiteral("⚠ Steve"));

        account->accountData()->accountState = AccountState::Gone;
        QCOMPARE(account->displayName(), QStringLiteral("⚠ Steve"));

        account->accountData()->accountState = AccountState::Errored;
        QCOMPARE(account->displayName(), QStringLiteral("⚠ Steve"));
    }

    void test_shouldRefresh()
    {
        auto account = MinecraftAccount::createOffline(QStringLiteral("Steve"));
        account->accountData()->validity_ = Validity::None;
        QVERIFY(!account->shouldRefresh());

        account->accountData()->validity_ = Validity::Assumed;
        QVERIFY(account->shouldRefresh());

        account->accountData()->validity_ = Validity::Certain;
        account->accountData()->yggdrasilToken.issueInstant = QDateTime::currentDateTimeUtc().addSecs(-3600);
        account->accountData()->yggdrasilToken.notAfter = QDateTime::currentDateTimeUtc().addSecs(20 * 3600);
        QVERIFY(!account->shouldRefresh());

        account->accountData()->yggdrasilToken.notAfter = QDateTime::currentDateTimeUtc().addSecs(3600);
        QVERIFY(account->shouldRefresh());

        account->accountData()->yggdrasilToken.notAfter = QDateTime();
        account->accountData()->yggdrasilToken.issueInstant = QDateTime::currentDateTimeUtc().addSecs(-23 * 3600);
        QVERIFY(account->shouldRefresh());

        UseLock lock(account.get());
        QVERIFY(!account->shouldRefresh());
    }

    void test_blocksLaunch()
    {
        QVERIFY(LaunchAccountUtils::blocksLaunch(nullptr));
        QVERIFY(!LaunchAccountUtils::launchBlockReason(nullptr).isEmpty());

        auto account = MinecraftAccount::createOffline(QStringLiteral("Steve"));
        const AccountState blocked[] = { AccountState::Expired, AccountState::Disabled, AccountState::Gone };
        for (auto state : blocked) {
            account->accountData()->accountState = state;
            QVERIFY(LaunchAccountUtils::blocksLaunch(account));
            QVERIFY(!LaunchAccountUtils::launchBlockReason(account).isEmpty());
        }

        const AccountState allowed[] = { AccountState::Unchecked, AccountState::Offline, AccountState::Working, AccountState::Online,
                                         AccountState::Errored };
        for (auto state : allowed) {
            account->accountData()->accountState = state;
            QVERIFY(!LaunchAccountUtils::blocksLaunch(account));
        }
    }

    void test_accountLabels()
    {
        QVERIFY(LaunchAccountUtils::accountKindLabel(nullptr).isEmpty());
        QVERIFY(LaunchAccountUtils::accountMetaLine(nullptr).isEmpty());
        QCOMPARE(LaunchAccountUtils::accountStatusLabel(nullptr), QStringLiteral("No account"));

        auto offline = MinecraftAccount::createOffline(QStringLiteral("Steve"));
        QCOMPARE(LaunchAccountUtils::accountKindLabel(offline), QStringLiteral("Offline"));
        offline->accountData()->accountState = AccountState::Online;
        QCOMPARE(LaunchAccountUtils::accountStatusLabel(offline), QStringLiteral("Ready"));
        QCOMPARE(LaunchAccountUtils::accountMetaLine(offline), QStringLiteral("Offline · Ready"));

        auto altening = MinecraftAccount::createTheAlteningFromApiKey(QStringLiteral("key"));
        QCOMPARE(LaunchAccountUtils::accountKindLabel(altening), QStringLiteral("The Altening"));

        auto msa = MinecraftAccount::createBlankMSA();
        QCOMPARE(LaunchAccountUtils::accountKindLabel(msa), QStringLiteral("Microsoft"));
        msa->accountData()->accountState = AccountState::Expired;
        QCOMPARE(LaunchAccountUtils::accountStatusLabel(msa), QStringLiteral("Expired"));
    }
};

QTEST_GUILESS_MAIN(LaunchAccountTest)

#include "LaunchAccount_test.moc"
