#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include <minecraft/auth/AccountData.h>
#include <minecraft/auth/TheAlteningConfig.h>
#include <minecraft/auth/steps/TheAlteningProfileStep.h>

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

const QByteArray kEmptyTextures = QByteArrayLiteral("{\"textures\":{}}");

}  // namespace

class AlteningProfileApplyTest : public QObject {
    Q_OBJECT
   private slots:
    void test_unmaskUsername()
    {
        QCOMPARE(TheAltening::unmaskUsername(QStringLiteral("Steve")), QStringLiteral("Steve"));
        QCOMPARE(TheAltening::unmaskUsername(QStringLiteral("BeanEater6942**")), QStringLiteral("BeanEater6942"));
        QCOMPARE(TheAltening::unmaskUsername(QStringLiteral("***")), QString());
        QVERIFY(TheAltening::isUsableMinecraftUsername(QStringLiteral("Steve")));
        QVERIFY(!TheAltening::isUsableMinecraftUsername(QStringLiteral("Steve**")));
        QVERIFY(!TheAltening::isUsableMinecraftUsername(QString()));
    }

    void test_prefersUnmaskedAuthNameAndKeepsUuid()
    {
        AccountData data;
        data.minecraftProfile.id = QStringLiteral("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
        data.minecraftProfile.name = QStringLiteral("CleanName");

        auto json = sessionPayload(QStringLiteral("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"), QStringLiteral("Masked**"), kEmptyTextures);
        QVERIFY(TheAlteningProfileStep::applyFetchedProfile(&data, json));
        QCOMPARE(data.minecraftProfile.id, QStringLiteral("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
        QCOMPARE(data.minecraftProfile.name, QStringLiteral("CleanName"));
        QVERIFY(!data.minecraftProfile.skin.url.isEmpty());
    }

    void test_stripsMaskWhenAuthNameIsMasked()
    {
        AccountData data;
        data.minecraftProfile.id = QStringLiteral("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
        data.minecraftProfile.name = QStringLiteral("Bean**");

        auto json = sessionPayload(QStringLiteral("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"), QStringLiteral("BeanEater6942**"), kEmptyTextures);
        QVERIFY(TheAlteningProfileStep::applyFetchedProfile(&data, json));
        QCOMPARE(data.minecraftProfile.id, QStringLiteral("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
        QCOMPARE(data.minecraftProfile.name, QStringLiteral("BeanEater6942"));
    }

    void test_invalidJsonRestoresIdentityAndUnmasks()
    {
        AccountData data;
        data.minecraftProfile.id = QStringLiteral("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
        data.minecraftProfile.name = QStringLiteral("Alt**");

        QVERIFY(!TheAlteningProfileStep::applyFetchedProfile(&data, QByteArrayLiteral("not json")));
        QCOMPARE(data.minecraftProfile.id, QStringLiteral("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
        QCOMPARE(data.minecraftProfile.name, QStringLiteral("Alt"));
    }

    void test_nullDataIsRejected() { QVERIFY(!TheAlteningProfileStep::applyFetchedProfile(nullptr, QByteArrayLiteral("{}"))); }
};

QTEST_GUILESS_MAIN(AlteningProfileApplyTest)
#include "AlteningProfileApply_test.moc"
