#include <QTest>
#include <memory>
#include <vector>

#include <BuildConfig.h>
#include <Version.h>
#include <modplatform/ModIndex.h>
#include <modplatform/ResourceAPI.h>
#include <modplatform/ResourceType.h>
#include <modplatform/flame/FlameAPI.h>

class FlameSearchUrlTest : public QObject {
    Q_OBJECT

    static ResourceAPI::SearchArgs searchArgs(ModPlatform::ResourceType type,
                                              ModPlatform::ModLoaderTypes loaders = ModPlatform::None,
                                              const QString& search = {},
                                              const QString& gameVersion = {})
    {
        ResourceAPI::SearchArgs args;
        args.type = type;
        args.offset = 25;
        args.loaders = loaders;
        if (!search.isEmpty()) {
            args.search = search;
        }
        if (!gameVersion.isEmpty()) {
            args.versions = std::vector<Version>{ Version(gameVersion) };
        }
        return args;
    }

    static ResourceAPI::VersionSearchArgs versionArgs(const QString& addonId,
                                                      ModPlatform::ModLoaderTypes loaders,
                                                      const QString& gameVersion = QStringLiteral("1.20.1"))
    {
        auto pack = std::make_shared<ModPlatform::IndexedPack>();
        pack->addonId = addonId;
        ResourceAPI::VersionSearchArgs args;
        args.pack = pack;
        args.loaders = loaders;
        args.mcVersions = std::vector<Version>{ Version(gameVersion) };
        return args;
    }

   private slots:
    void test_validateModLoaders()
    {
        QVERIFY(FlameAPI::validateModLoaders(ModPlatform::Forge));
        QVERIFY(FlameAPI::validateModLoaders(ModPlatform::Fabric));
        QVERIFY(FlameAPI::validateModLoaders(ModPlatform::Quilt));
        QVERIFY(FlameAPI::validateModLoaders(ModPlatform::NeoForge));
        QVERIFY(FlameAPI::validateModLoaders(ModPlatform::Forge | ModPlatform::DataPack));
        QVERIFY(!FlameAPI::validateModLoaders(ModPlatform::None));
        QVERIFY(!FlameAPI::validateModLoaders(ModPlatform::DataPack));
        QVERIFY(!FlameAPI::validateModLoaders(ModPlatform::Cauldron));
        QVERIFY(!FlameAPI::validateModLoaders(ModPlatform::LiteLoader));
    }

    void test_searchUrlUsesClassIdAndStripsDatapackLoader()
    {
        const auto& api = FlameAPI::get();
        const auto url = api.getSearchURL(searchArgs(ModPlatform::ResourceType::Mod, ModPlatform::Forge | ModPlatform::DataPack,
                                                     QStringLiteral("sodium"), QStringLiteral("1.20.1")));
        QVERIFY(url.has_value());
        QVERIFY(url->startsWith(BuildConfig.FLAME_BASE_URL + QStringLiteral("/mods/search?gameId=432&")));
        QVERIFY(url->contains(QStringLiteral("classId=6")));
        QVERIFY(url->contains(QStringLiteral("index=25")));
        QVERIFY(url->contains(QStringLiteral("pageSize=25")));
        QVERIFY(url->contains(QStringLiteral("searchFilter=sodium")));
        QVERIFY(url->contains(QStringLiteral("gameVersion=1.20.1")));
        QVERIFY(url->contains(QStringLiteral("modLoaderTypes=[1]")));
        QVERIFY(!url->contains(QStringLiteral("modLoaderTypes=[6]")));
    }

    void test_searchUrlOmitsLoaderFilterWhenOnlyDatapack()
    {
        const auto& api = FlameAPI::get();
        const auto url = api.getSearchURL(searchArgs(ModPlatform::ResourceType::DataPack, ModPlatform::DataPack));
        QVERIFY(url.has_value());
        QVERIFY(url->contains(QStringLiteral("classId=6945")));
        QVERIFY(!url->contains(QStringLiteral("modLoaderTypes=")));
    }

    void test_searchUrlMapsSupportedLoaders()
    {
        const auto& api = FlameAPI::get();
        const auto url =
            api.getSearchURL(searchArgs(ModPlatform::ResourceType::Mod, ModPlatform::NeoForge | ModPlatform::Fabric | ModPlatform::Quilt));
        QVERIFY(url.has_value());
        QVERIFY(url->contains(QStringLiteral("modLoaderTypes=[6,4,5]")));
    }

    void test_versionsUrlAddsSingleLoaderAndSkipsDatapack()
    {
        const auto& api = FlameAPI::get();

        const auto forgeUrl = api.getVersionsURL(versionArgs(QStringLiteral("99"), ModPlatform::Forge));
        QVERIFY(forgeUrl.has_value());
        QVERIFY(forgeUrl->contains(QStringLiteral("/mods/99/files?pageSize=10000")));
        QVERIFY(forgeUrl->contains(QStringLiteral("gameVersion=1.20.1")));
        QVERIFY(forgeUrl->contains(QStringLiteral("modLoaderType=1")));

        const auto datapackUrl = api.getVersionsURL(versionArgs(QStringLiteral("99"), ModPlatform::DataPack));
        QVERIFY(datapackUrl.has_value());
        QVERIFY(!datapackUrl->contains(QStringLiteral("modLoaderType=")));

        const auto multiUrl = api.getVersionsURL(versionArgs(QStringLiteral("99"), ModPlatform::Forge | ModPlatform::Fabric));
        QVERIFY(multiUrl.has_value());
        QVERIFY(!multiUrl->contains(QStringLiteral("modLoaderType=")));
    }
};

QTEST_GUILESS_MAIN(FlameSearchUrlTest)
#include "FlameSearchUrl_test.moc"
