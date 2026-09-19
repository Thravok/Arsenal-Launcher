#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QTest>

#include <Json.h>
#include <modplatform/ModIndex.h>
#include <modplatform/modrinth/ModrinthAPI.h>
#include <modplatform/modrinth/ModrinthPackIndex.h>

class ModrinthPackIndexTest : public QObject {
    Q_OBJECT

    static QJsonObject samplePack(const QString& projectType = QStringLiteral("mod"))
    {
        QJsonObject obj;
        obj.insert(QStringLiteral("project_id"), QStringLiteral("aabbcc"));
        obj.insert(QStringLiteral("title"), QStringLiteral("Cool Pack"));
        obj.insert(QStringLiteral("project_type"), projectType);
        obj.insert(QStringLiteral("slug"), QStringLiteral("cool-pack"));
        obj.insert(QStringLiteral("description"), QStringLiteral("A pack"));
        obj.insert(QStringLiteral("icon_url"), QStringLiteral("https://cdn.example/icon.png"));
        obj.insert(QStringLiteral("author"), QStringLiteral("alice"));
        obj.insert(QStringLiteral("client_side"), QStringLiteral("required"));
        obj.insert(QStringLiteral("server_side"), QStringLiteral("optional"));
        return obj;
    }

    static QJsonObject sampleVersion()
    {
        QJsonObject dep;
        dep.insert(QStringLiteral("project_id"), QStringLiteral("dep-1"));
        dep.insert(QStringLiteral("version_id"), QStringLiteral("dep-ver"));
        dep.insert(QStringLiteral("dependency_type"), QStringLiteral("required"));

        QJsonObject sources;
        sources.insert(QStringLiteral("filename"), QStringLiteral("cool-pack-sources.jar"));
        sources.insert(QStringLiteral("primary"), false);
        sources.insert(QStringLiteral("url"), QStringLiteral("https://cdn.example/sources.jar"));
        QJsonObject sourcesHashes;
        sourcesHashes.insert(QStringLiteral("sha1"), QStringLiteral("abc"));
        sources.insert(QStringLiteral("hashes"), sourcesHashes);

        QJsonObject primary;
        primary.insert(QStringLiteral("filename"), QStringLiteral("cool-pack.jar"));
        primary.insert(QStringLiteral("primary"), true);
        primary.insert(QStringLiteral("url"), QStringLiteral("https://cdn.example/cool-pack.jar"));
        QJsonObject primaryHashes;
        primaryHashes.insert(QStringLiteral("sha512"), QStringLiteral("deadbeef"));
        primaryHashes.insert(QStringLiteral("sha1"), QStringLiteral("abc"));
        primary.insert(QStringLiteral("hashes"), primaryHashes);

        QJsonObject obj;
        obj.insert(QStringLiteral("project_id"), QStringLiteral("aabbcc"));
        obj.insert(QStringLiteral("id"), QStringLiteral("ver-1"));
        obj.insert(QStringLiteral("date_published"), QStringLiteral("2026-01-02T00:00:00Z"));
        obj.insert(QStringLiteral("game_versions"), QJsonArray{ QStringLiteral("1.21.1"), QStringLiteral("1.21.1-pre2") });
        obj.insert(QStringLiteral("loaders"), QJsonArray{ QStringLiteral("fabric"), QStringLiteral("quilt") });
        obj.insert(QStringLiteral("name"), QStringLiteral("1.2.3"));
        obj.insert(QStringLiteral("version_number"), QStringLiteral("1.2.3"));
        obj.insert(QStringLiteral("version_type"), QStringLiteral("release"));
        obj.insert(QStringLiteral("changelog"), QStringLiteral("fixes"));
        obj.insert(QStringLiteral("dependencies"), QJsonArray{ dep });
        obj.insert(QStringLiteral("files"), QJsonArray{ sources, primary });
        return obj;
    }

   private slots:
    void resourceTypeMapping()
    {
        QCOMPARE(ModrinthAPI::getResourceType(QStringLiteral("mod")), ModPlatform::ResourceType::Mod);
        QCOMPARE(ModrinthAPI::getResourceType(QStringLiteral("datapack")), ModPlatform::ResourceType::DataPack);
        QCOMPARE(ModrinthAPI::getResourceType(QStringLiteral("resourcepack")), ModPlatform::ResourceType::ResourcePack);
        QCOMPARE(ModrinthAPI::getResourceType(QStringLiteral("shader")), ModPlatform::ResourceType::ShaderPack);
        QCOMPARE(ModrinthAPI::getResourceType(QStringLiteral("modpack")), ModPlatform::ResourceType::Modpack);
        QCOMPARE(ModrinthAPI::getResourceType(QStringLiteral("world")), ModPlatform::ResourceType::Unknown);
        QCOMPARE(ModrinthAPI::getResourceType(QString()), ModPlatform::ResourceType::Unknown);
    }

    void mapMCVersionFromModrinth()
    {
        QCOMPARE(ModrinthAPI::mapMCVersionFromModrinth(QStringLiteral("1.21.1")), QStringLiteral("1.21.1"));
        QCOMPARE(ModrinthAPI::mapMCVersionFromModrinth(QStringLiteral("1.21.1-pre2")), QStringLiteral("1.21.1 Pre-Release 2"));
        QCOMPARE(ModrinthAPI::mapMCVersionFromModrinth(QStringLiteral("26.1-snapshot")), QStringLiteral("26.1 snapshot"));
    }

    void indexedVersionTypeRoundTrip()
    {
        QCOMPARE(ModPlatform::IndexedVersionType::fromString(QStringLiteral("release")), ModPlatform::IndexedVersionType::Release);
        QCOMPARE(ModPlatform::IndexedVersionType::fromString(QStringLiteral("beta")), ModPlatform::IndexedVersionType::Beta);
        QCOMPARE(ModPlatform::IndexedVersionType::fromString(QStringLiteral("alpha")), ModPlatform::IndexedVersionType::Alpha);
        QCOMPARE(ModPlatform::IndexedVersionType::fromString(QStringLiteral("Release")).isValid(), false);
        QCOMPARE(ModPlatform::IndexedVersionType(ModPlatform::IndexedVersionType::Release).toString(), QStringLiteral("release"));
    }

    void loaderStringRoundTrip()
    {
        QCOMPARE(ModPlatform::getModLoaderFromString(QStringLiteral("fabric")), ModPlatform::Fabric);
        QCOMPARE(ModPlatform::getModLoaderFromString(QStringLiteral("neoforge")), ModPlatform::NeoForge);
        QCOMPARE(ModPlatform::getModLoaderAsString(ModPlatform::DataPack), QStringLiteral("datapack"));
        QCOMPARE(ModPlatform::getModLoaderFromString(QStringLiteral("datapack")), ModPlatform::None);
        QVERIFY(ModPlatform::hasSingleModLoaderSelected(ModPlatform::Fabric));
        QVERIFY(!ModPlatform::hasSingleModLoaderSelected(ModPlatform::Fabric | ModPlatform::Quilt));
        QVERIFY(!ModPlatform::hasSingleModLoaderSelected(ModPlatform::None));
    }

    void loadIndexedPackUsesProjectType()
    {
        auto obj = samplePack(QStringLiteral("datapack"));
        ModPlatform::IndexedPack pack;
        Modrinth::loadIndexedPack(pack, obj);

        QCOMPARE(pack.addonId.toString(), QStringLiteral("aabbcc"));
        QCOMPARE(pack.provider, ModPlatform::ResourceProvider::MODRINTH);
        QCOMPARE(pack.name, QStringLiteral("Cool Pack"));
        QCOMPARE(pack.resourceType, ModPlatform::ResourceType::DataPack);
        QCOMPARE(pack.slug, QStringLiteral("cool-pack"));
        QCOMPARE(pack.websiteUrl, QStringLiteral("https://modrinth.com/mod/cool-pack"));
        QCOMPARE(pack.authors.size(), 1);
        QCOMPARE(pack.authors.first().url, QStringLiteral("https://modrinth.com/user/alice"));
        QCOMPARE(pack.side, ModPlatform::SideType::UniversalSide);
        QVERIFY(!pack.extraDataLoaded);
    }

    void loadIndexedPackDetectsDatapackFromLoaders()
    {
        auto obj = samplePack(QStringLiteral("mod"));
        obj.insert(QStringLiteral("loaders"), QJsonArray{ QStringLiteral("fabric"), QStringLiteral("datapack") });
        ModPlatform::IndexedPack pack;
        Modrinth::loadIndexedPack(pack, obj);
        QCOMPARE(pack.resourceType, ModPlatform::ResourceType::DataPack);
    }

    void loadIndexedPackDetectsDatapackFromAllProjectTypes()
    {
        auto obj = samplePack(QStringLiteral("mod"));
        obj.insert(QStringLiteral("all_project_types"), QJsonArray{ QStringLiteral("mod"), QStringLiteral("datapack") });
        ModPlatform::IndexedPack pack;
        Modrinth::loadIndexedPack(pack, obj);
        QCOMPARE(pack.resourceType, ModPlatform::ResourceType::DataPack);
    }

    void loadIndexedPackKeepsModWhenLoadersAreUnrelated()
    {
        auto obj = samplePack(QStringLiteral("mod"));
        obj.insert(QStringLiteral("loaders"), QJsonArray{ QStringLiteral("fabric") });
        ModPlatform::IndexedPack pack;
        Modrinth::loadIndexedPack(pack, obj);
        QCOMPARE(pack.resourceType, ModPlatform::ResourceType::Mod);
    }

    void loadIndexedPackFallsBackToId()
    {
        auto obj = samplePack();
        obj.remove(QStringLiteral("project_id"));
        obj.insert(QStringLiteral("id"), QStringLiteral("from-id"));
        ModPlatform::IndexedPack pack;
        Modrinth::loadIndexedPack(pack, obj);
        QCOMPARE(pack.addonId.toString(), QStringLiteral("from-id"));
    }

    void loadIndexedPackSides()
    {
        auto clientOnly = samplePack();
        clientOnly.insert(QStringLiteral("client_side"), QStringLiteral("required"));
        clientOnly.insert(QStringLiteral("server_side"), QStringLiteral("unsupported"));
        ModPlatform::IndexedPack clientPack;
        Modrinth::loadIndexedPack(clientPack, clientOnly);
        QCOMPARE(clientPack.side, ModPlatform::SideType::ClientSide);

        auto serverOnly = samplePack();
        serverOnly.insert(QStringLiteral("client_side"), QStringLiteral("unsupported"));
        serverOnly.insert(QStringLiteral("server_side"), QStringLiteral("required"));
        ModPlatform::IndexedPack serverPack;
        Modrinth::loadIndexedPack(serverPack, serverOnly);
        QCOMPARE(serverPack.side, ModPlatform::SideType::ServerSide);
    }

    void loadIndexedPackRequiresTitle()
    {
        auto obj = samplePack();
        obj.remove(QStringLiteral("title"));
        ModPlatform::IndexedPack pack;
        QVERIFY_THROWS_EXCEPTION(Json::JsonException, Modrinth::loadIndexedPack(pack, obj));
    }

    void loadExtraPackDataStripsTrailingSlashes()
    {
        QJsonObject donate;
        donate.insert(QStringLiteral("id"), QStringLiteral("ko-fi"));
        donate.insert(QStringLiteral("platform"), QStringLiteral("Ko-fi"));
        donate.insert(QStringLiteral("url"), QStringLiteral("https://ko-fi.com/x"));

        QJsonObject obj;
        obj.insert(QStringLiteral("issues_url"), QStringLiteral("https://ex.test/issues/"));
        obj.insert(QStringLiteral("source_url"), QStringLiteral("https://ex.test/src/"));
        obj.insert(QStringLiteral("wiki_url"), QStringLiteral("https://ex.test/wiki"));
        obj.insert(QStringLiteral("discord_url"), QStringLiteral("https://ex.test/discord/"));
        obj.insert(QStringLiteral("status"), QStringLiteral("approved"));
        obj.insert(QStringLiteral("body"), QStringLiteral("hello<br>world"));
        obj.insert(QStringLiteral("donation_urls"), QJsonArray{ donate });
        ModPlatform::IndexedPack pack;
        Modrinth::loadExtraPackData(pack, obj);
        QVERIFY(pack.extraDataLoaded);
        QCOMPARE(pack.extraData.issuesUrl, QStringLiteral("https://ex.test/issues"));
        QCOMPARE(pack.extraData.sourceUrl, QStringLiteral("https://ex.test/src"));
        QCOMPARE(pack.extraData.wikiUrl, QStringLiteral("https://ex.test/wiki"));
        QCOMPARE(pack.extraData.discordUrl, QStringLiteral("https://ex.test/discord"));
        QCOMPARE(pack.extraData.body, QStringLiteral("helloworld"));
        QCOMPARE(pack.extraData.donate.size(), 1);
        QCOMPARE(pack.extraData.donate.first().platform, QStringLiteral("Ko-fi"));
    }

    void loadIndexedPackVersionPrefersPrimaryAndSha512()
    {
        auto obj = sampleVersion();
        const auto file = Modrinth::loadIndexedPackVersion(obj);
        QCOMPARE(file.addonId.toString(), QStringLiteral("aabbcc"));
        QCOMPARE(file.fileId.toString(), QStringLiteral("ver-1"));
        QCOMPARE(file.fileName, QStringLiteral("cool-pack.jar"));
        QCOMPARE(file.downloadUrl, QStringLiteral("https://cdn.example/cool-pack.jar"));
        QCOMPARE(file.hashType, QStringLiteral("sha512"));
        QCOMPARE(file.hash, QStringLiteral("deadbeef"));
        QVERIFY(file.isPreferred);
        QCOMPARE(file.versionType, ModPlatform::IndexedVersionType::Release);
        QVERIFY(file.loaders.testFlag(ModPlatform::Fabric));
        QVERIFY(file.loaders.testFlag(ModPlatform::Quilt));
        QVERIFY(!file.loaders.testFlag(ModPlatform::Forge));
        QCOMPARE(file.mcVersion.size(), 4);
        QCOMPARE(file.mcVersion.at(0), QStringLiteral("1.21.1"));
        QCOMPARE(file.mcVersion.at(2), QStringLiteral("1.21.1 Pre-Release 2"));
        QCOMPARE(file.dependencies.size(), 1);
        QCOMPARE(file.dependencies.first().type, ModPlatform::DependencyType::REQUIRED);
        QCOMPARE(file.changelog, QStringLiteral("fixes"));
    }

    void loadIndexedPackVersionPreferredFileNameWins()
    {
        auto obj = sampleVersion();
        const auto file = Modrinth::loadIndexedPackVersion(obj, QStringLiteral("sha1"), QStringLiteral("sources"));
        QCOMPARE(file.fileName, QStringLiteral("cool-pack-sources.jar"));
        QCOMPARE(file.hashType, QStringLiteral("sha1"));
        QCOMPARE(file.hash, QStringLiteral("abc"));
        QVERIFY(!file.isPreferred);
    }

    void loadIndexedPackVersionEmptyFilesReturnsEmpty()
    {
        auto obj = sampleVersion();
        obj.insert(QStringLiteral("files"), QJsonArray{});
        const auto file = Modrinth::loadIndexedPackVersion(obj);
        QVERIFY(file.downloadUrl.isEmpty());
        QVERIFY(file.fileId.toString().isEmpty());
    }

    void loadIndexedPackVersionEmptyGameVersionsReturnsEmpty()
    {
        auto obj = sampleVersion();
        obj.insert(QStringLiteral("game_versions"), QJsonArray{});
        const auto file = Modrinth::loadIndexedPackVersion(obj);
        QVERIFY(file.fileId.toString().isEmpty());
    }
};

QTEST_GUILESS_MAIN(ModrinthPackIndexTest)
#include "ModrinthPackIndex_test.moc"
