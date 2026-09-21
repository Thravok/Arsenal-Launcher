#include <QTest>

#include "modplatform/ModIndex.h"
#include "modplatform/ResourceType.h"
#include "modplatform/modrinth/ModrinthAPI.h"

class ModrinthSearchTest : public QObject {
    Q_OBJECT

   private slots:
    void test_disclosureTypeApiStrings()
    {
        using D = ModPlatform::DisclosureType;

        const struct {
            D type;
            const char* api;
        } cases[] = {
            { D::AIContent, "ai_content" },
            { D::AIContentCode, "ai_content_code" },
            { D::AIContentAssets, "ai_content_assets" },
            { D::AIContentText, "ai_content_text" },
            { D::AIContentFunctionality, "ai_content_functionality" },
            { D::Advertisements, "advertisements" },
            { D::EpilepsyTriggers, "epilepsy_triggers" },
            { D::SystemInteractions, "system_interactions" },
            { D::Telemetry, "telemetry" },
            { D::TelemetryOptIn, "telemetry_opt_in" },
            { D::TelemetryOptOut, "telemetry_opt_out" },
            { D::TelemetryAlwaysActive, "telemetry_always_active" },
            { D::DerivativeWork, "derivative_work" },
            { D::PaidFeatures, "paid_features" },
            { D::Archived, "archived" },
        };

        for (const auto& c : cases) {
            QCOMPARE(c.type.toString(), QString::fromLatin1(c.api));
            QCOMPARE(D::fromString(QString::fromLatin1(c.api)), c.type);
            QVERIFY(D::fromString(QString::fromLatin1(c.api)).isValid());
        }

        QVERIFY(!D(D::Unknown).isValid());
        QVERIFY(!D::fromString(QStringLiteral("AIContent")).isValid());
        QVERIFY(!D::fromString(QStringLiteral("TELEMETRY")).isValid());
        QVERIFY(!D::fromString(QStringLiteral("not-a-disclosure")).isValid());
        QCOMPARE(D::fromString(QStringLiteral("telemetry")).toString(), QStringLiteral("telemetry"));
    }

    void test_sideFilters()
    {
        QCOMPARE(ModrinthAPI::getSideFilters(ModPlatform::SideType::NoSide), QString());
        QVERIFY(ModrinthAPI::getSideFilters(ModPlatform::SideType::ClientSide).contains(QStringLiteral("environment:client_only")));
        QVERIFY(ModrinthAPI::getSideFilters(ModPlatform::SideType::ServerSide).contains(QStringLiteral("environment:server_only")));
        QCOMPARE(ModrinthAPI::getSideFilters(ModPlatform::SideType::UniversalSide),
                 QStringLiteral(R"("environment:client_and_server","client_or_server_prefers_both")"));

        QCOMPARE(ModPlatform::SideType::fromString(QStringLiteral("client")), ModPlatform::SideType::ClientSide);
        QCOMPARE(ModPlatform::SideType::fromString(QStringLiteral("server")), ModPlatform::SideType::ServerSide);
        QCOMPARE(ModPlatform::SideType::fromString(QStringLiteral("both")), ModPlatform::SideType::UniversalSide);
        QVERIFY(!ModPlatform::SideType::fromString(QStringLiteral("Client")).isValid());
    }

    void test_hasSingleModLoaderSelected()
    {
        QVERIFY(!ModPlatform::hasSingleModLoaderSelected(ModPlatform::None));
        QVERIFY(ModPlatform::hasSingleModLoaderSelected(ModPlatform::Fabric));
        QVERIFY(ModPlatform::hasSingleModLoaderSelected(ModPlatform::DataPack));
        QVERIFY(ModPlatform::hasSingleModLoaderSelected(ModPlatform::NeoForge));
        QVERIFY(!ModPlatform::hasSingleModLoaderSelected(ModPlatform::ModLoaderTypes(ModPlatform::Fabric) | ModPlatform::Forge));
        QVERIFY(!ModPlatform::hasSingleModLoaderSelected(ModPlatform::ModLoaderTypes(ModPlatform::Fabric) | ModPlatform::DataPack));
    }

    void test_modLoaderFiltersAndValidation()
    {
        QCOMPARE(ModrinthAPI::getModLoaderFilters(ModPlatform::Fabric), QStringLiteral("\"categories:fabric\""));
        QCOMPARE(ModrinthAPI::getModLoaderFilters(ModPlatform::DataPack), QStringLiteral("\"categories:datapack\""));
        QVERIFY(ModrinthAPI::getModLoaderFilters(ModPlatform::ModLoaderTypes(ModPlatform::Fabric) | ModPlatform::Quilt)
                    .contains(QStringLiteral("\"categories:fabric\"")));
        QVERIFY(ModrinthAPI::getModLoaderFilters(ModPlatform::ModLoaderTypes(ModPlatform::Fabric) | ModPlatform::Quilt)
                    .contains(QStringLiteral("\"categories:quilt\"")));

        QVERIFY(ModrinthAPI::validateModLoaders(ModPlatform::Fabric));
        QVERIFY(ModrinthAPI::validateModLoaders(ModPlatform::DataPack));
        QVERIFY(ModrinthAPI::validateModLoaders(ModPlatform::ModLoaderTypes(ModPlatform::Forge) | ModPlatform::Fabric));
        QVERIFY(!ModrinthAPI::validateModLoaders(ModPlatform::None));
        QVERIFY(!ModrinthAPI::validateModLoaders(ModPlatform::Cauldron));
    }

    void test_createFacetsDisclosureAndProjectType()
    {
        ResourceAPI::SearchArgs args;
        args.type = ModPlatform::ResourceType::Mod;
        args.excludeDisclosureTypes = { ModPlatform::DisclosureType::Telemetry, ModPlatform::DisclosureType::AIContent };
        args.openSource = true;

        const QString facets = ModrinthAPI::createFacets(args);
        QVERIFY(facets.contains(QStringLiteral("[\"disclosure_types!=telemetry\"]")));
        QVERIFY(facets.contains(QStringLiteral("[\"disclosure_types!=ai_content\"]")));
        QVERIFY(facets.contains(QStringLiteral("[\"open_source:true\"]")));
        QVERIFY(facets.contains(QStringLiteral("[\"project_type:mod\"]")));
        QVERIFY(facets.startsWith(QLatin1Char('[')));
        QVERIFY(facets.endsWith(QLatin1Char(']')));
    }

    void test_createFacetsLoadersSideAndDatapack()
    {
        ResourceAPI::SearchArgs args;
        args.type = ModPlatform::ResourceType::DataPack;
        args.loaders = ModPlatform::DataPack;
        args.side = ModPlatform::SideType::ClientSide;
        args.categoryIds = QStringList{ QStringLiteral("library") };

        const QString facets = ModrinthAPI::createFacets(args);
        QVERIFY(facets.contains(QStringLiteral("[\"categories:datapack\"]")));
        QVERIFY(facets.contains(QStringLiteral("environment:client_only")));
        QVERIFY(facets.contains(QStringLiteral("[\"categories:library\"]")));
        QVERIFY(facets.contains(QStringLiteral("[\"project_type:datapack\"]")));
        QVERIFY(!facets.contains(QStringLiteral("disclosure_types")));
    }

    void test_createFacetsPreReleaseGameVersion()
    {
        ResourceAPI::SearchArgs args;
        args.type = ModPlatform::ResourceType::Mod;
        args.versions = std::vector<Version>{ Version(QStringLiteral("1.21.1 Pre-Release 2")) };

        const QString facets = ModrinthAPI::createFacets(args);
        QVERIFY(facets.contains(QStringLiteral("\"versions:1.21.1-pre2\"")));
        QCOMPARE(ModrinthAPI::getGameVersionsArray({ Version(QStringLiteral("1.21.1 Pre-Release 2")) }),
                 QStringLiteral(R"("versions:1.21.1-pre2")"));
        QCOMPARE(ModrinthAPI::getGameVersionsArray({ Version(QStringLiteral("26.1 snapshot")) }),
                 QStringLiteral(R"("versions:26.1-snapshot")"));
        QCOMPARE(ModrinthAPI::getGameVersionsArray({ Version(QStringLiteral("1.21.1")), Version(QStringLiteral("1.21.1 Pre-Release 2")) }),
                 QStringLiteral(R"("versions:1.21.1","versions:1.21.1-pre2")"));
    }

    void test_getResourceType()
    {
        QCOMPARE(ModrinthAPI::getResourceType(QStringLiteral("mod")), ModPlatform::ResourceType::Mod);
        QCOMPARE(ModrinthAPI::getResourceType(QStringLiteral("resourcepack")), ModPlatform::ResourceType::ResourcePack);
        QCOMPARE(ModrinthAPI::getResourceType(QStringLiteral("shader")), ModPlatform::ResourceType::ShaderPack);
        QCOMPARE(ModrinthAPI::getResourceType(QStringLiteral("datapack")), ModPlatform::ResourceType::DataPack);
        QCOMPARE(ModrinthAPI::getResourceType(QStringLiteral("modpack")), ModPlatform::ResourceType::Modpack);
        QCOMPARE(ModrinthAPI::getResourceType(QStringLiteral("unknown-type")), ModPlatform::ResourceType::Unknown);
        QCOMPARE(ModrinthAPI::getResourceType(QStringLiteral("Mod")), ModPlatform::ResourceType::Unknown);
    }

    void test_loadCategoriesFiltersByProjectType()
    {
        const QByteArray json = QByteArrayLiteral(R"([
            {"name":"adventure","project_type":"mod"},
            {"name":"library","project_type":"mod"},
            {"name":"combat","project_type":"datapack"},
            {"name":"uncategorized"}
        ])");

        const auto mods = ModrinthAPI::loadCategories(json, QStringLiteral("mod"));
        QCOMPARE(mods.size(), 2);
        QCOMPARE(mods.at(0).name, QStringLiteral("adventure"));
        QCOMPARE(mods.at(0).id, QStringLiteral("adventure"));
        QCOMPARE(mods.at(1).name, QStringLiteral("library"));

        const auto packs = ModrinthAPI::loadCategories(json, QStringLiteral("datapack"));
        QCOMPARE(packs.size(), 1);
        QCOMPARE(packs.first().name, QStringLiteral("combat"));

        QVERIFY(ModrinthAPI::loadCategories(QByteArrayLiteral("not json"), QStringLiteral("mod")).isEmpty());
        QVERIFY(ModrinthAPI::loadCategories(QByteArrayLiteral("{}"), QStringLiteral("mod")).isEmpty());
    }

    void test_getSearchURLIncludesDisclosureFacets()
    {
        ResourceAPI::SearchArgs args;
        args.type = ModPlatform::ResourceType::Mod;
        args.offset = 25;
        args.search = QStringLiteral("sodium");
        args.excludeDisclosureTypes = { ModPlatform::DisclosureType::TelemetryAlwaysActive };
        args.openSource = true;

        const auto url = ModrinthAPI::get().getSearchURL(args);
        QVERIFY(url.has_value());
        QVERIFY(url->startsWith(BuildConfig.MODRINTH_PROD_URL + QStringLiteral("/search?")));
        QVERIFY(url->contains(QStringLiteral("offset=25")));
        QVERIFY(url->contains(QStringLiteral("query=sodium")));
        QVERIFY(url->contains(QStringLiteral("disclosure_types!=telemetry_always_active")));
        QVERIFY(url->contains(QStringLiteral("open_source:true")));
        QVERIFY(url->contains(QStringLiteral("project_type:mod")));
    }
};

QTEST_GUILESS_MAIN(ModrinthSearchTest)
#include "ModrinthSearch_test.moc"
