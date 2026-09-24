#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include <minecraft/auth/Parsers.h>

namespace {

QByteArray sessionPayload(const QString& id, const QString& name, const QByteArray& texturesJson)
{
    QJsonObject root;
    root.insert(QStringLiteral("id"), id);
    root.insert(QStringLiteral("name"), name);
    QJsonObject prop;
    prop.insert(QStringLiteral("name"), QStringLiteral("textures"));
    prop.insert(QStringLiteral("value"), QString::fromLatin1(texturesJson.toBase64()));
    root.insert(QStringLiteral("properties"), QJsonArray{ prop });
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

const QByteArray kMsProfile = QByteArrayLiteral(
    "{"
    "\"id\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\","
    "\"name\":\"Steve\","
    "\"skins\":["
    "{\"id\":\"old\",\"state\":\"INACTIVE\",\"url\":\"https://textures.minecraft.net/old\",\"variant\":\"CLASSIC\"},"
    "{\"id\":\"skin-id\",\"state\":\"ACTIVE\",\"url\":\"http://textures.minecraft.net/texture/abc\",\"variant\":\"SLIM\"}"
    "],"
    "\"capes\":["
    "{\"id\":\"cape-id\",\"state\":\"ACTIVE\",\"url\":\"http://textures.minecraft.net/texture/cape\",\"alias\":\"Minecon\"}"
    "]"
    "}");

const QByteArray kEmptyTextures = QByteArrayLiteral("{\"textures\":{}}");

const QByteArray kCustomTextures = QByteArrayLiteral(
    "{"
    "\"textures\":{"
    "\"SKIN\":{\"url\":\"http://textures.minecraft.net/texture/custom\",\"metadata\":{\"model\":\"slim\"}},"
    "\"CAPE\":{\"url\":\"http://textures.minecraft.net/texture/cape\"}"
    "}"
    "}");

const QByteArray kXTokenValid = QByteArrayLiteral(
    "{"
    "\"IssueInstant\":\"2020-12-07T19:52:08.4463796Z\","
    "\"NotAfter\":\"2020-12-21T19:52:08.4463796Z\","
    "\"Token\":\"xbox-token\","
    "\"DisplayClaims\":{\"xui\":[{\"uhs\":\"userhash\",\"extra\":\"ok\"}]}"
    "}");

const QByteArray kXTokenNoUhs = QByteArrayLiteral(
    "{"
    "\"IssueInstant\":\"2020-12-07T19:52:08.4463796Z\","
    "\"NotAfter\":\"2020-12-21T19:52:08.4463796Z\","
    "\"Token\":\"xbox-token\","
    "\"DisplayClaims\":{\"xui\":[{\"xid\":\"1\"}]}"
    "}");

const QByteArray kXTokenNonStringClaim = QByteArrayLiteral(
    "{"
    "\"IssueInstant\":\"2020-12-07T19:52:08.4463796Z\","
    "\"NotAfter\":\"2020-12-21T19:52:08.4463796Z\","
    "\"Token\":\"xbox-token\","
    "\"DisplayClaims\":{\"xui\":[{\"uhs\":123}]}"
    "}");

}  // namespace

class AuthParsersTest : public QObject {
    Q_OBJECT
   private slots:
    void test_parseHelpers()
    {
        QDateTime dt;
        QVERIFY(Parsers::getDateTime(QJsonValue(QStringLiteral("2020-12-07T19:52:08.4463796Z")), dt));
        QVERIFY(dt.isValid());
        QVERIFY(!Parsers::getDateTime(QJsonValue(12), dt));
        QVERIFY(!Parsers::getDateTime(QJsonValue(QStringLiteral("not-a-date")), dt));

        QString s;
        QVERIFY(Parsers::getString(QJsonValue(QStringLiteral("token")), s));
        QCOMPARE(s, QStringLiteral("token"));
        QVERIFY(!Parsers::getString(QJsonValue(1), s));

        double d = 0;
        QVERIFY(Parsers::getNumber(QJsonValue(3.5), d));
        QCOMPARE(d, 3.5);
        QVERIFY(!Parsers::getNumber(QJsonValue(QStringLiteral("3")), d));

        int64_t n = 0;
        QVERIFY(Parsers::getNumber(QJsonValue(42), n));
        QCOMPARE(n, static_cast<int64_t>(42));

        bool b = false;
        QVERIFY(Parsers::getBool(QJsonValue(true), b));
        QVERIFY(b);
        QVERIFY(!Parsers::getBool(QJsonValue(QStringLiteral("true")), b));
    }

    void test_parseMinecraftProfileActiveSkinAndCape()
    {
        QByteArray json = kMsProfile;
        MinecraftProfile profile;
        QVERIFY(Parsers::parseMinecraftProfile(json, profile));
        QCOMPARE(profile.id, QStringLiteral("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
        QCOMPARE(profile.name, QStringLiteral("Steve"));
        QCOMPARE(profile.skin.id, QStringLiteral("skin-id"));
        QCOMPARE(profile.skin.variant, QStringLiteral("SLIM"));
        QCOMPARE(profile.skin.url, QStringLiteral("https://textures.minecraft.net/texture/abc"));
        QCOMPARE(profile.currentCape, QStringLiteral("cape-id"));
        QCOMPARE(profile.capes.value(QStringLiteral("cape-id")).url, QStringLiteral("https://textures.minecraft.net/texture/cape"));
        QCOMPARE(profile.validity, Validity::Certain);
    }

    void test_parseMinecraftProfileRejectsInvalid()
    {
        MinecraftProfile profile;
        QByteArray missingName = QByteArrayLiteral("{\"id\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\"}");
        QVERIFY(!Parsers::parseMinecraftProfile(missingName, profile));
        QByteArray notJson = QByteArrayLiteral("not json");
        QVERIFY(!Parsers::parseMinecraftProfile(notJson, profile));
    }

    void test_parseMinecraftProfileMojangDefaultSteveAndAlex()
    {
        MinecraftProfile steve;
        auto steveJson = sessionPayload(QStringLiteral("00000000000000000000000000000000"), QStringLiteral("Steve"), kEmptyTextures);
        QVERIFY(Parsers::parseMinecraftProfileMojang(steveJson, steve));
        QCOMPARE(steve.skin.variant, QStringLiteral("CLASSIC"));
        QCOMPARE(steve.skin.url,
                 QStringLiteral("https://textures.minecraft.net/texture/1a4af718455d4aab528e7a61f86fa25e6a369d1768dcb13f7df319a713eb810b"));

        MinecraftProfile alex;
        auto alexJson = sessionPayload(QStringLiteral("00000000000000000000000000000001"), QStringLiteral("Alex"), kEmptyTextures);
        QVERIFY(Parsers::parseMinecraftProfileMojang(alexJson, alex));
        QCOMPARE(alex.skin.variant, QStringLiteral("SLIM"));
        QCOMPARE(alex.skin.url,
                 QStringLiteral("https://textures.minecraft.net/texture/83cee5ca6afcdb171285aa00e8049c297b2dbeba0efb8ff970a5677a1b644032"));
    }

    void test_parseMinecraftProfileMojangCustomTextures()
    {
        MinecraftProfile profile;
        auto json = sessionPayload(QStringLiteral("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), QStringLiteral("Player"), kCustomTextures);
        QVERIFY(Parsers::parseMinecraftProfileMojang(json, profile));
        QCOMPARE(profile.name, QStringLiteral("Player"));
        QCOMPARE(profile.skin.variant, QStringLiteral("slim"));
        QCOMPARE(profile.skin.url, QStringLiteral("https://textures.minecraft.net/texture/custom"));
        QCOMPARE(profile.currentCape, QStringLiteral("cape"));
        QCOMPARE(profile.capes.value(QStringLiteral("cape")).url, QStringLiteral("https://textures.minecraft.net/texture/cape"));
        QCOMPARE(profile.validity, Validity::Certain);
    }

    void test_parseMinecraftProfileMojangRejectsMissingTextures()
    {
        MinecraftProfile profile;
        QByteArray noProps = QByteArrayLiteral("{\"id\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\",\"name\":\"Player\"}");
        QVERIFY(!Parsers::parseMinecraftProfileMojang(noProps, profile));

        QByteArray notJson = QByteArrayLiteral("{");
        QVERIFY(!Parsers::parseMinecraftProfileMojang(notJson, profile));
    }

    void test_parseXTokenResponse()
    {
        QByteArray valid = kXTokenValid;
        Token token;
        QVERIFY(Parsers::parseXTokenResponse(valid, token, QStringLiteral("user")));
        QCOMPARE(token.token, QStringLiteral("xbox-token"));
        QCOMPARE(token.extra.value(QStringLiteral("uhs")).toString(), QStringLiteral("userhash"));
        QCOMPARE(token.validity, Validity::Certain);

        Token missingUhs;
        QByteArray noUhs = kXTokenNoUhs;
        QVERIFY(!Parsers::parseXTokenResponse(noUhs, missingUhs, QStringLiteral("user")));

        Token badClaim;
        QByteArray nonStringClaim = kXTokenNonStringClaim;
        QVERIFY(!Parsers::parseXTokenResponse(nonStringClaim, badClaim, QStringLiteral("user")));
    }

    void test_parseMojangResponse()
    {
        QByteArray valid = QByteArrayLiteral("{\"expires_in\":120,\"username\":\"player\",\"access_token\":\"mc-token\"}");
        Token token;
        const auto before = QDateTime::currentDateTimeUtc();
        QVERIFY(Parsers::parseMojangResponse(valid, token));
        const auto after = QDateTime::currentDateTimeUtc();
        QCOMPARE(token.token, QStringLiteral("mc-token"));
        QCOMPARE(token.validity, Validity::Certain);
        QVERIFY(token.notAfter >= before.addSecs(119));
        QVERIFY(token.notAfter <= after.addSecs(121));

        Token missing;
        QByteArray noToken = QByteArrayLiteral("{\"expires_in\":120,\"username\":\"player\"}");
        QVERIFY(!Parsers::parseMojangResponse(noToken, missing));
    }

    void test_parseMinecraftEntitlements()
    {
        MinecraftEntitlement entitlement;
        QByteArray both = QByteArrayLiteral("{\"items\":[{\"name\":\"game_minecraft\"},{\"name\":\"product_minecraft\"}]}");
        QVERIFY(Parsers::parseMinecraftEntitlements(both, entitlement));
        QVERIFY(entitlement.canPlayMinecraft);
        QVERIFY(entitlement.ownsMinecraft);
        QCOMPARE(entitlement.validity, Validity::Certain);

        MinecraftEntitlement empty;
        QByteArray none = QByteArrayLiteral("{\"items\":[]}");
        QVERIFY(Parsers::parseMinecraftEntitlements(none, empty));
        QVERIFY(!empty.canPlayMinecraft);
        QVERIFY(!empty.ownsMinecraft);

        MinecraftEntitlement invalid;
        QByteArray notJson = QByteArrayLiteral("not json");
        QVERIFY(!Parsers::parseMinecraftEntitlements(notJson, invalid));
    }

    void test_parseRolloutResponse()
    {
        bool result = false;
        QByteArray enabled = QByteArrayLiteral("{\"feature\":\"msamigration\",\"rollout\":true}");
        QVERIFY(Parsers::parseRolloutResponse(enabled, result));
        QVERIFY(result);

        QByteArray disabled = QByteArrayLiteral("{\"feature\":\"msamigration\",\"rollout\":false}");
        QVERIFY(Parsers::parseRolloutResponse(disabled, result));
        QVERIFY(!result);

        QByteArray wrongFeature = QByteArrayLiteral("{\"feature\":\"other\",\"rollout\":true}");
        QVERIFY(!Parsers::parseRolloutResponse(wrongFeature, result));
    }
};

QTEST_GUILESS_MAIN(AuthParsersTest)
#include "AuthParsers_test.moc"
