#include <QTest>

#include <modplatform/ModIndex.h>
#include <modplatform/helpers/HashUtils.h>

class HashUtilsTest : public QObject {
    Q_OBJECT
   private slots:
    void test_algorithmRoundTrip_data()
    {
        QTest::addColumn<Hashing::Algorithm>("algorithm");
        QTest::addColumn<QString>("name");

        QTest::newRow("md4") << Hashing::Algorithm::Md4 << QStringLiteral("md4");
        QTest::newRow("md5") << Hashing::Algorithm::Md5 << QStringLiteral("md5");
        QTest::newRow("sha1") << Hashing::Algorithm::Sha1 << QStringLiteral("sha1");
        QTest::newRow("sha256") << Hashing::Algorithm::Sha256 << QStringLiteral("sha256");
        QTest::newRow("sha512") << Hashing::Algorithm::Sha512 << QStringLiteral("sha512");
        QTest::newRow("murmur2") << Hashing::Algorithm::Murmur2 << QStringLiteral("murmur2");
    }
    void test_algorithmRoundTrip()
    {
        QFETCH(Hashing::Algorithm, algorithm);
        QFETCH(QString, name);

        QCOMPARE(Hashing::algorithmToString(algorithm), name);
        QCOMPARE(Hashing::algorithmFromString(name), algorithm);
    }

    void test_unknownAlgorithm()
    {
        QCOMPARE(Hashing::algorithmFromString(QStringLiteral("not-a-hash")), Hashing::Algorithm::Unknown);
        QCOMPARE(Hashing::algorithmToString(Hashing::Algorithm::Unknown), QStringLiteral("unknown"));
        QVERIFY(Hashing::hash(QByteArrayLiteral("hello"), Hashing::Algorithm::Unknown).isEmpty());
    }

    void test_digestHashesMatchKnownVectors()
    {
        const QByteArray hello = QByteArrayLiteral("hello");
        QCOMPARE(Hashing::hash(hello, Hashing::Algorithm::Md5), QStringLiteral("5d41402abc4b2a76b9719d911017c592"));
        QCOMPARE(Hashing::hash(hello, Hashing::Algorithm::Sha1), QStringLiteral("aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d"));
        QCOMPARE(Hashing::hash(hello, Hashing::Algorithm::Sha256),
                 QStringLiteral("2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824"));
        QCOMPARE(Hashing::hash(hello, Hashing::Algorithm::Sha512),
                 QStringLiteral("9b71d224bd62f3785d96d46ad3ea3d73319bfbc2890caadae2dff72519673ca7"
                                "2323c3d99ba5c11d7c7acc6e14b8c5da0c4663475c2e5c3adef46f73bcdec043"));
    }

    void test_murmur2IgnoresCurseForgeWhitespace()
    {
        const auto plain = Hashing::hash(QByteArrayLiteral("hello"), Hashing::Algorithm::Murmur2);
        QVERIFY(!plain.isEmpty());
        QCOMPARE(Hashing::hash(QByteArrayLiteral("he llo"), Hashing::Algorithm::Murmur2), plain);
        QCOMPARE(Hashing::hash(QByteArrayLiteral("he\tllo"), Hashing::Algorithm::Murmur2), plain);
        QCOMPARE(Hashing::hash(QByteArrayLiteral("he\nllo"), Hashing::Algorithm::Murmur2), plain);
        QCOMPARE(Hashing::hash(QByteArrayLiteral("he\rllo"), Hashing::Algorithm::Murmur2), plain);
        QVERIFY(Hashing::hash(QByteArrayLiteral("world"), Hashing::Algorithm::Murmur2) != plain);
    }

    void test_createHasherPicksProviderAlgorithm()
    {
        auto flame = Hashing::createHasher(QStringLiteral("/tmp/mod.jar"), ModPlatform::ResourceProvider::FLAME);
        QVERIFY(flame);
        QCOMPARE(Hashing::algorithmToString(Hashing::Algorithm::Murmur2), QStringLiteral("murmur2"));

        auto modrinth = Hashing::createHasher(QStringLiteral("/tmp/mod.jar"), ModPlatform::ResourceProvider::MODRINTH);
        QVERIFY(modrinth);
        QCOMPARE(ModPlatform::ProviderCapabilities::hashType(ModPlatform::ResourceProvider::MODRINTH).first(), QStringLiteral("sha512"));
        QCOMPARE(ModPlatform::ProviderCapabilities::hashType(ModPlatform::ResourceProvider::FLAME).first(), QStringLiteral("sha1"));
    }
};

QTEST_GUILESS_MAIN(HashUtilsTest)
#include "HashUtils_test.moc"
