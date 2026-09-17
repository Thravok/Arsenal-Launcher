#include <QJsonObject>
#include <QTest>

#include <minecraft/auth/AccountData.h>
#include <minecraft/auth/TheAlteningAuthParse.h>

class TheAlteningAuthTest : public QObject {
    Q_OBJECT
   private slots:
    void test_applyLicenseJson()
    {
        AccountData data;
        QString error;
        QVERIFY(!TheAlteningAuth::applyLicenseJson(&data, QJsonObject{}, &error));
        QVERIFY(error.contains(QStringLiteral("no active license"), Qt::CaseInsensitive));

        QVERIFY(!TheAlteningAuth::applyLicenseJson(&data, QJsonObject{ { "hasLicense", false } }, &error));

        QVERIFY(TheAlteningAuth::applyLicenseJson(
            &data, QJsonObject{ { "hasLicense", true }, { "licenseType", "premium" }, { "expires", "2026-12-01" } }, &error));
        QCOMPARE(data.theAlteningLicenseType, QStringLiteral("premium"));
        QCOMPARE(data.theAlteningLicenseExpires, QStringLiteral("2026-12-01"));
    }

    void test_applyGenerateJson()
    {
        AccountData data;
        auto missing = TheAlteningAuth::applyGenerateJson(&data, QJsonObject{ { "username", "Steve" } });
        QVERIFY(!missing.ok);
        QVERIFY(missing.error.contains(QStringLiteral("alt token"), Qt::CaseInsensitive));
        QVERIFY(data.yggdrasilToken.extra.value(QStringLiteral("userName")).toString().isEmpty());

        auto generated = TheAlteningAuth::applyGenerateJson(&data, QJsonObject{
                                                                       { "token", "alt-token" },
                                                                       { "username", "Steve" },
                                                                       { "skin", "skin-id" },
                                                                       { "limit", true },
                                                                   });
        QVERIFY(generated.ok);
        QVERIFY(generated.dailyLimit);
        QCOMPARE(data.yggdrasilToken.extra.value(QStringLiteral("userName")).toString(), QStringLiteral("alt-token"));
        QCOMPARE(data.theAlteningPendingPassword, QStringLiteral("anything"));
        QCOMPARE(data.theAlteningAltUsername, QStringLiteral("Steve"));
        QCOMPARE(data.yggdrasilToken.extra.value(QStringLiteral("alteningSkin")).toString(), QStringLiteral("skin-id"));

        auto withPassword = TheAlteningAuth::applyGenerateJson(&data, QJsonObject{
                                                                          { "token", "other-token" },
                                                                          { "password", "secret" },
                                                                      });
        QVERIFY(withPassword.ok);
        QVERIFY(!withPassword.dailyLimit);
        QCOMPARE(data.theAlteningPendingPassword, QStringLiteral("secret"));
        QCOMPARE(data.yggdrasilToken.extra.value(QStringLiteral("userName")).toString(), QStringLiteral("other-token"));
        QCOMPARE(data.theAlteningAltUsername, QStringLiteral("Steve"));
    }

    void test_applyAuthenticateJson()
    {
        AccountData data;
        QString error;
        QVERIFY(!TheAlteningAuth::applyAuthenticateJson(&data, QByteArrayLiteral("not json"), &error));
        QVERIFY(error.contains(QStringLiteral("invalid JSON"), Qt::CaseInsensitive));

        QVERIFY(!TheAlteningAuth::applyAuthenticateJson(&data, QByteArrayLiteral("{}"), &error));
        QVERIFY(error.contains(QStringLiteral("incomplete"), Qt::CaseInsensitive));

        QVERIFY(!TheAlteningAuth::applyAuthenticateJson(
            &data, QByteArrayLiteral(R"({"clientToken":"ct","accessToken":"at","selectedProfile":{"name":"Steve"}})"), &error));
        QVERIFY(error.contains(QStringLiteral("profile"), Qt::CaseInsensitive));

        QVERIFY(TheAlteningAuth::applyAuthenticateJson(
            &data,
            QByteArrayLiteral(
                R"({"clientToken":"ct","accessToken":"at","selectedProfile":{"id":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa","name":"Steve"}})"),
            &error));
        QCOMPARE(data.yggdrasilToken.extra.value(QStringLiteral("clientToken")).toString(), QStringLiteral("ct"));
        QCOMPARE(data.accessToken(), QStringLiteral("at"));
        QCOMPARE(data.yggdrasilToken.validity, Validity::Certain);
        QVERIFY(data.yggdrasilToken.issueInstant.isValid());
        QVERIFY(!data.yggdrasilToken.notAfter.isValid());
        QCOMPARE(data.profileId(), QStringLiteral("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
        QCOMPARE(data.minecraftProfile.name, QStringLiteral("Steve"));
        QVERIFY(data.minecraftEntitlement.canPlayMinecraft);
        QVERIFY(data.minecraftEntitlement.ownsMinecraft);
    }
};

QTEST_GUILESS_MAIN(TheAlteningAuthTest)
#include "TheAlteningAuth_test.moc"
