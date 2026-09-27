#include <QTest>

#include <minecraft/ParseUtils.h>
#include <minecraft/VersionFilterData.h>

class VersionFilterTest : public QObject {
    Q_OBJECT
   private slots:
    void test_forgeInstallerBlacklist()
    {
        QVERIFY(g_VersionFilterData.forgeInstallerBlacklist.contains(QStringLiteral("1.5.2")));
        QVERIFY(!g_VersionFilterData.forgeInstallerBlacklist.contains(QStringLiteral("1.7.10")));
        QVERIFY(!g_VersionFilterData.forgeInstallerBlacklist.contains(QStringLiteral("1.20.1")));
    }

    void test_fmlLibraryChecksums()
    {
        QVERIFY(g_VersionFilterData.fmlLibsMapping.contains(QStringLiteral("1.3.2")));
        QCOMPARE(g_VersionFilterData.fmlLibsMapping.value(QStringLiteral("1.3.2")).size(), 3);

        const auto libs152 = g_VersionFilterData.fmlLibsMapping.value(QStringLiteral("1.5.2"));
        QVERIFY(!libs152.isEmpty());
        bool foundDeobf = false;
        for (const auto& lib : libs152) {
            if (lib.filename == QStringLiteral("deobfuscation_data_1.5.2.zip")) {
                QCOMPARE(lib.checksum, QStringLiteral("446e55cd986582c70fcf12cb27bc00114c5adfd9"));
                foundDeobf = true;
            }
        }
        QVERIFY(foundDeobf);

        QCOMPARE(g_VersionFilterData.fmlLibsMapping.value(QStringLiteral("1.4")).size(), 4);
        QCOMPARE(g_VersionFilterData.fmlLibsMapping.value(QStringLiteral("1.4.7")).size(), 4);
        QVERIFY(!g_VersionFilterData.fmlLibsMapping.contains(QStringLiteral("1.6.4")));
    }

    void test_lwjglWhitelist()
    {
        QVERIFY(g_VersionFilterData.lwjglWhitelist.contains(QStringLiteral("org.lwjgl.lwjgl:lwjgl")));
        QVERIFY(g_VersionFilterData.lwjglWhitelist.contains(QStringLiteral("org.lwjgl.lwjgl:lwjgl-platform")));
        QVERIFY(g_VersionFilterData.lwjglWhitelist.contains(QStringLiteral("net.java.jinput:jinput")));
        QVERIFY(!g_VersionFilterData.lwjglWhitelist.contains(QStringLiteral("com.mojang:minecraft")));
    }

    void test_javaRequirementDates()
    {
        QCOMPARE(g_VersionFilterData.java8BeginsDate, timeFromS3Time(QStringLiteral("2017-03-30T09:32:19+00:00")));
        QCOMPARE(g_VersionFilterData.java16BeginsDate, timeFromS3Time(QStringLiteral("2021-05-12T11:19:15+00:00")));
        QCOMPARE(g_VersionFilterData.java17BeginsDate, timeFromS3Time(QStringLiteral("2021-11-16T17:04:48+00:00")));
        QVERIFY(g_VersionFilterData.java8BeginsDate < g_VersionFilterData.java16BeginsDate);
        QVERIFY(g_VersionFilterData.java16BeginsDate < g_VersionFilterData.java17BeginsDate);
        QVERIFY(g_VersionFilterData.legacyCutoffDate < g_VersionFilterData.java8BeginsDate);
    }
};

QTEST_GUILESS_MAIN(VersionFilterTest)

#include "VersionFilter_test.moc"
