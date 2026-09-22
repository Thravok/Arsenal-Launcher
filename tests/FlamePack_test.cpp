#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

#include <FileSystem.h>
#include <Json.h>
#include <modplatform/flame/FlameModIndex.h>
#include <modplatform/flame/PackManifest.h>

class FlamePackTest : public QObject {
    Q_OBJECT

    static QString writeJson(const QTemporaryDir& dir, const QString& name, const QByteArray& json)
    {
        const QString path = QDir(dir.path()).filePath(name);
        FS::write(path, json);
        return path;
    }

   private slots:
    void test_loadManifestParsesFilesLoadersAndDefaults()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QByteArray json = R"({
          "manifestType": "minecraftModpack",
          "manifestVersion": 1,
          "minecraft": {
            "version": "1.20.1",
            "recommendedRam": 4096,
            "modLoaders": [
              { "id": "forge-47.2.0", "primary": true },
              { "id": "fabric-0.16.9" }
            ]
          },
          "name": "Example Pack",
          "version": "1.3.0",
          "author": "PackAuthor",
          "overrides": "overrides",
          "files": [
            { "projectID": 238222, "fileID": 4575702, "required": false },
            { "projectID": 306612, "fileID": 5000001 }
          ]
        })";

        Flame::Manifest manifest;
        Flame::loadManifest(manifest, writeJson(dir, "manifest.json", json));

        QVERIFY(manifest.isLoaded);
        QCOMPARE(manifest.manifestType, QStringLiteral("minecraftModpack"));
        QCOMPARE(manifest.manifestVersion, 1);
        QCOMPARE(manifest.name, QStringLiteral("Example Pack"));
        QCOMPARE(manifest.version, QStringLiteral("1.3.0"));
        QCOMPARE(manifest.author, QStringLiteral("PackAuthor"));
        QCOMPARE(manifest.overrides, QStringLiteral("overrides"));
        QCOMPARE(manifest.minecraft.version, QStringLiteral("1.20.1"));
        QCOMPARE(manifest.minecraft.recommendedRAM, 4096);
        QCOMPARE(manifest.minecraft.modLoaders.size(), 2);
        QCOMPARE(manifest.minecraft.modLoaders.first().id, QStringLiteral("forge-47.2.0"));
        QVERIFY(manifest.minecraft.modLoaders.first().primary);
        QVERIFY(!manifest.minecraft.modLoaders.last().primary);
        QCOMPARE(manifest.files.size(), 2);
        QCOMPARE(manifest.files.value(4575702).projectId, 238222);
        QVERIFY(!manifest.files.value(4575702).required);
        QCOMPARE(manifest.files.value(5000001).projectId, 306612);
        QVERIFY(manifest.files.value(5000001).required);
    }

    void test_loadManifestDefaultsAndRejectsUnknownKinds()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QByteArray minimal = R"({
          "manifestType": "minecraftModpack",
          "manifestVersion": 1,
          "minecraft": { "version": "1.21.1" }
        })";

        Flame::Manifest manifest;
        Flame::loadManifest(manifest, writeJson(dir, "minimal.json", minimal));
        QCOMPARE(manifest.name, QStringLiteral("Unnamed"));
        QCOMPARE(manifest.author, QStringLiteral("Anonymous"));
        QCOMPARE(manifest.overrides, QStringLiteral("overrides"));
        QVERIFY(manifest.files.isEmpty());
        QVERIFY(manifest.minecraft.modLoaders.isEmpty());

        QVERIFY_THROWS_EXCEPTION(JSONValidationError, Flame::loadManifest(manifest, writeJson(dir, "wrong-type.json", R"({"manifestType":"modpack","manifestVersion":1,"minecraft":{"version":"1.20.1"}})")));
        QVERIFY_THROWS_EXCEPTION(JSONValidationError, Flame::loadManifest(manifest, writeJson(dir, "wrong-version.json", R"({"manifestType":"minecraftModpack","manifestVersion":2,"minecraft":{"version":"1.20.1"}})")));
    }

    void test_loadIndexedPackMapsClassAndLogoFallback()
    {
        QJsonObject packObj{
            { "id", 238222 },
            { "name", QStringLiteral("JEI") },
            { "slug", QStringLiteral("jei") },
            { "summary", QStringLiteral("Just Enough Items") },
            { "classId", 6 },
            { "links",
              QJsonObject{ { "websiteUrl", QStringLiteral("https://www.curseforge.com/minecraft/mc-mods/jei") },
                           { "issuesUrl", QStringLiteral("https://github.com/mezz/JustEnoughItems/issues/") },
                           { "sourceUrl", QStringLiteral("https://github.com/mezz/JustEnoughItems") } } },
            { "logo", QJsonObject{ { "title", QStringLiteral("logo") }, { "url", QStringLiteral("https://cdn.example/logo.png") } } },
            { "authors", QJsonArray{ QJsonObject{ { "name", QStringLiteral("mezz") }, { "url", QStringLiteral("https://example.test/mezz") } } } },
        };

        ModPlatform::IndexedPack pack;
        FlameMod::loadIndexedPack(pack, packObj);
        QCOMPARE(pack.addonId.toInt(), 238222);
        QCOMPARE(pack.provider, ModPlatform::ResourceProvider::FLAME);
        QCOMPARE(pack.name, QStringLiteral("JEI"));
        QCOMPARE(pack.slug, QStringLiteral("jei"));
        QCOMPARE(pack.resourceType, ModPlatform::ResourceType::Mod);
        QCOMPARE(pack.logoUrl, QStringLiteral("https://cdn.example/logo.png"));
        QCOMPARE(pack.authors.size(), 1);
        QCOMPARE(pack.authors.first().name, QStringLiteral("mezz"));
        QCOMPARE(pack.extraData.issuesUrl, QStringLiteral("https://github.com/mezz/JustEnoughItems/issues"));
        QCOMPARE(pack.extraData.sourceUrl, QStringLiteral("https://github.com/mezz/JustEnoughItems"));

        packObj["classId"] = 6945;
        packObj["logo"] = QJsonObject{ { "title", QStringLiteral("thumb") },
                                       { "thumbnailUrl", QStringLiteral("https://cdn.example/thumb.png") },
                                       { "url", QStringLiteral("https://cdn.example/full.png") } };
        ModPlatform::IndexedPack datapack;
        FlameMod::loadIndexedPack(datapack, packObj);
        QCOMPARE(datapack.resourceType, ModPlatform::ResourceType::DataPack);
        QCOMPARE(datapack.logoUrl, QStringLiteral("https://cdn.example/thumb.png"));
    }

    void test_loadIndexedPackVersionParsersLoadersHashesAndDependencies()
    {
        QJsonObject file{
            { "modId", 238222 },
            { "id", 4575702 },
            { "fileDate", QStringLiteral("2024-06-01T00:00:00Z") },
            { "displayName", QStringLiteral("jei-1.20.1") },
            { "downloadUrl", QStringLiteral("https://cdn.example/jei.jar") },
            { "fileName", QStringLiteral("jei:1.20.1?.jar") },
            { "releaseType", 1 },
            { "gameVersions", QJsonArray{ "1.20.1", "Forge", "Client" } },
            { "hashes", QJsonArray{ QJsonObject{ { "algo", 2 }, { "value", QStringLiteral("md5hash") } },
                                    QJsonObject{ { "algo", 1 }, { "value", QStringLiteral("sha1hash") } } } },
            { "dependencies",
              QJsonArray{ QJsonObject{ { "modId", 100 }, { "relationType", 3 } }, QJsonObject{ { "modId", 200 }, { "relationType", 5 } },
                          QJsonObject{ { "modId", 300 }, { "relationType", 2 } } } },
        };

        auto version = FlameMod::loadIndexedPackVersion(file);
        QCOMPARE(version.addonId.toInt(), 238222);
        QCOMPARE(version.fileId.toInt(), 4575702);
        QCOMPARE(version.mcVersion, QStringList{ QStringLiteral("1.20.1") });
        QVERIFY(version.loaders & ModPlatform::Forge);
        QVERIFY(!(version.loaders & ModPlatform::Fabric));
        QCOMPARE(version.side, ModPlatform::SideType::ClientSide);
        QCOMPARE(version.versionType, ModPlatform::IndexedVersionType::Release);
        QCOMPARE(version.fileName, QStringLiteral("jei-1.20.1-.jar"));
        QCOMPARE(version.hashType, QStringLiteral("md5"));
        QCOMPARE(version.hash, QStringLiteral("md5hash"));
        QCOMPARE(version.dependencies.size(), 3);
        QCOMPARE(version.dependencies[0].type, ModPlatform::DependencyType::REQUIRED);
        QCOMPARE(version.dependencies[1].type, ModPlatform::DependencyType::INCOMPATIBLE);
        QCOMPARE(version.dependencies[2].type, ModPlatform::DependencyType::OPTIONAL);

        file["gameVersions"] = QJsonArray{ "1.21.1", "NeoForge", "Quilt", "Fabric" };
        file["releaseType"] = 2;
        file["hashes"] = QJsonArray{ QJsonObject{ { "algo", 1 }, { "value", QStringLiteral("onlysha1") } } };
        file["fileName"] = QStringLiteral("multi.jar");
        auto multi = FlameMod::loadIndexedPackVersion(file);
        QVERIFY(multi.loaders & ModPlatform::NeoForge);
        QVERIFY(multi.loaders & ModPlatform::Quilt);
        QVERIFY(multi.loaders & ModPlatform::Fabric);
        QCOMPARE(multi.versionType, ModPlatform::IndexedVersionType::Beta);
        QCOMPARE(multi.hashType, QStringLiteral("sha1"));
        QCOMPARE(multi.side, ModPlatform::SideType::NoSide);

        file["releaseType"] = 99;
        QCOMPARE(FlameMod::loadIndexedPackVersion(file).versionType, ModPlatform::IndexedVersionType::Unknown);

        file.remove("modId");
        QVERIFY_THROWS_EXCEPTION(JSONValidationError, FlameMod::loadIndexedPackVersion(file));
    }

    void test_loadIndexedPackVersionsSortsByDateDescending()
    {
        QJsonArray arr{
            QJsonObject{ { "modId", 1 },
                         { "id", 10 },
                         { "fileDate", QStringLiteral("2023-01-01T00:00:00Z") },
                         { "displayName", QStringLiteral("old") },
                         { "fileName", QStringLiteral("old.jar") },
                         { "releaseType", 1 },
                         { "gameVersions", QJsonArray{ "1.20.1" } } },
            QJsonObject{ { "modId", 1 },
                         { "id", 20 },
                         { "fileDate", QStringLiteral("2024-06-01T00:00:00Z") },
                         { "displayName", QStringLiteral("new") },
                         { "fileName", QStringLiteral("new.jar") },
                         { "releaseType", 1 },
                         { "gameVersions", QJsonArray{ "1.20.1" } } },
        };

        ModPlatform::IndexedPack pack;
        pack.addonId = 1;
        FlameMod::loadIndexedPackVersions(pack, arr);
        QVERIFY(pack.versionsLoaded);
        QCOMPARE(pack.versions.size(), 2);
        QCOMPARE(pack.versions.first().fileName, QStringLiteral("new.jar"));
        QCOMPARE(pack.versions.last().fileName, QStringLiteral("old.jar"));
    }
};

QTEST_GUILESS_MAIN(FlamePackTest)
#include "FlamePack_test.moc"
