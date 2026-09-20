#include <QSet>
#include <QTest>

#include <hackclients/BaritoneMaven.h>

class BaritoneFallbackTest : public QObject {
    Q_OBJECT

   private slots:
    void test_officialArtifactMapping()
    {
        using HackClients::BaritoneMaven::officialArtifactVersion;
        using HackClients::BaritoneMaven::officialStandaloneFabricDownloadUrl;

        QCOMPARE(officialArtifactVersion(QStringLiteral("1.16.5")), QStringLiteral("1.6.5"));
        QCOMPARE(officialArtifactVersion(QStringLiteral("1.17.1")), QStringLiteral("1.7.3"));
        QCOMPARE(officialArtifactVersion(QStringLiteral("1.18.2")), QStringLiteral("1.8.5"));
        // cabaletta's 1.19.x tags are not monotonic with Minecraft versions
        QCOMPARE(officialArtifactVersion(QStringLiteral("1.19.2")), QStringLiteral("1.9.4"));
        QCOMPARE(officialArtifactVersion(QStringLiteral("1.19.3")), QStringLiteral("1.9.1"));
        QCOMPARE(officialArtifactVersion(QStringLiteral("1.19.4")), QStringLiteral("1.9.3"));
        QCOMPARE(officialArtifactVersion(QStringLiteral("1.20.1")), QStringLiteral("1.10.1"));
        QVERIFY(officialArtifactVersion(QStringLiteral("1.21.8")).isEmpty());
        QVERIFY(officialArtifactVersion(QString()).isEmpty());

        const auto url = officialStandaloneFabricDownloadUrl(QStringLiteral("1.20.1"));
        QCOMPARE(url.toString(),
                 QStringLiteral("https://github.com/cabaletta/baritone/releases/download/v1.10.1/baritone-standalone-fabric-1.10.1.jar"));
        QVERIFY(officialStandaloneFabricDownloadUrl(QStringLiteral("1.21.8")).isEmpty());
    }

    void test_officialOnlyReleasesDoNotReplaceMaven()
    {
        auto official = HackClients::BaritoneMaven::officialOnlyReleases();
        QSet<QString> versions;
        for (const auto& rel : official) {
            QVERIFY(!rel.minecraftVersion.isEmpty());
            QVERIFY(rel.mavenVersion.isEmpty());
            versions.insert(rel.minecraftVersion);
        }
        QVERIFY(versions.contains(QStringLiteral("1.16.5")));
        QVERIFY(versions.contains(QStringLiteral("1.20.1")));
        QVERIFY(!versions.contains(QStringLiteral("1.21.8")));

        QList<HackClients::BaritoneRelease> releases;
        HackClients::BaritoneRelease mavenRel;
        mavenRel.minecraftVersion = QStringLiteral("1.20.1");
        mavenRel.mavenVersion = QStringLiteral("1.20.1-SNAPSHOT");
        releases.append(mavenRel);

        HackClients::BaritoneMaven::appendOfficialOnlyReleases(releases);

        QCOMPARE(HackClients::BaritoneMaven::mavenVersionForMinecraft(releases, QStringLiteral("1.20.1")),
                 QStringLiteral("1.20.1-SNAPSHOT"));
        QCOMPARE(HackClients::BaritoneMaven::mavenVersionForMinecraft(releases, QStringLiteral("1.16.5")), QString());

        QSet<QString> after;
        for (const auto& rel : releases)
            after.insert(rel.minecraftVersion);
        QCOMPARE(after.size(), versions.size());
        QVERIFY(after.contains(QStringLiteral("1.16.5")));
        QVERIFY(after.contains(QStringLiteral("1.20.1")));
    }

    void test_formatSupportedMinecraftVersions()
    {
        using HackClients::BaritoneMaven::formatSupportedMinecraftVersions;

        QCOMPARE(formatSupportedMinecraftVersions({}), QStringLiteral("(none listed)"));

        QList<HackClients::BaritoneRelease> releases;
        for (const auto& mc : { QStringLiteral("1.16.5"), QStringLiteral("1.21.8"), QStringLiteral("1.20.1") }) {
            HackClients::BaritoneRelease rel;
            rel.minecraftVersion = mc;
            releases.append(rel);
        }

        QCOMPARE(formatSupportedMinecraftVersions(releases), QStringLiteral("1.21.8, 1.20.1, 1.16.5"));
        QCOMPARE(formatSupportedMinecraftVersions(releases, 2), QStringLiteral("1.21.8, 1.20.1, … (3 total)"));
    }
};

QTEST_GUILESS_MAIN(BaritoneFallbackTest)

#include "BaritoneFallback_test.moc"
