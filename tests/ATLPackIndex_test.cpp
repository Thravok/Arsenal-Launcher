#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include <Json.h>
#include <modplatform/atlauncher/ATLPackIndex.h>

class ATLPackIndexTest : public QObject {
    Q_OBJECT
   private:
    static QJsonObject packObject(const QString& name, const QString& type)
    {
        QJsonObject obj;
        obj.insert(QStringLiteral("id"), 42);
        obj.insert(QStringLiteral("position"), 7);
        obj.insert(QStringLiteral("name"), name);
        obj.insert(QStringLiteral("type"), type);
        QJsonArray versions;
        QJsonObject version;
        version.insert(QStringLiteral("version"), QStringLiteral("1.2.3"));
        version.insert(QStringLiteral("minecraft"), QStringLiteral("1.20.1"));
        versions.append(version);
        obj.insert(QStringLiteral("versions"), versions);
        obj.insert(QStringLiteral("description"), QStringLiteral("A pack"));
        obj.insert(QStringLiteral("system"), false);
        return obj;
    }

   private slots:
    void test_loadPublicPackAndSafeName()
    {
        auto obj = packObject(QStringLiteral("All the Mods 9!"), QStringLiteral("public"));
        ATLauncher::IndexedPack pack;
        ATLauncher::loadIndexedPack(pack, obj);

        QCOMPARE(pack.id, 42);
        QCOMPARE(pack.position, 7);
        QCOMPARE(pack.name, QStringLiteral("All the Mods 9!"));
        QCOMPARE(pack.type, ATLauncher::PackType::Public);
        QCOMPARE(pack.versions.size(), 1);
        QCOMPARE(pack.versions.first().version, QStringLiteral("1.2.3"));
        QCOMPARE(pack.versions.first().minecraft, QStringLiteral("1.20.1"));
        QCOMPARE(pack.system, false);
        QCOMPARE(pack.description, QStringLiteral("A pack"));
        QCOMPARE(pack.safeName, QStringLiteral("allthemods9.png"));
    }

    void test_privateTypeAndAlphanumericSafeName()
    {
        auto obj = packObject(QStringLiteral("RLCraft"), QStringLiteral("private"));
        ATLauncher::IndexedPack pack;
        ATLauncher::loadIndexedPack(pack, obj);

        QCOMPARE(pack.type, ATLauncher::PackType::Private);
        QCOMPARE(pack.safeName, QStringLiteral("rlcraft.png"));
    }

    void test_safeNameStripsSymbolsAndCase()
    {
        auto obj = packObject(QStringLiteral("Foo-Bar_Baz 1.0"), QStringLiteral("public"));
        ATLauncher::IndexedPack pack;
        ATLauncher::loadIndexedPack(pack, obj);
        QCOMPARE(pack.safeName, QStringLiteral("foobarbaz10.png"));
    }

    void test_unknownTypeIsPublic()
    {
        auto obj = packObject(QStringLiteral("Vanilla"), QStringLiteral("third-party"));
        ATLauncher::IndexedPack pack;
        ATLauncher::loadIndexedPack(pack, obj);
        QCOMPARE(pack.type, ATLauncher::PackType::Public);
    }

    void test_missingRequiredFieldThrows()
    {
        auto obj = packObject(QStringLiteral("Broken"), QStringLiteral("public"));
        obj.remove(QStringLiteral("versions"));
        ATLauncher::IndexedPack pack;
        bool threw = false;
        try {
            ATLauncher::loadIndexedPack(pack, obj);
        } catch (const Json::JsonException&) {
            threw = true;
        }
        QVERIFY(threw);
    }
};

QTEST_GUILESS_MAIN(ATLPackIndexTest)
#include "ATLPackIndex_test.moc"
