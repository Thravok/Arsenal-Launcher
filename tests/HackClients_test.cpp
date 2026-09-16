#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include <hackclients/BaritoneMaven.h>
#include <hackclients/FDPProvider.h>
#include <hackclients/HackClientInstanceDetect.h>
#include <hackclients/HackClientTypes.h>
#include <hackclients/ImpactReleasesProvider.h>
#include <hackclients/LambdaProvider.h>
#include <hackclients/LiquidBounceProvider.h>
#include <hackclients/MeteorAddonsProvider.h>
#include <hackclients/MeteorProvider.h>
#include <hackclients/WurstProvider.h>

class HackClientsTest : public QObject {
    Q_OBJECT
   private slots:
    void test_instanceVersionLabels()
    {
        HackClients::ImpactRelease impact;
        impact.tagName = QStringLiteral("4.9.1-1.16.5");
        impact.impactVersion = QStringLiteral("4.9.1");
        impact.minecraftVersion = QStringLiteral("1.16.5");
        QCOMPARE(impact.instanceVersionLabel(), QStringLiteral("4.9.1 (1.16.5)"));
        impact.impactVersion.clear();
        QCOMPARE(impact.instanceVersionLabel(), QStringLiteral("4.9.1-1.16.5"));

        HackClients::MeteorBuild meteor;
        meteor.minecraftVersion = QStringLiteral("1.21.8");
        meteor.buildNumber = 42;
        QCOMPARE(meteor.instanceVersionLabel(), QStringLiteral("1.21.8 · #42"));
        meteor.buildNumber = 0;
        QCOMPARE(meteor.instanceVersionLabel(), QStringLiteral("1.21.8"));

        HackClients::WurstRelease wurst;
        wurst.tagName = QStringLiteral("v7.55.1-MC26.2");
        wurst.wurstVersion = QStringLiteral("v7.55.1");
        wurst.minecraftVersion = QStringLiteral("26.2");
        QCOMPARE(wurst.instanceVersionLabel(), QStringLiteral("v7.55.1 (26.2)"));
    }

    void test_meteorAddonSupportsAndJarPick()
    {
        HackClients::MeteorAddonEntry entry;
        entry.minecraftVersion = QStringLiteral("1.21.8");
        entry.supportedVersions = { QStringLiteral("1.21.7"), QStringLiteral("1.21.8") };
        QVERIFY(entry.supportsMinecraft({}));
        QVERIFY(entry.supportsMinecraft(QStringLiteral("1.21.8")));
        QVERIFY(entry.supportsMinecraft(QStringLiteral("1.21.7")));
        QVERIFY(!entry.supportsMinecraft(QStringLiteral("1.20.1")));

        entry.latestReleaseUrl = QStringLiteral("https://example.test/addon-dev.jar");
        entry.downloadUrls = { QStringLiteral("https://example.test/addon-sources.jar"), QStringLiteral("https://example.test/addon.jar") };
        QCOMPARE(entry.pickJarDownloadUrl(), QStringLiteral("https://example.test/addon.jar"));

        entry.latestReleaseUrl = QStringLiteral("https://example.test/addon.jar");
        QCOMPARE(entry.pickJarDownloadUrl(), QStringLiteral("https://example.test/addon.jar"));

        entry.latestReleaseUrl.clear();
        entry.downloadUrls = { QStringLiteral("https://example.test/addon-sources.jar") };
        QVERIFY(entry.pickJarDownloadUrl().isEmpty());
    }

    void test_modsFolderHasMeteorClient()
    {
        QVERIFY(!HackClients::modsFolderHasMeteorClient(QStringLiteral("/no/such/mods")));

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(!HackClients::modsFolderHasMeteorClient(dir.path()));

        {
            QFile other(QDir(dir.path()).filePath("sodium.jar"));
            QVERIFY(other.open(QIODevice::WriteOnly));
            other.write("x");
        }
        QVERIFY(!HackClients::modsFolderHasMeteorClient(dir.path()));

        {
            QFile meteor(QDir(dir.path()).filePath("Meteor-Client-0.5.9.jar"));
            QVERIFY(meteor.open(QIODevice::WriteOnly));
            meteor.write("x");
        }
        QVERIFY(HackClients::modsFolderHasMeteorClient(dir.path()));

        QTemporaryDir underscoreDir;
        {
            QFile meteor(QDir(underscoreDir.path()).filePath("meteor_client-nightly.JAR"));
            QVERIFY(meteor.open(QIODevice::WriteOnly));
            meteor.write("x");
        }
        QVERIFY(HackClients::modsFolderHasMeteorClient(underscoreDir.path()));
    }

    void test_baritoneMavenIndexPrefersSnapshot()
    {
        const QByteArray html = R"(
<a href="./maven-metadata.xml">meta</a>
<a href="./1.20.1/">1.20.1</a>
<a href="./1.21.11-SNAPSHOT/">snap</a>
<a href="./1.21.11/">release</a>
<a href="./1.19.4/">old</a>
)";
        const auto releases = HackClients::BaritoneMaven::parseVersionIndex(html);
        QCOMPARE(HackClients::BaritoneMaven::mavenVersionForMinecraft(releases, QStringLiteral("1.21.11")),
                 QStringLiteral("1.21.11-SNAPSHOT"));
        QCOMPARE(HackClients::BaritoneMaven::mavenVersionForMinecraft(releases, QStringLiteral("1.20.1")), QStringLiteral("1.20.1"));
        QVERIFY(HackClients::BaritoneMaven::mavenVersionForMinecraft(releases, QStringLiteral("1.16.5")).isEmpty());

        auto withOfficial = releases;
        HackClients::BaritoneMaven::appendOfficialOnlyReleases(withOfficial);
        bool foundOfficialOnly = false;
        for (const auto& rel : withOfficial) {
            if (rel.minecraftVersion == QStringLiteral("1.16.5")) {
                foundOfficialOnly = true;
                QVERIFY(rel.mavenVersion.isEmpty());
            }
        }
        QVERIFY(foundOfficialOnly);

        const QString formatted = HackClients::BaritoneMaven::formatSupportedMinecraftVersions(releases, 2);
        QVERIFY(formatted.contains(QStringLiteral("1.21.11")));
        QVERIFY(formatted.contains(QStringLiteral("total")));
        QCOMPARE(HackClients::BaritoneMaven::formatSupportedMinecraftVersions({}), QStringLiteral("(none listed)"));
    }

    void test_meteorProviderParse()
    {
        HackClients::MeteorProvider provider(nullptr);
        QVERIFY(!provider.parse(QByteArrayLiteral("not json")));
        QVERIFY(!provider.parse(QByteArrayLiteral("[]")));
        QVERIFY(!provider.parse(QByteArrayLiteral(R"({"builds":{}})")));
        QVERIFY(provider.parse(QByteArrayLiteral(R"({"builds":{"1.20.1":10,"1.21.8":42,"bad":0}})")));
        QCOMPARE(provider.builds().size(), 2);
        QCOMPARE(provider.buildForMinecraft(QStringLiteral("1.21.8")).buildNumber, 42);
        QVERIFY(provider.buildForMinecraft(QStringLiteral("1.19.2")).minecraftVersion.isEmpty());
        QCOMPARE(provider.builds().first().minecraftVersion, QStringLiteral("1.21.8"));
    }

    void test_meteorAddonsProviderParse()
    {
        HackClients::MeteorAddonsProvider provider(nullptr);
        QVERIFY(!provider.parse(QByteArrayLiteral("{}")));
        QVERIFY(provider.parse(QByteArrayLiteral("[]")));
        QVERIFY(provider.addons().isEmpty());

        const QByteArray json = R"([
          {"name":"Zebra","mc_version":"1.20.1","verified":false,"links":{"downloads":["https://ex.test/z.jar"]}},
          {"name":"","mc_version":"1.21.8","links":{"latest_release":"https://ex.test/empty.jar"}},
          {"name":"Alpha","mc_version":"1.21.8","verified":true,"custom":{"supported_versions":["1.21.7"]},
           "links":{"latest_release":"https://ex.test/alpha-dev.jar","downloads":["https://ex.test/alpha.jar"]}},
          {"name":"NoFiles","mc_version":"1.21.8"}
        ])";
        QVERIFY(provider.parse(json));
        QCOMPARE(provider.addons().size(), 2);
        QCOMPARE(provider.addons().first().name, QStringLiteral("Alpha"));
        QVERIFY(provider.addons().first().verified);
        QCOMPARE(provider.addonsForMinecraft(QStringLiteral("1.21.8")).size(), 1);
        QCOMPARE(provider.addonsForMinecraft(QStringLiteral("1.21.8")).first().pickJarDownloadUrl(),
                 QStringLiteral("https://ex.test/alpha.jar"));
        QCOMPARE(provider.addonsForMinecraft(QStringLiteral("1.21.8"), true).size(), 2);
    }

    void test_wurstProviderParse()
    {
        QVERIFY(HackClients::WurstProvider::isUnstableMinecraftVersion(QStringLiteral("1.21.4-pre1")));
        QVERIFY(HackClients::WurstProvider::isUnstableMinecraftVersion(QStringLiteral("1.21-rc1")));
        QVERIFY(HackClients::WurstProvider::isUnstableMinecraftVersion(QStringLiteral("24w10a snapshot")));
        QVERIFY(!HackClients::WurstProvider::isUnstableMinecraftVersion(QStringLiteral("1.21.4")));

        HackClients::WurstProvider provider(nullptr);
        QVERIFY(!provider.parse(QByteArrayLiteral("{}")));
        const QByteArray json = R"([
          {"assets":[
            {"name":"Wurst-Client-v7.55.1-MC1.21.4.jar","browser_download_url":"https://ex.test/w1.jar"},
            {"name":"Wurst-Client-v7.54.0-MC1.21.4.jar","browser_download_url":"https://ex.test/w0.jar"},
            {"name":"Wurst-Client-v7.55.1-MC1.21.4-pre1.jar","browser_download_url":"https://ex.test/pre.jar"},
            {"name":"Wurst-Client-v7.55.1-MC1.21.4-sources.jar","browser_download_url":"https://ex.test/src.jar"},
            {"name":"readme.txt","browser_download_url":"https://ex.test/readme"}
          ]}
        ])";
        QVERIFY(provider.parse(json));
        QCOMPARE(provider.releases(true).size(), 3);
        QCOMPARE(provider.releases(false).size(), 2);
        const auto latest = provider.latestStablePerMinecraft();
        QCOMPARE(latest.size(), 1);
        QCOMPARE(latest.first().wurstVersion, QStringLiteral("v7.55.1"));
        QCOMPARE(provider.releaseForTag(QStringLiteral("v7.54.0-MC1.21.4")).jarUrl, QStringLiteral("https://ex.test/w0.jar"));
    }

    void test_impactProviderParse()
    {
        HackClients::ImpactReleasesProvider provider(nullptr);
        QVERIFY(!provider.parse(QByteArrayLiteral("{}")));
        const QByteArray json = R"([
          {"tag_name":"4.9.1-1.16.5","prerelease":false},
          {"tag_name":"4.8.0-1.16.5","prerelease":false},
          {"tag_name":"5.0.0-1.12.2","prerelease":true},
          {"tag_name":"nightly","prerelease":false}
        ])";
        QVERIFY(provider.parse(json));
        QCOMPARE(provider.releases(true).size(), 3);
        QCOMPARE(provider.releases(false).size(), 2);
        QCOMPARE(provider.latestStableForMinecraft(QStringLiteral("1.16.5")).impactVersion, QStringLiteral("4.9.1"));
        QCOMPARE(provider.latestStablePerMinecraft().first().minecraftVersion, QStringLiteral("1.16.5"));
        QVERIFY(provider.latestStableForMinecraft(QStringLiteral("1.12.2")).tagName.isEmpty());
    }

    void test_lambdaProviderParse()
    {
        HackClients::LambdaProvider provider(nullptr);
        const QByteArray json = R"([
          {"tag_name":"0.2.0+1.21.11","prerelease":false,"assets":[
            {"name":"lambda-0.2.0.jar","browser_download_url":"https://ex.test/l2.jar"}
          ]},
          {"tag_name":"0.1.0+1.21.11","prerelease":false,"assets":[
            {"name":"lambda-0.1.0.jar","browser_download_url":"https://ex.test/l1.jar"}
          ]},
          {"tag_name":"0.3.0-beta","prerelease":false,"assets":[
            {"name":"lambda-0.3.0.jar","browser_download_url":"https://ex.test/l3.jar"}
          ]},
          {"tag_name":"0.4.0+1.20.1","prerelease":true,"assets":[
            {"name":"lambda-0.4.0.jar","browser_download_url":"https://ex.test/l4.jar"}
          ]}
        ])";
        QVERIFY(provider.parse(json));
        QCOMPARE(provider.releases(true).size(), 3);
        QCOMPARE(provider.releases(false).size(), 2);
        const auto latest = provider.latestStablePerMinecraft();
        QCOMPARE(latest.size(), 1);
        QCOMPARE(latest.first().lambdaVersion, QStringLiteral("0.2.0"));
        QCOMPARE(provider.releaseForTag(QStringLiteral("0.1.0+1.21.11")).jarUrl, QStringLiteral("https://ex.test/l1.jar"));
    }

    void test_fdpProviderParse()
    {
        HackClients::FDPProvider provider(nullptr);
        const QByteArray json = R"([
          {"tag_name":"b18","prerelease":true,"assets":[
            {"name":"FDPClient-b18.jar","browser_download_url":"https://ex.test/b18.jar"}
          ]},
          {"tag_name":"b17","prerelease":false,"assets":[
            {"name":"FDPClient-b17.jar","browser_download_url":"https://ex.test/b17.jar"}
          ]},
          {"tag_name":"notes","prerelease":false,"assets":[
            {"name":"changelog.txt","browser_download_url":"https://ex.test/notes"}
          ]}
        ])";
        QVERIFY(provider.parse(json));
        QCOMPARE(provider.releases(true).size(), 2);
        QCOMPARE(provider.latestStable().tagName, QStringLiteral("b17"));
        QCOMPARE(provider.releaseForTag(QStringLiteral("b18")).jarUrl, QStringLiteral("https://ex.test/b18.jar"));
    }

    void test_liquidBounceParseHelpers()
    {
        HackClients::LiquidBounceProvider provider(nullptr);
        QVERIFY(!provider.parseDownloadPage(QByteArrayLiteral("<html>no builds</html>")));
        QVERIFY(provider.parseDownloadPage(QByteArrayLiteral("see /download/12 and /download/44 and /download/12")));
        QCOMPARE(provider.candidateBuildIds(), (QList<int>{ 44, 12 }));

        HackClients::LiquidBounceBuild build;
        QVERIFY(!provider.parseBuildJson(QByteArrayLiteral("[]"), build));
        QVERIFY(provider.parseBuildJson(QByteArrayLiteral(R"({
          "build_id": 44,
          "lb_version": "0.30.0",
          "mc_version": "1.21.4",
          "release": true,
          "fabric_loader_version": "0.16.9",
          "url": "https://liquidbounce.net/download/44"
        })"),
                                        build));
        QCOMPARE(build.buildId, 44);
        QCOMPARE(build.mcVersion, QStringLiteral("1.21.4"));
        QVERIFY(build.release);

        HackClients::LiquidBounceBuild queued;
        QVERIFY(!provider.parseQueuePage(QByteArrayLiteral("no pid here"), queued));
        QVERIFY(provider.parseQueuePage(
            QByteArrayLiteral(
                R"("pid","AbC123","file_name","lb.zip","file_path","x","file_size",1,"file_checksum","aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa")"),
            queued));
        QCOMPARE(queued.filePid, QStringLiteral("AbC123"));
        QCOMPARE(queued.fileName, QStringLiteral("lb.zip"));
        QCOMPARE(queued.fileChecksumSha256.size(), 64);

        HackClients::LiquidBounceBuild escaped;
        QVERIFY(provider.parseQueuePage(QByteArrayLiteral(R"(\"pid\",\"Zz9\",\"file_name\",\"client.zip\")"), escaped));
        QCOMPARE(escaped.filePid, QStringLiteral("Zz9"));
        QCOMPARE(escaped.fileName, QStringLiteral("client.zip"));
    }
};

QTEST_GUILESS_MAIN(HackClientsTest)
#include "HackClients_test.moc"
