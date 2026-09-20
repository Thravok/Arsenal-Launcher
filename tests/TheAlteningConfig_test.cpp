#include <QTest>

#include <minecraft/auth/TheAlteningConfig.h>

class TheAlteningConfigTest : public QObject {
    Q_OBJECT

   private slots:
    void test_errorMessageForStatus()
    {
        using TheAltening::errorMessageForStatus;

        QVERIFY(errorMessageForStatus(401).contains(QStringLiteral("Invalid or missing"), Qt::CaseInsensitive));
        QVERIFY(errorMessageForStatus(403).contains(QStringLiteral("Starter")));
        QVERIFY(errorMessageForStatus(404).contains(QStringLiteral("not found"), Qt::CaseInsensitive));
        QVERIFY(errorMessageForStatus(500).contains(QStringLiteral("internal server error"), Qt::CaseInsensitive));
        QCOMPARE(errorMessageForStatus(418), QStringLiteral("The Altening API request failed (HTTP 418)."));
        QCOMPARE(errorMessageForStatus(0), QStringLiteral("The Altening API request failed (HTTP 0)."));
    }

    void test_skinCdnUrls()
    {
        QCOMPARE(TheAltening::skinCdnBodyUrl(QStringLiteral("abc")), QStringLiteral("https://cdn.thealtening.com/skins/body/abc.png"));
        QCOMPARE(TheAltening::skinCdnHeadUrl(QStringLiteral("abc")), QStringLiteral("https://cdn.thealtening.com/skins/head/abc.png"));
        QCOMPARE(TheAltening::skinCdnUrl(QStringLiteral("abc")), TheAltening::skinCdnHeadUrl(QStringLiteral("abc")));
        QVERIFY(TheAltening::skinCdnHeadUrl(QString()).endsWith(QStringLiteral(".png")));
        QVERIFY(!TheAltening::AuthlibInjectorSentinel.contains(QLatin1Char(' ')));
        QCOMPARE(TheAltening::ApiKeySettingName, QStringLiteral("TheAlteningApiKey"));
    }
};

QTEST_GUILESS_MAIN(TheAlteningConfigTest)

#include "TheAlteningConfig_test.moc"
