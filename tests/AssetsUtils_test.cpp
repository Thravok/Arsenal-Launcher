#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

#include <FileSystem.h>
#include <minecraft/AssetsUtils.h>

class AssetsUtilsTest : public QObject {
    Q_OBJECT

    static QString writeIndex(const QTemporaryDir& dir, const QString& name, const QByteArray& json)
    {
        const QString path = FS::PathCombine(dir.path(), name);
        FS::write(path, json);
        return path;
    }

   private slots:
    void test_loadValidIndexWithFlagsAndObjects()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QByteArray json = QByteArrayLiteral(
            "{"
            "\"virtual\":true,"
            "\"map_to_resources\":false,"
            "\"objects\":{"
            "\"icons/icon_16x16.png\":{\"hash\":\"bdf48ef6b5d0d23bbb02e17d04865216179f510a\",\"size\":3665},"
            "\"minecraft/sounds.json\":{\"hash\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\",\"size\":12}"
            "}"
            "}");

        AssetsIndex index;
        QVERIFY(AssetsUtils::loadAssetsIndexJson(QStringLiteral("1.20"), writeIndex(dir, QStringLiteral("1.20.json"), json), index));
        QCOMPARE(index.id, QStringLiteral("1.20"));
        QVERIFY(index.isVirtual);
        QVERIFY(!index.mapToResources);
        QCOMPARE(index.objects.size(), 2);

        const AssetObject icon = index.objects.value(QStringLiteral("icons/icon_16x16.png"));
        QCOMPARE(icon.hash, QStringLiteral("bdf48ef6b5d0d23bbb02e17d04865216179f510a"));
        QCOMPARE(icon.size, 3665);
        QCOMPARE(icon.getRelPath(), QStringLiteral("bd/bdf48ef6b5d0d23bbb02e17d04865216179f510a"));
        QCOMPARE(icon.getLocalPath(), QStringLiteral("assets/objects/bd/bdf48ef6b5d0d23bbb02e17d04865216179f510a"));
    }

    void test_loadMapToResourcesWithoutVirtual()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QByteArray json = QByteArrayLiteral("{\"map_to_resources\":true,\"objects\":{}}");

        AssetsIndex index;
        QVERIFY(AssetsUtils::loadAssetsIndexJson(QStringLiteral("legacy"), writeIndex(dir, QStringLiteral("legacy.json"), json), index));
        QVERIFY(!index.isVirtual);
        QVERIFY(index.mapToResources);
        QVERIFY(index.objects.isEmpty());
    }

    void test_missingFileFails()
    {
        AssetsIndex index;
        QVERIFY(!AssetsUtils::loadAssetsIndexJson(QStringLiteral("missing"), QStringLiteral("/no/such/assets-index.json"), index));
    }

    void test_invalidJsonFails()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        AssetsIndex index;
        QVERIFY(!AssetsUtils::loadAssetsIndexJson(QStringLiteral("bad"),
                                                  writeIndex(dir, QStringLiteral("bad.json"), QByteArrayLiteral("{not-json")), index));
    }

    void test_arrayRootFails()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        AssetsIndex index;
        QVERIFY(!AssetsUtils::loadAssetsIndexJson(QStringLiteral("arr"),
                                                  writeIndex(dir, QStringLiteral("arr.json"), QByteArrayLiteral("[]")), index));
    }

    void test_missingObjectsYieldsEmptyMap()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        AssetsIndex index;
        QVERIFY(AssetsUtils::loadAssetsIndexJson(QStringLiteral("empty"),
                                                 writeIndex(dir, QStringLiteral("empty.json"), QByteArrayLiteral("{}")), index));
        QVERIFY(index.objects.isEmpty());
        QVERIFY(!index.isVirtual);
        QVERIFY(!index.mapToResources);
    }

    void test_getRelPathUsesFirstTwoHashChars()
    {
        AssetObject object;
        object.hash = QStringLiteral("ab");
        QCOMPARE(object.getRelPath(), QStringLiteral("ab/ab"));

        object.hash.clear();
        QCOMPARE(object.getRelPath(), QStringLiteral("/"));
    }
};

QTEST_GUILESS_MAIN(AssetsUtilsTest)
#include "AssetsUtils_test.moc"
