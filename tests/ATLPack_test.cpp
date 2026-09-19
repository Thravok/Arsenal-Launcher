#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QTemporaryDir>
#include <QTest>

#include <FileSystem.h>
#include <modplatform/atlauncher/ATLPackManifest.h>
#include <modplatform/atlauncher/ATLShareCode.h>

class ATLPackTest : public QObject {
    Q_OBJECT

    static QJsonObject sampleMod(const QString& type = QStringLiteral("mods"))
    {
        QJsonObject mod;
        mod.insert(QStringLiteral("name"), QStringLiteral("Example Mod"));
        mod.insert(QStringLiteral("version"), QStringLiteral("1.0"));
        mod.insert(QStringLiteral("url"), QStringLiteral("https://ex.test/mod.jar"));
        mod.insert(QStringLiteral("file"), QStringLiteral("example.jar"));
        mod.insert(QStringLiteral("download"), QStringLiteral("server"));
        mod.insert(QStringLiteral("type"), type);
        return mod;
    }

   private slots:
    void isPathTraversalAllowsSameAndChildPaths()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString base = dir.path();
        QVERIFY(QDir().mkpath(FS::PathCombine(base, QStringLiteral("child"))));

        QVERIFY(!ATLauncher::isPathTraversal(base, QString()));
        QVERIFY(!ATLauncher::isPathTraversal(base, QStringLiteral(".")));
        QVERIFY(!ATLauncher::isPathTraversal(base, QStringLiteral("child")));
        QVERIFY(!ATLauncher::isPathTraversal(base, QStringLiteral("child/nested")));
    }

    void isPathTraversalBlocksParentEscape()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString base = FS::PathCombine(dir.path(), QStringLiteral("minecraft"));
        QVERIFY(QDir().mkpath(base));

        QVERIFY(ATLauncher::isPathTraversal(base, QStringLiteral("..")));
        QVERIFY(ATLauncher::isPathTraversal(base, QStringLiteral("../secret")));
        QVERIFY(ATLauncher::isPathTraversal(base, QStringLiteral("../../outside")));
        QVERIFY(ATLauncher::isPathTraversal(base, QStringLiteral("child/../../outside")));
    }

    void loadVersionParsesExtractAndForgeJarAlias()
    {
        QJsonObject loaderMeta;
        loaderMeta.insert(QStringLiteral("latest"), false);
        loaderMeta.insert(QStringLiteral("recommended"), true);
        loaderMeta.insert(QStringLiteral("version"), QStringLiteral("36.2.39"));
        QJsonObject loader;
        loader.insert(QStringLiteral("type"), QStringLiteral("forge"));
        loader.insert(QStringLiteral("choose"), false);
        loader.insert(QStringLiteral("metadata"), loaderMeta);

        QJsonObject forge;
        forge.insert(QStringLiteral("name"), QStringLiteral("Minecraft Forge"));
        forge.insert(QStringLiteral("version"), QStringLiteral("36.2.39"));
        forge.insert(QStringLiteral("url"), QStringLiteral("https://ex.test/forge.jar"));
        forge.insert(QStringLiteral("file"), QStringLiteral("forge.jar"));
        forge.insert(QStringLiteral("download"), QStringLiteral("browser"));
        forge.insert(QStringLiteral("type"), QStringLiteral("jar"));

        QJsonObject extracted;
        extracted.insert(QStringLiteral("name"), QStringLiteral("Extracted Configs"));
        extracted.insert(QStringLiteral("version"), QStringLiteral("1"));
        extracted.insert(QStringLiteral("url"), QStringLiteral("https://ex.test/configs.zip"));
        extracted.insert(QStringLiteral("file"), QStringLiteral("configs.zip"));
        extracted.insert(QStringLiteral("download"), QStringLiteral("direct"));
        extracted.insert(QStringLiteral("type"), QStringLiteral("extract"));
        extracted.insert(QStringLiteral("extractTo"), QStringLiteral("root"));
        extracted.insert(QStringLiteral("extractFolder"), QStringLiteral("%s%config"));
        extracted.insert(QStringLiteral("optional"), true);
        extracted.insert(QStringLiteral("library"), true);

        QJsonObject dependency;
        dependency.insert(QStringLiteral("name"), QStringLiteral("Legacy Dep"));
        dependency.insert(QStringLiteral("version"), QStringLiteral("2"));
        dependency.insert(QStringLiteral("url"), QStringLiteral("https://ex.test/dep.jar"));
        dependency.insert(QStringLiteral("file"), QStringLiteral("dep.jar"));
        dependency.insert(QStringLiteral("download"), QStringLiteral("server"));
        dependency.insert(QStringLiteral("type"), QStringLiteral("depandency"));
        dependency.insert(QStringLiteral("decompType"), QStringLiteral("mods"));
        dependency.insert(QStringLiteral("decompFile"), QStringLiteral("../escape.jar"));
        dependency.insert(QStringLiteral("hidden"), true);

        QJsonObject root;
        root.insert(QStringLiteral("version"), QStringLiteral("1.2"));
        root.insert(QStringLiteral("minecraft"), QStringLiteral("1.16.5"));
        root.insert(QStringLiteral("noConfigs"), true);
        root.insert(QStringLiteral("loader"), loader);
        root.insert(QStringLiteral("mods"), QJsonArray{ forge, extracted, dependency });

        ATLauncher::PackVersion version;
        ATLauncher::loadVersion(version, root);
        QCOMPARE(version.version, QStringLiteral("1.2"));
        QCOMPARE(version.minecraft, QStringLiteral("1.16.5"));
        QVERIFY(version.noConfigs);
        QCOMPARE(version.loader.type, QStringLiteral("forge"));
        QCOMPARE(version.loader.version, QStringLiteral("36.2.39"));
        QVERIFY(version.loader.recommended);
        QCOMPARE(version.mods.size(), 3);

        QCOMPARE(version.mods[0].type, ATLauncher::ModType::Forge);
        QCOMPARE(version.mods[0].type_raw, QStringLiteral("forge"));
        QCOMPARE(version.mods[0].download, ATLauncher::DownloadType::Browser);

        QCOMPARE(version.mods[1].type, ATLauncher::ModType::Extract);
        QCOMPARE(version.mods[1].extractTo, ATLauncher::ModType::Root);
        QCOMPARE(version.mods[1].extractFolder, QStringLiteral("/config"));
        QVERIFY(version.mods[1].optional);
        QVERIFY(version.mods[1].effectively_hidden);

        QCOMPARE(version.mods[2].type, ATLauncher::ModType::Dependency);
        QCOMPARE(version.mods[2].decompType, ATLauncher::ModType::Mods);
        QCOMPARE(version.mods[2].decompFile, QStringLiteral("../escape.jar"));
        QVERIFY(version.mods[2].effectively_hidden);
    }

    void loadVersionUnknownDownloadAndType()
    {
        auto mod = sampleMod(QStringLiteral("not-a-type"));
        mod.insert(QStringLiteral("download"), QStringLiteral("ftp"));

        QJsonObject root;
        root.insert(QStringLiteral("version"), QStringLiteral("x"));
        root.insert(QStringLiteral("minecraft"), QStringLiteral("1.8.9"));
        root.insert(QStringLiteral("mods"), QJsonArray{ mod });

        ATLauncher::PackVersion version;
        ATLauncher::loadVersion(version, root);
        QCOMPARE(version.mods.size(), 1);
        QCOMPARE(version.mods[0].type, ATLauncher::ModType::Unknown);
        QCOMPARE(version.mods[0].download, ATLauncher::DownloadType::Unknown);
    }

    void loadShareCodeResponseSuccessAndError()
    {
        QJsonObject optifine;
        optifine.insert(QStringLiteral("selected"), true);
        optifine.insert(QStringLiteral("name"), QStringLiteral("OptiFine"));
        QJsonObject journeymap;
        journeymap.insert(QStringLiteral("selected"), false);
        journeymap.insert(QStringLiteral("name"), QStringLiteral("JourneyMap"));

        QJsonObject mods;
        mods.insert(QStringLiteral("optional"), QJsonArray{ optifine, journeymap });
        QJsonObject data;
        data.insert(QStringLiteral("pack"), QStringLiteral("SkyFactory"));
        data.insert(QStringLiteral("version"), QStringLiteral("4.2.4"));
        data.insert(QStringLiteral("mods"), mods);

        QJsonObject ok;
        ok.insert(QStringLiteral("error"), false);
        ok.insert(QStringLiteral("code"), 200);
        ok.insert(QStringLiteral("data"), data);

        ATLauncher::ShareCodeResponse response;
        ATLauncher::loadShareCodeResponse(response, ok);
        QVERIFY(!response.error);
        QCOMPARE(response.code, 200);
        QCOMPARE(response.data.pack, QStringLiteral("SkyFactory"));
        QCOMPARE(response.data.version, QStringLiteral("4.2.4"));
        QCOMPARE(response.data.mods.size(), 2);
        QVERIFY(response.data.mods[0].selected);
        QCOMPARE(response.data.mods[1].name, QStringLiteral("JourneyMap"));

        QJsonObject err;
        err.insert(QStringLiteral("error"), true);
        err.insert(QStringLiteral("code"), 404);
        err.insert(QStringLiteral("message"), QStringLiteral("Share code not found"));

        ATLauncher::ShareCodeResponse failed;
        ATLauncher::loadShareCodeResponse(failed, err);
        QVERIFY(failed.error);
        QCOMPARE(failed.code, 404);
        QCOMPARE(failed.message, QStringLiteral("Share code not found"));
        QVERIFY(failed.data.pack.isEmpty());
    }
};

QTEST_GUILESS_MAIN(ATLPackTest)
#include "ATLPack_test.moc"
