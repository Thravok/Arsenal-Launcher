#include <QRegularExpression>
#include <QTest>

#include <InstanceCopyPrefs.h>

class InstanceCopyPrefsTest : public QObject {
    Q_OBJECT

    static bool matches(const QString& regex, const QString& path)
    {
        return QRegularExpression(regex).match(path).hasMatch();
    }

   private slots:
    void test_defaultCopiesEverything()
    {
        InstanceCopyPrefs prefs;
        QVERIFY(prefs.allTrue());
        QVERIFY(prefs.getSelectedFiltersAsRegex().isEmpty());
    }

    void test_excludedFoldersBecomeMinecraftRootedRegex()
    {
        InstanceCopyPrefs prefs;
        prefs.enableCopySaves(false);
        prefs.enableCopyMods(false);
        prefs.enableCopyServers(false);
        prefs.enableCopyScreenshots(false);
        prefs.enableCopyGameOptions(false);
        prefs.enableCopyResourcePacks(false);
        prefs.enableCopyShaderPacks(false);
        QVERIFY(!prefs.allTrue());

        const QString regex = prefs.getSelectedFiltersAsRegex();
        QVERIFY(matches(regex, QStringLiteral(".minecraft/saves")));
        QVERIFY(matches(regex, QStringLiteral("minecraft/saves")));
        QVERIFY(matches(regex, QStringLiteral(".minecraft/mods")));
        QVERIFY(matches(regex, QStringLiteral(".minecraft/coremods")));
        QVERIFY(matches(regex, QStringLiteral(".minecraft/config")));
        QVERIFY(matches(regex, QStringLiteral(".minecraft/servers.dat")));
        QVERIFY(matches(regex, QStringLiteral(".minecraft/servers.dat_old")));
        QVERIFY(matches(regex, QStringLiteral(".minecraft/server-resource-packs")));
        QVERIFY(matches(regex, QStringLiteral(".minecraft/screenshots")));
        QVERIFY(matches(regex, QStringLiteral(".minecraft/options.txt")));
        QVERIFY(matches(regex, QStringLiteral(".minecraft/resourcepacks")));
        QVERIFY(matches(regex, QStringLiteral(".minecraft/texturepacks")));
        QVERIFY(matches(regex, QStringLiteral(".minecraft/shaderpacks")));
        QVERIFY(!matches(regex, QStringLiteral("saves")));
        QVERIFY(regex.contains(QStringLiteral("[.]?minecraft/saves")));
        QVERIFY(regex.contains(QStringLiteral("[.]?minecraft/mods")));
    }

    void test_additionalFiltersAreJoined()
    {
        InstanceCopyPrefs prefs;
        prefs.enableCopySaves(false);
        const QString regex = prefs.getSelectedFiltersAsRegex({ QStringLiteral("crash-reports") });
        QVERIFY(matches(regex, QStringLiteral(".minecraft/saves")));
        QVERIFY(matches(regex, QStringLiteral(".minecraft/crash-reports")));
    }
};

QTEST_GUILESS_MAIN(InstanceCopyPrefsTest)
#include "InstanceCopyPrefs_test.moc"
