#include <QTest>

#include <minecraft/mod/tasks/LocalModParseTask.h>

namespace {

const QByteArray kMcmodArray = R"([
  {
    "modid": "mod_example",
    "name": "Example Mod",
    "version": "1.0",
    "url": "example.com/mod",
    "description": "desc",
    "authorList": ["Alice"],
    "requiredMods": ["mod_MinecraftForge", "mod_jei:1.0@1", "fabric-api"]
  }
])";

const QByteArray kMcmodObject = R"({
  "modListVersion": "2",
  "modList": [
    {
      "modid": "sodium",
      "name": "Sodium",
      "version": "0.5",
      "url": "https://example.test/sodium",
      "authors": ["jelly"],
      "dependencies": ["fabric-api"]
    }
  ]
})";

const QByteArray kModsToml = R"(
authors = "Dev"
license = "MIT"
[[mods]]
modId = "coolmod"
version = "1.2.3"
displayName = "Cool Mod"
description = "A cool mod"
[[dependencies.coolmod]]
modId = "forge"
mandatory = true
[[dependencies.coolmod]]
modId = "jei"
mandatory = true
[[dependencies.coolmod]]
modId = "optionalmod"
mandatory = false
[[dependencies.coolmod]]
modId = "neolib"
type = "required"
)";

const QByteArray kFabricMod = R"({
  "schemaVersion": 1,
  "id": "lithium",
  "version": "0.12.0",
  "name": "Lithium",
  "description": "optimization",
  "authors": ["jelly", {"name": "Contributor"}],
  "contact": {"homepage": "https://example.test", "issues": "https://example.test/issues"},
  "license": ["MIT", {"name": "Apache", "id": "Apache-2.0", "url": "https://apache.org", "description": "asl"}],
  "icon": {"16x16": "small.png", "64x64": "large.png"},
  "depends": {"fabricloader": "*", "minecraft": "1.21", "fabric-api": "*", "sodium": "*"}
})";

const QByteArray kQuiltMod = R"({
  "schema_version": 1,
  "quilt_loader": {
    "id": "qsl",
    "version": "6.0.0",
    "metadata": {
      "name": "Quilt Standard Libraries",
      "description": "libraries",
      "contributors": {"Quilt": "Owner", "Alice": "Author"},
      "contact": {"homepage": "https://quiltmc.org", "issues": "https://quiltmc.org/issues"},
      "license": "MPL-2.0",
      "icon": {"32x32": "icon32.png", "128x128": "icon128.png"}
    }
  },
  "depends": [
    "minecraft",
    "quilt_loader",
    "sodium",
    {"id": "optional-thing", "optional": true},
    {"id": "required-thing"}
  ]
})";

const QByteArray kForgeIni = R"(
forge.major.number=14
forge.minor.number=23
forge.revision.number=5
forge.build.number=2854
)";

const QByteArray kLitemod = R"({
  "name": "MacroMod",
  "revision": "0.15",
  "mcversion": "1.8.9",
  "author": "mumfrey",
  "description": "macros",
  "url": "https://example.test/litemod"
})";

const QByteArray kNilMod = R"(
@nilmod {
  name: "Nil Example";
  description: "css metadata";
  authors: "unascribed";
  version: "0.1";
}
)";

}  // namespace

class LocalModParseTest : public QObject {
    Q_OBJECT
   private slots:
    void test_mcmodInfoArrayStripsForgeAndExampleName()
    {
        const auto details = ModUtils::ReadMCModInfo(kMcmodArray);
        QCOMPARE(details.mod_id, QStringLiteral("example"));
        QVERIFY(details.name.isEmpty());
        QCOMPARE(details.homeurl, QStringLiteral("http://example.com/mod"));
        QCOMPARE(details.authors, QStringList{ QStringLiteral("Alice") });
        QCOMPARE(details.dependencies, QStringList{ QStringLiteral("jei"), QStringLiteral("fabric-api") });
    }

    void test_mcmodInfoObjectForm()
    {
        const auto details = ModUtils::ReadMCModInfo(kMcmodObject);
        QCOMPARE(details.mod_id, QStringLiteral("sodium"));
        QCOMPARE(details.name, QStringLiteral("Sodium"));
        QCOMPARE(details.homeurl, QStringLiteral("https://example.test/sodium"));
        QCOMPARE(details.authors, QStringList{ QStringLiteral("jelly") });
        QCOMPARE(details.dependencies, QStringList{ QStringLiteral("fabric-api") });
    }

    void test_mcmodInfoRejectsGarbage()
    {
        const auto details = ModUtils::ReadMCModInfo(QByteArrayLiteral("not json"));
        QVERIFY(details.mod_id.isEmpty());
    }

    void test_modsTomlRequiredDependencies()
    {
        const auto details = ModUtils::ReadMCModTOML(kModsToml);
        QCOMPARE(details.mod_id, QStringLiteral("coolmod"));
        QCOMPARE(details.name, QStringLiteral("Cool Mod"));
        QCOMPARE(details.version, QStringLiteral("1.2.3"));
        QCOMPARE(details.authors, QStringList{ QStringLiteral("Dev") });
        QVERIFY(details.licenses.size() >= 1);
        QVERIFY(details.dependencies.contains(QStringLiteral("jei")));
        QVERIFY(details.dependencies.contains(QStringLiteral("neolib")));
        QVERIFY(!details.dependencies.contains(QStringLiteral("forge")));
        QVERIFY(!details.dependencies.contains(QStringLiteral("optionalmod")));
    }

    void test_modsTomlRejectsMissingModsTable()
    {
        const auto details = ModUtils::ReadMCModTOML(QByteArrayLiteral("license = \"MIT\"\n"));
        QVERIFY(details.mod_id.isEmpty());
    }

    void test_fabricModInfo()
    {
        const auto details = ModUtils::ReadFabricModInfo(kFabricMod);
        QCOMPARE(details.mod_id, QStringLiteral("lithium"));
        QCOMPARE(details.name, QStringLiteral("Lithium"));
        QCOMPARE(details.authors, (QStringList{ QStringLiteral("jelly"), QStringLiteral("Contributor") }));
        QCOMPARE(details.homeurl, QStringLiteral("https://example.test"));
        QCOMPARE(details.issue_tracker, QStringLiteral("https://example.test/issues"));
        QCOMPARE(details.icon_file, QStringLiteral("large.png"));
        QCOMPARE(details.dependencies, QStringList{ QStringLiteral("sodium") });
        QCOMPARE(details.licenses.size(), 2);
    }

    void test_fabricSchemaZeroSkipsDepends()
    {
        const auto details = ModUtils::ReadFabricModInfo(QByteArrayLiteral(R"({"id":"x","version":"1","depends":{"sodium":"*"}})"));
        QCOMPARE(details.mod_id, QStringLiteral("x"));
        QVERIFY(details.dependencies.isEmpty());
        QCOMPARE(details.name, QStringLiteral("x"));
    }

    void test_quiltModInfo()
    {
        const auto details = ModUtils::ReadQuiltModInfo(kQuiltMod);
        QCOMPARE(details.mod_id, QStringLiteral("qsl"));
        QCOMPARE(details.name, QStringLiteral("Quilt Standard Libraries"));
        QVERIFY(details.authors.contains(QStringLiteral("Quilt")));
        QVERIFY(details.authors.contains(QStringLiteral("Alice")));
        QCOMPARE(details.icon_file, QStringLiteral("icon128.png"));
        QVERIFY(details.dependencies.contains(QStringLiteral("sodium")));
        QVERIFY(details.dependencies.contains(QStringLiteral("required-thing")));
        QVERIFY(!details.dependencies.contains(QStringLiteral("minecraft")));
        QVERIFY(!details.dependencies.contains(QStringLiteral("quilt_loader")));
        QVERIFY(!details.dependencies.contains(QStringLiteral("optional-thing")));
    }

    void test_quiltMissingLoaderReturnsEmpty()
    {
        const auto details = ModUtils::ReadQuiltModInfo(QByteArrayLiteral("{}"));
        QVERIFY(details.mod_id.isEmpty());
    }

    void test_forgeInfoVersion()
    {
        const auto details = ModUtils::ReadForgeInfo(kForgeIni);
        QCOMPARE(details.mod_id, QStringLiteral("Forge"));
        QCOMPARE(details.version, QStringLiteral("14.23.5.2854"));
    }

    void test_litemodUsesRevisionFallback()
    {
        const auto details = ModUtils::ReadLiteModInfo(kLitemod);
        QCOMPARE(details.name, QStringLiteral("MacroMod"));
        QCOMPARE(details.version, QStringLiteral("0.15"));
        QCOMPARE(details.mcversion, QStringLiteral("1.8.9"));
        QCOMPARE(details.authors, QStringList{ QStringLiteral("mumfrey") });
    }

    void test_nilModCss()
    {
        const auto details = ModUtils::ReadNilModInfo(kNilMod, QStringLiteral("example.nilmod.css"));
        QCOMPARE(details.name, QStringLiteral("Nil Example"));
        QCOMPARE(details.description, QStringLiteral("css metadata"));
        QCOMPARE(details.authors, QStringList{ QStringLiteral("unascribed") });
        QCOMPARE(details.version, QStringLiteral("0.1"));
        QCOMPARE(details.mod_id, QStringLiteral("example"));
    }
};

QTEST_GUILESS_MAIN(LocalModParseTest)
#include "LocalModParse_test.moc"
