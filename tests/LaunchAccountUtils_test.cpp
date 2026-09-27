#include <QTest>

#include <minecraft/auth/MinecraftAccount.h>
#include <ui/LaunchAccountUtils.h>

class LaunchAccountUtilsTest : public QObject {
    Q_OBJECT
   private slots:
    void resolveAccountForLaunch_usesDefaultWhenOverrideDisabled()
    {
        auto offline = MinecraftAccount::createOffline("Local");
        auto altening = MinecraftAccount::createTheAlteningFromApiKey("dummy-key");

        QVERIFY(LaunchAccountUtils::resolveAccountForLaunch(false, altening, offline) == offline);
    }

    void resolveAccountForLaunch_usesPinnedAccountWhenOverrideEnabled()
    {
        auto offline = MinecraftAccount::createOffline("Local");
        auto altening = MinecraftAccount::createTheAlteningFromApiKey("dummy-key");

        QVERIFY(LaunchAccountUtils::resolveAccountForLaunch(true, altening, offline) == altening);
    }

    void resolveAccountForLaunch_doesNotFallBackWhenPinnedAccountIsMissing()
    {
        auto offline = MinecraftAccount::createOffline("Local");

        QVERIFY(LaunchAccountUtils::resolveAccountForLaunch(true, {}, offline).isNull());
    }

    void launchModeForOfflineAccount_isOfflineWhenSomeoneOwnsMinecraft()
    {
        QCOMPARE(LaunchAccountUtils::launchModeForOfflineAccount(true), LaunchMode::Offline);
    }

    void launchModeForOfflineAccount_isDemoWhenNoOneOwnsMinecraft()
    {
        QCOMPARE(LaunchAccountUtils::launchModeForOfflineAccount(false), LaunchMode::Demo);
    }
};

QTEST_GUILESS_MAIN(LaunchAccountUtilsTest)

#include "LaunchAccountUtils_test.moc"
