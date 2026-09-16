#include <QJsonObject>
#include <QTest>

#include <minecraft/auth/AccountData.h>

class AccountDataTest : public QObject {
    Q_OBJECT
   private slots:
    void test_theAlteningRoundTrip()
    {
        AccountData original;
        original.type = AccountType::TheAltening;
        original.theAlteningApiKey = QStringLiteral("test-api-key");
        original.theAlteningLicenseType = QStringLiteral("premium");
        original.theAlteningLicenseExpires = QStringLiteral("2026-12-01");
        original.theAlteningAltUsername = QStringLiteral("Steve");
        original.yggdrasilToken.token = QStringLiteral("ygg-token");
        original.yggdrasilToken.persistent = true;
        original.minecraftProfile.id = QStringLiteral("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
        original.minecraftProfile.name = QStringLiteral("Steve");
        original.minecraftProfile.skin.id = QStringLiteral("skin-id");
        original.minecraftProfile.skin.url = QStringLiteral("https://cdn.thealtening.com/skins/head/skin-id.png");
        original.minecraftProfile.skin.variant = QStringLiteral("classic");
        original.minecraftEntitlement.ownsMinecraft = true;
        original.minecraftEntitlement.canPlayMinecraft = true;
        original.minecraftEntitlement.validity = Validity::Assumed;

        const QJsonObject saved = original.saveState();
        QCOMPARE(saved.value("type").toString(), QStringLiteral("TheAltening"));
        QCOMPARE(saved.value("alteningApiKey").toString(), QStringLiteral("test-api-key"));
        QVERIFY(!saved.contains("msa"));
        QVERIFY(!saved.contains("msa-client-id"));

        AccountData restored;
        QVERIFY(restored.resumeStateFromV3(saved));
        QCOMPARE(restored.type, AccountType::TheAltening);
        QCOMPARE(restored.theAlteningApiKey, original.theAlteningApiKey);
        QCOMPARE(restored.theAlteningLicenseType, original.theAlteningLicenseType);
        QCOMPARE(restored.theAlteningLicenseExpires, original.theAlteningLicenseExpires);
        QCOMPARE(restored.theAlteningAltUsername, original.theAlteningAltUsername);
        QCOMPARE(restored.accessToken(), QStringLiteral("ygg-token"));
        QCOMPARE(restored.profileName(), QStringLiteral("Steve"));
        QCOMPARE(restored.profileId(), original.minecraftProfile.id);
    }

    void test_profileNameStripsAlteningMasks()
    {
        AccountData data;
        data.type = AccountType::TheAltening;
        data.theAlteningAltUsername = QStringLiteral("St***ve");
        QCOMPARE(data.profileName(), QStringLiteral("Stve"));

        data.minecraftProfile.name = QStringLiteral("Ste**ve");
        QCOMPARE(data.profileName(), QStringLiteral("Steve"));

        data.minecraftProfile.name = QStringLiteral("Alex");
        QCOMPARE(data.profileName(), QStringLiteral("Alex"));
    }

    void test_resumeRejectsUnknownType()
    {
        AccountData data;
        QVERIFY(!data.resumeStateFromV3(QJsonObject{}));
        QVERIFY(!data.resumeStateFromV3(QJsonObject{ { "type", "NotARealAccount" } }));
    }

    void test_offlineAndMsaRoundTrip()
    {
        AccountData offline;
        offline.type = AccountType::Offline;
        offline.minecraftProfile.id = QStringLiteral("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
        offline.minecraftProfile.name = QStringLiteral("Player");
        offline.minecraftProfile.skin.id = QStringLiteral("id");
        offline.minecraftProfile.skin.url = QStringLiteral("https://textures.minecraft.net/skin");
        offline.minecraftProfile.skin.variant = QStringLiteral("classic");
        AccountData offlineRestored;
        QVERIFY(offlineRestored.resumeStateFromV3(offline.saveState()));
        QCOMPARE(offlineRestored.type, AccountType::Offline);
        QCOMPARE(offlineRestored.profileName(), QStringLiteral("Player"));

        AccountData msa;
        msa.type = AccountType::MSA;
        msa.msaClientID = QStringLiteral("client-id");
        msa.msaToken.token = QStringLiteral("msa-token");
        msa.msaToken.persistent = true;
        AccountData msaRestored;
        QVERIFY(msaRestored.resumeStateFromV3(msa.saveState()));
        QCOMPARE(msaRestored.type, AccountType::MSA);
        QCOMPARE(msaRestored.msaClientID, QStringLiteral("client-id"));
        QCOMPARE(msaRestored.msaToken.token, QStringLiteral("msa-token"));
    }
};

QTEST_GUILESS_MAIN(AccountDataTest)
#include "AccountData_test.moc"
