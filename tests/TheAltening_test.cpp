#include <QTest>

#include <minecraft/auth/MinecraftAccount.h>
#include <minecraft/auth/TheAlteningApi.h>
#include <minecraft/auth/TheAlteningConfig.h>
#include <ui/LaunchAccountUtils.h>

class TheAlteningTest : public QObject {
    Q_OBJECT
   private slots:
    void test_errorMessageForStatus_data()
    {
        QTest::addColumn<int>("status");
        QTest::addColumn<QString>("needle");

        QTest::newRow("401") << 401 << QStringLiteral("Invalid or missing");
        QTest::newRow("403") << 403 << QStringLiteral("Starter");
        QTest::newRow("404") << 404 << QStringLiteral("not found");
        QTest::newRow("500") << 500 << QStringLiteral("internal server error");
        QTest::newRow("502") << 502 << QStringLiteral("HTTP 502");
    }
    void test_errorMessageForStatus()
    {
        QFETCH(int, status);
        QFETCH(QString, needle);

        const QString message = TheAlteningApi::errorMessageForStatus(status);
        QVERIFY2(message.contains(needle, Qt::CaseInsensitive), qPrintable(message));
        QVERIFY(!message.contains("api-key", Qt::CaseInsensitive));
        QVERIFY(!message.contains("token", Qt::CaseInsensitive));
    }

    void test_skinCdnUrls()
    {
        QCOMPARE(TheAltening::skinCdnHeadUrl("abc"), QStringLiteral("https://cdn.thealtening.com/skins/head/abc.png"));
        QCOMPARE(TheAltening::skinCdnBodyUrl("abc"), QStringLiteral("https://cdn.thealtening.com/skins/body/abc.png"));
        QCOMPARE(TheAltening::skinCdnUrl("abc"), TheAltening::skinCdnHeadUrl("abc"));
        QVERIFY(TheAltening::AuthlibInjectorSentinel.startsWith(QStringLiteral("thealtening://")));
    }

    void test_launchAccountLabelsAndBlocks()
    {
        auto altening = MinecraftAccount::createTheAlteningFromApiKey(QStringLiteral("test-api-key"));
        QCOMPARE(LaunchAccountUtils::accountKindLabel(altening), QStringLiteral("The Altening"));
        QCOMPARE(LaunchAccountUtils::accountStatusLabel(altening), QStringLiteral("Unchecked"));
        QCOMPARE(LaunchAccountUtils::accountMetaLine(altening), QStringLiteral("The Altening · Unchecked"));
        QVERIFY(!LaunchAccountUtils::blocksLaunch(altening));

        altening->accountData()->accountState = AccountState::Expired;
        QVERIFY(LaunchAccountUtils::blocksLaunch(altening));
        QVERIFY(LaunchAccountUtils::launchBlockReason(altening).contains(QStringLiteral("expired"), Qt::CaseInsensitive));

        altening->accountData()->accountState = AccountState::Disabled;
        QVERIFY(LaunchAccountUtils::blocksLaunch(altening));
        altening->accountData()->accountState = AccountState::Gone;
        QVERIFY(LaunchAccountUtils::blocksLaunch(altening));
        altening->accountData()->accountState = AccountState::Online;
        QVERIFY(!LaunchAccountUtils::blocksLaunch(altening));

        auto offline = MinecraftAccount::createOffline(QStringLiteral("Player"));
        QCOMPARE(LaunchAccountUtils::accountKindLabel(offline), QStringLiteral("Offline"));
        QVERIFY(!LaunchAccountUtils::blocksLaunch(offline));

        QVERIFY(LaunchAccountUtils::blocksLaunch(nullptr));
        QVERIFY(LaunchAccountUtils::launchBlockReason(nullptr).contains(QStringLiteral("Choose an account")));
        QVERIFY(LaunchAccountUtils::accountKindLabel(nullptr).isEmpty());
    }
};

QTEST_GUILESS_MAIN(TheAlteningTest)
#include "TheAltening_test.moc"
