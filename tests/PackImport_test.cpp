#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

#include <FileSystem.h>
#include <Json.h>
#include <modplatform/ftb/FTBPackManifest.h>
#include <modplatform/import_ftb/PackHelpers.h>
#include <modplatform/technic/SolderPackManifest.h>

namespace {
const QByteArray kNeoForgeInstance = QByteArrayLiteral(
    "{"
    "\"uuid\":\"abc-123\","
    "\"id\":42,"
    "\"versionId\":99,"
    "\"name\":\"All the Mods\","
    "\"version\":\"2.1.0\","
    "\"mcVersion\":\"1.21.1\","
    "\"totalPlayTime\":15,"
    "\"modLoader\":\"neoforge-21.1.65\""
    "}");

const QByteArray kLegacyInstance = QByteArrayLiteral(
    "{"
    "\"uuid\":\"abc-123\","
    "\"id\":1,"
    "\"versionId\":2,"
    "\"name\":\"Legacy Pack\","
    "\"version\":\"1.0.0\","
    "\"mcVersion\":\"1.20.1\","
    "\"totalPlayTime\":0,"
    "\"modLoader\":\"\""
    "}");

const QByteArray kLegacyTargets = QByteArrayLiteral(
    "{"
    "\"targets\":["
    "{\"name\":\"minecraft\",\"version\":\"1.20.1\"},"
    "{\"name\":\"forge\",\"version\":\"47.2.0\"}"
    "]"
    "}");

const QByteArray kFtbVersion = QByteArrayLiteral(
    "{"
    "\"id\":10,"
    "\"parent\":5,"
    "\"name\":\"1.0.0\","
    "\"type\":\"Release\","
    "\"installs\":3,"
    "\"plays\":8,"
    "\"updated\":1700000000,"
    "\"specs\":{\"id\":1,\"minimum\":4096,\"recommended\":8192},"
    "\"targets\":[{\"id\":1,\"name\":\"minecraft\",\"type\":\"game\",\"version\":\"1.21.1\",\"updated\":1}],"
    "\"files\":[{"
    "\"id\":100,"
    "\"type\":\"mod\","
    "\"path\":\"mods/example.jar\","
    "\"name\":\"example.jar\","
    "\"version\":\"1.2.3\","
    "\"url\":\"https://cdn.example/example.jar\","
    "\"sha1\":\"abc123\","
    "\"clientonly\":false,"
    "\"serveronly\":false,"
    "\"optional\":true,"
    "\"updated\":2,"
    "\"curseforge\":{\"project\":238222,\"file\":4575702}"
    "}]"
    "}");

const QByteArray kSolderPack = QByteArrayLiteral(
    "{"
    "\"recommended\":\"1.2.0\","
    "\"latest\":\"1.3.0-beta\","
    "\"builds\":[\"1.1.0\",\"1.2.0\",\"1.3.0-beta\"]"
    "}");

const QByteArray kSolderBuild = QByteArrayLiteral(
    "{"
    "\"minecraft\":\"1.20.1\","
    "\"mods\":["
    "{\"name\":\"jei\",\"version\":\"15.2.0\",\"md5\":\"abc\",\"url\":\"https://example.test/jei.jar\"},"
    "{\"name\":\"optional-no-ver\",\"md5\":\"def\",\"url\":\"https://example.test/opt.jar\"}"
    "]"
    "}");

void writeFile(const QString& path, const QByteArray& data)
{
    FS::ensureFilePathExists(path);
    FS::write(path, data);
}
}  // namespace

class PackImportTest : public QObject {
    Q_OBJECT
   private slots:
    void test_ftbImportParsesLoaderFromInstanceJson()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        writeFile(FS::PathCombine(dir.path(), "instance.json"), kNeoForgeInstance);

        const auto pack = FTBImportAPP::parseDirectory(dir.path());
        QCOMPARE(pack.name, QStringLiteral("All the Mods"));
        QCOMPARE(pack.mcVersion, QStringLiteral("1.21.1"));
        QCOMPARE(pack.id, 42);
        QCOMPARE(pack.versionId, 99);
        QVERIFY(pack.loaderType.has_value());
        QCOMPARE(*pack.loaderType, ModPlatform::NeoForge);
        QCOMPARE(pack.loaderVersion, QStringLiteral("21.1.65"));
    }

    void test_ftbImportFallsBackToLegacyTargets()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        writeFile(FS::PathCombine(dir.path(), "instance.json"), kLegacyInstance);
        writeFile(FS::PathCombine(dir.path(), ".ftbapp", "version.json"), kLegacyTargets);

        const auto pack = FTBImportAPP::parseDirectory(dir.path());
        QVERIFY(pack.loaderType.has_value());
        QCOMPARE(*pack.loaderType, ModPlatform::Forge);
        QCOMPARE(pack.loaderVersion, QStringLiteral("47.2.0"));
    }

    void test_ftbImportRejectsMissingOrInvalidInstance()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(FTBImportAPP::parseDirectory(dir.path()).name.isEmpty());

        writeFile(FS::PathCombine(dir.path(), "instance.json"), QByteArrayLiteral("{"));
        QVERIFY(FTBImportAPP::parseDirectory(dir.path()).name.isEmpty());
    }

    void test_ftbLoadVersionKeepsOptionalCurseforgeAndPaths()
    {
        QJsonObject obj = QJsonDocument::fromJson(kFtbVersion).object();

        FTB::Version version;
        FTB::loadVersion(version, obj);
        QCOMPARE(version.id, 10);
        QCOMPARE(version.parent, 5);
        QCOMPARE(version.specs.recommended, 8192);
        QCOMPARE(version.targets.size(), 1);
        QCOMPARE(version.targets.first().version, QStringLiteral("1.21.1"));
        QCOMPARE(version.files.size(), 1);
        QCOMPARE(version.files.first().path, QStringLiteral("mods/example.jar"));
        QVERIFY(version.files.first().optional);
        QCOMPARE(version.files.first().curseforge.project_id, 238222);
        QCOMPARE(version.files.first().curseforge.file_id, 4575702);
    }

    void test_technicSolderPackAndBuild()
    {
        QJsonObject packObj = QJsonDocument::fromJson(kSolderPack).object();
        TechnicSolder::Pack pack;
        TechnicSolder::loadPack(pack, packObj);
        QCOMPARE(pack.recommended, QStringLiteral("1.2.0"));
        QCOMPARE(pack.latest, QStringLiteral("1.3.0-beta"));
        QCOMPARE(pack.builds, (QStringList{ QStringLiteral("1.1.0"), QStringLiteral("1.2.0"), QStringLiteral("1.3.0-beta") }));

        QJsonObject buildObj = QJsonDocument::fromJson(kSolderBuild).object();
        TechnicSolder::PackBuild build;
        TechnicSolder::loadPackBuild(build, buildObj);
        QCOMPARE(build.minecraft, QStringLiteral("1.20.1"));
        QCOMPARE(build.mods.size(), 2);
        QCOMPARE(build.mods.first().name, QStringLiteral("jei"));
        QCOMPARE(build.mods.first().version, QStringLiteral("15.2.0"));
        QVERIFY(build.mods.last().version.isEmpty());

        QJsonObject missingRecommended{ { "latest", QStringLiteral("1.0.0") }, { "builds", QJsonArray{} } };
        QVERIFY_THROWS_EXCEPTION(JSONValidationError, TechnicSolder::loadPack(pack, missingRecommended));
    }
};

QTEST_GUILESS_MAIN(PackImportTest)
#include "PackImport_test.moc"
