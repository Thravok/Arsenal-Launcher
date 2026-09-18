#include <QTest>

#include "minecraft/auth/AuthSession.h"
#include "minecraft/auth/MinecraftAccount.h"
#include "minecraft/auth/TheAlteningConfig.h"

class AlteningLaunchTest : public QObject {
    Q_OBJECT
   private slots:
    void createTheAlteningFromApiKey_trimsAndEntitles()
    {
        auto account = MinecraftAccount::createTheAlteningFromApiKey(QStringLiteral("  api-key  "));
        QCOMPARE(account->accountType(), AccountType::TheAltening);
        QCOMPARE(account->accountData()->theAlteningApiKey, QStringLiteral("api-key"));
        QVERIFY(account->ownsMinecraft());
        QCOMPARE(account->typeString(), QStringLiteral("mojang"));
        QVERIFY(!account->hasProfile());
    }

    void fillSession_setsAuthlibInjectorSentinel()
    {
        auto account = MinecraftAccount::createTheAlteningFromApiKey(QStringLiteral("api-key"));
        account->accountData()->yggdrasilToken.token = QStringLiteral("access-token");
        account->accountData()->minecraftProfile.id = QStringLiteral("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
        account->accountData()->minecraftProfile.name = QStringLiteral("Steve");

        auto session = std::make_shared<AuthSession>();
        account->fillSession(session);

        QCOMPARE(session->access_token, QStringLiteral("access-token"));
        QCOMPARE(session->player_name, QStringLiteral("Steve"));
        QCOMPARE(session->uuid, QStringLiteral("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
        QCOMPARE(session->user_type, QStringLiteral("mojang"));
        QCOMPARE(session->authlib_injector_base_url, TheAltening::AuthlibInjectorSentinel);
        QCOMPARE(session->session, QStringLiteral("token:access-token:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
    }

    void fillSession_offlineDoesNotSetInjectorSentinel()
    {
        auto account = MinecraftAccount::createOffline(QStringLiteral("Player"));
        auto session = std::make_shared<AuthSession>();
        account->fillSession(session);

        QCOMPARE(session->user_type, QStringLiteral("offline"));
        QVERIFY(session->authlib_injector_base_url.isEmpty());
        QCOMPARE(session->player_name, QStringLiteral("Player"));
        QVERIFY(!session->uuid.isEmpty());
        QCOMPARE(session->session, QStringLiteral("token:0:") + session->uuid);
    }

    void fillSession_missingUuidFallsBackToOfflinePlayerUuid()
    {
        auto account = MinecraftAccount::createTheAlteningFromApiKey(QStringLiteral("api-key"));
        account->accountData()->minecraftProfile.name = QStringLiteral("Steve");
        account->accountData()->yggdrasilToken.token = QStringLiteral("tok");

        auto session = std::make_shared<AuthSession>();
        account->fillSession(session);

        QCOMPARE(session->uuid, MinecraftAccount::uuidFromUsername(QStringLiteral("Steve")).toString(QUuid::Id128));
        QCOMPARE(session->session, QStringLiteral("token:tok:") + session->uuid);
    }

    void fillSession_emptyAccessTokenUsesDashSession()
    {
        auto account = MinecraftAccount::createTheAlteningFromApiKey(QStringLiteral("api-key"));
        auto session = std::make_shared<AuthSession>();
        account->fillSession(session);
        QCOMPARE(session->session, QStringLiteral("-"));
    }

    void displayName_warnsWhenExpired()
    {
        auto account = MinecraftAccount::createTheAlteningFromApiKey(QStringLiteral("api-key"));
        account->accountData()->minecraftProfile.name = QStringLiteral("Steve");
        QCOMPARE(account->displayName(), QStringLiteral("Steve"));

        account->accountData()->accountState = AccountState::Expired;
        QCOMPARE(account->displayName(), QStringLiteral("⚠ Steve"));
    }

    void uuidFromUsername_isDeterministicVersion3()
    {
        const auto first = MinecraftAccount::uuidFromUsername(QStringLiteral("Player"));
        const auto second = MinecraftAccount::uuidFromUsername(QStringLiteral("Player"));
        QCOMPARE(first, second);
        QVERIFY(first != MinecraftAccount::uuidFromUsername(QStringLiteral("Other")));
        QCOMPARE(first.version(), 3);
    }

    void resolveAlteningSkinDownloadUrl_data()
    {
        QTest::addColumn<QString>("currentUrl");
        QTest::addColumn<QString>("skinId");
        QTest::addColumn<QString>("expected");

        const QString mojang = QStringLiteral("https://textures.minecraft.net/texture/abc123");
        const QString mhfSteve =
            QStringLiteral("http://textures.minecraft.net/texture/1a4af718455d4aab528e7a61f86fa25e6a369d1768dcb13f7df319a713eb810b");
        const QString mhfAlex =
            QStringLiteral("http://textures.minecraft.net/texture/83cee5ca6afcdb171285aa00e8049c297b2dbeba0efb8ff970a5677a1b644032");
        const QString body = QStringLiteral("https://cdn.thealtening.com/skins/body/skin-id.png");
        const QString head = TheAltening::skinCdnHeadUrl(QStringLiteral("skin-id"));

        QTest::newRow("real mojang texture kept") << mojang << "skin-id" << mojang;
        QTest::newRow("mhf steve replaced with head cdn") << mhfSteve << "skin-id" << head;
        QTest::newRow("mhf alex replaced with head cdn") << mhfAlex << "skin-id" << head;
        QTest::newRow("body cdn replaced with head cdn") << body << "skin-id" << head;
        QTest::newRow("empty url uses head cdn") << QString() << "skin-id" << head;
        QTest::newRow("mhf steve without skin id is cleared") << mhfSteve << "" << QString();
        QTest::newRow("body cdn without skin id is cleared") << body << "" << QString();
        QTest::newRow("unrelated url without skin id kept")
            << QStringLiteral("https://example.test/skin.png") << "" << QStringLiteral("https://example.test/skin.png");
        QTest::newRow("empty without skin id stays empty") << QString() << "" << QString();
    }
    void resolveAlteningSkinDownloadUrl()
    {
        QFETCH(QString, currentUrl);
        QFETCH(QString, skinId);
        QFETCH(QString, expected);

        QCOMPARE(TheAltening::resolveAlteningSkinDownloadUrl(currentUrl, skinId), expected);
    }
};

QTEST_GUILESS_MAIN(AlteningLaunchTest)
#include "AlteningLaunch_test.moc"
