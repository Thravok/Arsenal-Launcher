#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

#include <FileSystem.h>
#include <hackclients/ImpactInstallTask.h>
#include <hackclients/LiquidBounceInstallTask.h>

class HackClientInstallPathsTest : public QObject {
    Q_OBJECT
   private slots:
    void test_findInstalledImpactInstancePrefersInstancesThenMacBundle()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());

        QString found;
        QVERIFY(!HackClients::findInstalledImpactInstance(root.path(), found));

        const QString instances = FS::PathCombine(root.path(), "instances", "Impact");
        QVERIFY(FS::ensureFolderPathExists(instances));
        {
            QFile pack(FS::PathCombine(instances, "mmc-pack.json"));
            QVERIFY(pack.open(QIODevice::WriteOnly));
            pack.write("{}");
        }
        QVERIFY(HackClients::findInstalledImpactInstance(root.path(), found));
        QCOMPARE(QFileInfo(found).canonicalFilePath(), QFileInfo(instances).canonicalFilePath());

        QTemporaryDir macRoot;
        const QString macos = FS::PathCombine(macRoot.path(), "Contents", "MacOS", "instances", "Impact");
        QVERIFY(FS::ensureFolderPathExists(macos));
        {
            QFile pack(FS::PathCombine(macos, "mmc-pack.json"));
            QVERIFY(pack.open(QIODevice::WriteOnly));
            pack.write("{}");
        }
        QVERIFY(HackClients::findInstalledImpactInstance(macRoot.path(), found));
        QCOMPARE(QFileInfo(found).canonicalFilePath(), QFileInfo(macos).canonicalFilePath());

        QTemporaryDir resourcesRoot;
        const QString resources = FS::PathCombine(resourcesRoot.path(), "Contents", "Resources", "instances", "Impact");
        QVERIFY(FS::ensureFolderPathExists(resources));
        {
            QFile pack(FS::PathCombine(resources, "mmc-pack.json"));
            QVERIFY(pack.open(QIODevice::WriteOnly));
            pack.write("{}");
        }
        QVERIFY(HackClients::findInstalledImpactInstance(resourcesRoot.path(), found));
        QCOMPARE(QFileInfo(found).canonicalFilePath(), QFileInfo(resources).canonicalFilePath());
    }

    void test_findInstalledImpactInstanceIgnoresFoldersWithoutPack()
    {
        QTemporaryDir root;
        const QString decoy = FS::PathCombine(root.path(), "instances", "empty");
        QVERIFY(FS::ensureFolderPathExists(decoy));
        QString found;
        QVERIFY(!HackClients::findInstalledImpactInstance(root.path(), found));
    }

    void test_liquidBounceCompanionDownloadsPercentEncodePlus()
    {
        HackClients::LiquidBounceBuild empty;
        QVERIFY(HackClients::liquidBounceCompanionDownloads(empty).isEmpty());

        HackClients::LiquidBounceBuild build;
        build.fabricApiVersion = QStringLiteral("0.110.0+1.21.4");
        build.kotlinModVersion = QStringLiteral("1.13.0+kotlin.2.1.0");
        const auto mods = HackClients::liquidBounceCompanionDownloads(build);
        QCOMPARE(mods.size(), 2);

        QCOMPARE(mods[0].filename, QStringLiteral("fabric-api-0.110.0+1.21.4.jar"));
        QVERIFY(mods[0].url.toString().contains(QStringLiteral("0.110.0%2B1.21.4")));
        QVERIFY(!mods[0].url.toString().contains(QStringLiteral("0.110.0+1.21.4")));
        QVERIFY(mods[0].url.isValid());

        QCOMPARE(mods[1].filename, QStringLiteral("fabric-language-kotlin-1.13.0+kotlin.2.1.0.jar"));
        QVERIFY(mods[1].url.toString().contains(QStringLiteral("%2B")));
        QVERIFY(mods[1].url.isValid());
    }
};

QTEST_GUILESS_MAIN(HackClientInstallPathsTest)
#include "HackClientInstallPaths_test.moc"
