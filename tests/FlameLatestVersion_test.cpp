#include <QTest>

#include "modplatform/ModIndex.h"
#include "modplatform/ResourceType.h"
#include "modplatform/flame/FlameAPI.h"

namespace {

ModPlatform::IndexedVersion makeVersion(const QString& fileName,
                                        const QString& date,
                                        ModPlatform::ModLoaderTypes loaders = ModPlatform::None)
{
    ModPlatform::IndexedVersion version;
    version.fileName = fileName;
    version.date = date;
    version.loaders = loaders;
    return version;
}

}  // namespace

class FlameLatestVersionTest : public QObject {
    Q_OBJECT

   private slots:
    void test_emptyVersions()
    {
        const auto latest = FlameAPI::getLatestVersion({}, { ModPlatform::Forge }, ModPlatform::None, true);
        QVERIFY(!latest.has_value());
    }

    void test_newestDateWinsWhenNotCheckingLoaders()
    {
        const QList versions{
            makeVersion(QStringLiteral("old-fabric.jar"), QStringLiteral("2024-01-01"), ModPlatform::Fabric),
            makeVersion(QStringLiteral("new-forge.jar"), QStringLiteral("2024-06-01"), ModPlatform::Forge),
        };

        const auto latest = FlameAPI::getLatestVersion(versions, { ModPlatform::Fabric }, ModPlatform::None, false);
        QVERIFY(latest.has_value());
        QCOMPARE(latest->fileName, QStringLiteral("new-forge.jar"));
    }

    void test_prefersInstanceLoaderOverNewerOtherLoader()
    {
        const QList versions{
            makeVersion(QStringLiteral("forge-new.jar"), QStringLiteral("2024-12-01"), ModPlatform::Forge),
            makeVersion(QStringLiteral("fabric-old.jar"), QStringLiteral("2023-01-01"), ModPlatform::Fabric),
        };

        const auto latest = FlameAPI::getLatestVersion(versions, { ModPlatform::Fabric }, ModPlatform::Forge, true);
        QVERIFY(latest.has_value());
        QCOMPARE(latest->fileName, QStringLiteral("fabric-old.jar"));
    }

    void test_fallsBackToAlternateLoaderThenUnspecified()
    {
        const QList versions{
            makeVersion(QStringLiteral("quilt.jar"), QStringLiteral("2024-02-01"), ModPlatform::Quilt),
            makeVersion(QStringLiteral("unspecified.jar"), QStringLiteral("2024-01-01")),
        };

        const auto viaFallback = FlameAPI::getLatestVersion(versions, { ModPlatform::Fabric }, ModPlatform::Quilt, true);
        QVERIFY(viaFallback.has_value());
        QCOMPARE(viaFallback->fileName, QStringLiteral("quilt.jar"));

        const auto unspecified = FlameAPI::getLatestVersion(versions, { ModPlatform::Forge }, ModPlatform::None, true);
        QVERIFY(unspecified.has_value());
        QCOMPARE(unspecified->fileName, QStringLiteral("unspecified.jar"));
    }

    void test_newerUnspecifiedFileWhenOnlyTwoLoaderBuckets()
    {
        const QList versions{
            makeVersion(QStringLiteral("forge.jar"), QStringLiteral("2024-01-01"), ModPlatform::Forge),
            makeVersion(QStringLiteral("unspecified-newer.jar"), QStringLiteral("2024-08-01")),
        };

        const auto latest = FlameAPI::getLatestVersion(versions, { ModPlatform::Forge }, ModPlatform::None, true);
        QVERIFY(latest.has_value());
        QCOMPARE(latest->fileName, QStringLiteral("unspecified-newer.jar"));
    }

    void test_noMatchReturnsEmpty()
    {
        const QList versions{
            makeVersion(QStringLiteral("forge.jar"), QStringLiteral("2024-01-01"), ModPlatform::Forge),
        };

        const auto latest = FlameAPI::getLatestVersion(versions, { ModPlatform::Fabric }, ModPlatform::Quilt, true);
        QVERIFY(!latest.has_value());
    }

    void test_getResourceTypeClassIds()
    {
        QCOMPARE(FlameAPI::getResourceType(6), ModPlatform::ResourceType::Mod);
        QCOMPARE(FlameAPI::getResourceType(12), ModPlatform::ResourceType::ResourcePack);
        QCOMPARE(FlameAPI::getResourceType(17), ModPlatform::ResourceType::World);
        QCOMPARE(FlameAPI::getResourceType(6552), ModPlatform::ResourceType::ShaderPack);
        QCOMPARE(FlameAPI::getResourceType(4471), ModPlatform::ResourceType::Modpack);
        QCOMPARE(FlameAPI::getResourceType(6945), ModPlatform::ResourceType::DataPack);
        QCOMPARE(FlameAPI::getResourceType(0), ModPlatform::ResourceType::Unknown);
        QCOMPARE(FlameAPI::getResourceType(12345), ModPlatform::ResourceType::Unknown);
    }

    void test_searchUrlOmitsDatapackLoaderAndSetsClassId()
    {
        ResourceAPI::SearchArgs args;
        args.type = ModPlatform::ResourceType::DataPack;
        args.loaders = ModPlatform::DataPack;
        args.versions = std::vector<Version>{ Version(QStringLiteral("1.21.1")) };

        const auto url = FlameAPI::get().getSearchURL(args);
        QVERIFY(url.has_value());
        QVERIFY(url->contains(QStringLiteral("classId=6945")));
        QVERIFY(url->contains(QStringLiteral("gameVersion=1.21.1")));
        QVERIFY(!url->contains(QStringLiteral("modLoaderTypes")));
    }
};

QTEST_GUILESS_MAIN(FlameLatestVersionTest)
#include "FlameLatestVersion_test.moc"
