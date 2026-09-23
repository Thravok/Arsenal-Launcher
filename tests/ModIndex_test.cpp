#include <QTest>

#include <modplatform/ModIndex.h>

class ModIndexTest : public QObject {
    Q_OBJECT
   private slots:
    void test_loaderStringRoundTrip_data()
    {
        QTest::addColumn<ModPlatform::ModLoaderType>("type");
        QTest::addColumn<QString>("name");

        QTest::newRow("neoforge") << ModPlatform::NeoForge << QStringLiteral("neoforge");
        QTest::newRow("forge") << ModPlatform::Forge << QStringLiteral("forge");
        QTest::newRow("cauldron") << ModPlatform::Cauldron << QStringLiteral("cauldron");
        QTest::newRow("liteloader") << ModPlatform::LiteLoader << QStringLiteral("liteloader");
        QTest::newRow("fabric") << ModPlatform::Fabric << QStringLiteral("fabric");
        QTest::newRow("quilt") << ModPlatform::Quilt << QStringLiteral("quilt");
        QTest::newRow("babric") << ModPlatform::Babric << QStringLiteral("babric");
        QTest::newRow("bta-babric") << ModPlatform::BTA << QStringLiteral("bta-babric");
        QTest::newRow("legacy-fabric") << ModPlatform::LegacyFabric << QStringLiteral("legacy-fabric");
        QTest::newRow("ornithe") << ModPlatform::Ornithe << QStringLiteral("ornithe");
        QTest::newRow("rift") << ModPlatform::Rift << QStringLiteral("rift");
    }
    void test_loaderStringRoundTrip()
    {
        QFETCH(ModPlatform::ModLoaderType, type);
        QFETCH(QString, name);

        QCOMPARE(ModPlatform::getModLoaderAsString(type), name);
        QCOMPARE(ModPlatform::getModLoaderFromString(name), type);
    }

    void test_loaderStringEdgeCases()
    {
        QCOMPARE(ModPlatform::getModLoaderAsString(ModPlatform::DataPack), QStringLiteral("datapack"));
        QCOMPARE(ModPlatform::getModLoaderFromString(QStringLiteral("datapack")), ModPlatform::None);
        QCOMPARE(ModPlatform::getModLoaderFromString(QStringLiteral("Fabric")), ModPlatform::None);
        QCOMPARE(ModPlatform::getModLoaderFromString(QStringLiteral("unknown-loader")), ModPlatform::None);
        QCOMPARE(ModPlatform::getModLoaderFromString(QString()), ModPlatform::None);
        QCOMPARE(ModPlatform::getModLoaderAsString(ModPlatform::None), QString());
    }

    void test_modLoaderTypesToList()
    {
        QCOMPARE(ModPlatform::modLoaderTypesToList({}), QList<ModPlatform::ModLoaderType>{});
        QCOMPARE(ModPlatform::modLoaderTypesToList(ModPlatform::Fabric), QList<ModPlatform::ModLoaderType>{ ModPlatform::Fabric });

        const auto combined = ModPlatform::modLoaderTypesToList(ModPlatform::NeoForge | ModPlatform::Fabric | ModPlatform::Quilt);
        QCOMPARE(combined, (QList<ModPlatform::ModLoaderType>{ ModPlatform::NeoForge, ModPlatform::Quilt, ModPlatform::Fabric }));
        QVERIFY(!combined.contains(ModPlatform::DataPack));
    }

    void test_hasSingleModLoaderSelected()
    {
        QVERIFY(!ModPlatform::hasSingleModLoaderSelected(ModPlatform::None));
        QVERIFY(ModPlatform::hasSingleModLoaderSelected(ModPlatform::Fabric));
        QVERIFY(ModPlatform::hasSingleModLoaderSelected(ModPlatform::DataPack));
        QVERIFY(!ModPlatform::hasSingleModLoaderSelected(ModPlatform::Fabric | ModPlatform::Forge));
    }

    void test_providerCapabilities()
    {
        QCOMPARE(QString::fromLatin1(ModPlatform::ProviderCapabilities::name(ModPlatform::ResourceProvider::MODRINTH)),
                 QStringLiteral("modrinth"));
        QCOMPARE(QString::fromLatin1(ModPlatform::ProviderCapabilities::name(ModPlatform::ResourceProvider::FLAME)),
                 QStringLiteral("curseforge"));
        QCOMPARE(ModPlatform::ProviderCapabilities::readableName(ModPlatform::ResourceProvider::MODRINTH), QStringLiteral("Modrinth"));
        QCOMPARE(ModPlatform::ProviderCapabilities::readableName(ModPlatform::ResourceProvider::FLAME), QStringLiteral("CurseForge"));
        QCOMPARE(ModPlatform::ProviderCapabilities::hashType(ModPlatform::ResourceProvider::MODRINTH),
                 (QStringList{ QStringLiteral("sha512"), QStringLiteral("sha1") }));
        QCOMPARE(ModPlatform::ProviderCapabilities::hashType(ModPlatform::ResourceProvider::FLAME),
                 (QStringList{ QStringLiteral("sha1"), QStringLiteral("md5"), QStringLiteral("murmur2") }));
    }

    void test_getMetaURL()
    {
        QCOMPARE(ModPlatform::getMetaURL(ModPlatform::ResourceProvider::MODRINTH, QStringLiteral("P7dR8mSH")),
                 QStringLiteral("https://modrinth.com/mod/P7dR8mSH"));
        QCOMPARE(ModPlatform::getMetaURL(ModPlatform::ResourceProvider::FLAME, 306612),
                 QStringLiteral("https://www.curseforge.com/projects/306612"));
    }

    void test_dependencyAndSideFromString()
    {
        QCOMPARE(ModPlatform::DependencyType::fromString(QStringLiteral("required")).value(), ModPlatform::DependencyType::REQUIRED);
        QCOMPARE(ModPlatform::DependencyType::fromString(QStringLiteral("OPTIONAL")).value(), ModPlatform::DependencyType::OPTIONAL);
        QCOMPARE(ModPlatform::DependencyType::fromString(QStringLiteral("incompatible")).value(),
                 ModPlatform::DependencyType::INCOMPATIBLE);
        QCOMPARE(ModPlatform::DependencyType::fromString(QStringLiteral("not-a-type")).value(), ModPlatform::DependencyType::UNKNOWN);

        QCOMPARE(ModPlatform::SideType::fromString(QStringLiteral("client")).value(), ModPlatform::SideType::ClientSide);
        QCOMPARE(ModPlatform::SideType::fromString(QStringLiteral("server")).value(), ModPlatform::SideType::ServerSide);
        QCOMPARE(ModPlatform::SideType::fromString(QStringLiteral("both")).value(), ModPlatform::SideType::UniversalSide);
        QCOMPARE(ModPlatform::SideType::fromString(QStringLiteral("CLIENT")).value(), ModPlatform::SideType::NoSide);
    }

    void test_versionDisplayString()
    {
        ModPlatform::IndexedVersion named;
        named.version = QStringLiteral("Sodium");
        named.versionNumber = QStringLiteral("0.5.8");
        named.mcVersion = QStringList{ QStringLiteral("1.21.1"), QStringLiteral("1.21") };
        named.versionType = ModPlatform::IndexedVersionType::Release;
        QCOMPARE(named.getVersionDisplayString(), QStringLiteral("Sodium for 1.21.1 — 0.5.8 [Release]"));

        ModPlatform::IndexedVersion embedded;
        embedded.version = QStringLiteral("1.21.1+build.1");
        embedded.versionNumber = QStringLiteral("1.21.1+build.1");
        embedded.mcVersion = QStringList{ QStringLiteral("1.21.1") };
        QCOMPARE(embedded.getVersionDisplayString(), QStringLiteral("1.21.1+build.1 — "));

        ModPlatform::IndexedVersion beta;
        beta.version = QStringLiteral("Nightly");
        beta.versionNumber = QStringLiteral("0.1.0");
        beta.mcVersion = QStringList{ QStringLiteral("1.20.1") };
        beta.versionType = ModPlatform::IndexedVersionType::Beta;
        QCOMPARE(beta.getVersionDisplayString(), QStringLiteral("Nightly for 1.20.1 — 0.1.0 [Beta]"));
    }

    void test_packVersionSelection()
    {
        ModPlatform::IndexedPack unloaded;
        QVERIFY(!unloaded.isAnyVersionSelected());

        ModPlatform::IndexedPack pack;
        pack.versionsLoaded = true;
        pack.versions = { ModPlatform::IndexedVersion{}, ModPlatform::IndexedVersion{} };
        QVERIFY(!pack.isAnyVersionSelected());
        QVERIFY(!pack.isVersionSelected(0));

        pack.versions[1].isCurrentlySelected = true;
        QVERIFY(pack.isAnyVersionSelected());
        QVERIFY(!pack.isVersionSelected(0));
        QVERIFY(pack.isVersionSelected(1));
    }
};

QTEST_GUILESS_MAIN(ModIndexTest)
#include "ModIndex_test.moc"
