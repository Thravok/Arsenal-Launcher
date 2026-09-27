#include <QDir>
#include <QTemporaryDir>
#include <QTest>
#include <memory>

#include <BaseInstance.h>
#include <FileSystem.h>
#include <NullInstance.h>
#include <minecraft/MinecraftInstance.h>
#include <settings/INISettingsObject.h>

namespace {

INISettingsObject* makeGlobalSettings(const QString& path)
{
    auto* global = new INISettingsObject(path);
    global->registerSetting(QStringLiteral("ShowGameTime"), true);
    global->registerSetting(QStringLiteral("RecordGameTime"), true);
    global->registerSetting(QStringLiteral("PreLoadCommand"), QString());
    global->registerSetting(QStringLiteral("PreLaunchCommand"), QString());
    global->registerSetting(QStringLiteral("WrapperCommand"), QString());
    global->registerSetting(QStringLiteral("PostExitCommand"), QString());
    global->registerSetting(QStringLiteral("ShowConsole"), true);
    global->registerSetting(QStringLiteral("AutoCloseConsole"), false);
    global->registerSetting(QStringLiteral("ShowConsoleOnError"), true);
    global->registerSetting(QStringLiteral("LogPrePostOutput"), true);
    global->registerSetting(QStringLiteral("ConsoleMaxLines"), 1000);
    global->registerSetting(QStringLiteral("ConsoleOverflowStop"), false);
    return global;
}

std::unique_ptr<SettingsObject> makeInstanceSettings(const QString& instanceRoot)
{
    return std::make_unique<INISettingsObject>(FS::PathCombine(instanceRoot, QStringLiteral("instance.cfg")));
}

}  // namespace

class InstanceCommandsTest : public QObject {
    Q_OBJECT
   private slots:
    void test_preloadUsesGlobalUntilOverridden()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        auto* global = makeGlobalSettings(FS::PathCombine(temp.path(), QStringLiteral("global.cfg")));
        global->set(QStringLiteral("PreLoadCommand"), QStringLiteral("/usr/bin/packwiz refresh"));
        global->set(QStringLiteral("PreLaunchCommand"), QStringLiteral("echo pre"));
        global->set(QStringLiteral("WrapperCommand"), QStringLiteral("mangohud"));
        global->set(QStringLiteral("PostExitCommand"), QStringLiteral("echo post"));

        const QString root = FS::PathCombine(temp.path(), QStringLiteral("MyInst"));
        QVERIFY(QDir().mkpath(root));
        std::unique_ptr<NullInstance> instance(new NullInstance(global, makeInstanceSettings(root), root));

        QCOMPARE(instance->getPreLoadCommand(), QStringLiteral("/usr/bin/packwiz refresh"));
        QCOMPARE(instance->getPreLaunchCommand(), QStringLiteral("echo pre"));
        QCOMPARE(instance->getWrapperCommand(), QStringLiteral("mangohud"));
        QCOMPARE(instance->getPostExitCommand(), QStringLiteral("echo post"));

        instance->settings()->set(QStringLiteral("OverrideCommands"), true);
        instance->settings()->set(QStringLiteral("PreLoadCommand"), QStringLiteral("./preload.sh"));
        instance->settings()->set(QStringLiteral("PreLaunchCommand"), QString());
        instance->settings()->set(QStringLiteral("WrapperCommand"), QString());
        QCOMPARE(instance->getPreLoadCommand(), QStringLiteral("./preload.sh"));
        QCOMPARE(instance->getPreLaunchCommand(), QString());
        QCOMPARE(instance->getWrapperCommand(), QString());
    }

    void test_getVariablesExposeInstanceIdentity()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        auto* global = makeGlobalSettings(FS::PathCombine(temp.path(), QStringLiteral("global.cfg")));
        const QString root = FS::PathCombine(temp.path(), QStringLiteral("DemoWorld"));
        QVERIFY(QDir().mkpath(root));
        std::unique_ptr<MinecraftInstance> instance(new MinecraftInstance(global, makeInstanceSettings(root), root));
        if (!instance->settings()->getSetting(QStringLiteral("UseLatestMinecraftVersion")))
            instance->settings()->registerSetting(QStringLiteral("UseLatestMinecraftVersion"), false);
        instance->setName(QStringLiteral("Demo World"));

        const auto vars = instance->getVariables();
        QCOMPARE(vars.value(QStringLiteral("INST_NAME")), QStringLiteral("Demo World"));
        QCOMPARE(vars.value(QStringLiteral("INST_ID")), QStringLiteral("DemoWorld"));
        QCOMPARE(vars.value(QStringLiteral("INST_DIR")), QDir::toNativeSeparators(QDir(root).absolutePath()));
        QCOMPARE(vars.value(QStringLiteral("INST_MC_DIR")), QDir::toNativeSeparators(QDir(instance->gameRoot()).absolutePath()));
        QCOMPARE(vars.value(QStringLiteral("NO_COLOR")), QStringLiteral("1"));
        QVERIFY(vars.contains(QStringLiteral("INST_JAVA")));
        QVERIFY(vars.contains(QStringLiteral("INST_JAVA_ARGS")));
    }

    void test_consoleLimitsFallbackOnGarbage()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        INISettingsObject settings(FS::PathCombine(temp.path(), QStringLiteral("console.cfg")));
        settings.registerSetting(QStringLiteral("ConsoleMaxLines"), 2000);
        settings.registerSetting(QStringLiteral("ConsoleOverflowStop"), true);

        QCOMPARE(getConsoleMaxLines(&settings), 2000);
        QVERIFY(shouldStopOnConsoleOverflow(&settings));

        settings.set(QStringLiteral("ConsoleMaxLines"), QStringLiteral("not-a-number"));
        QCOMPARE(getConsoleMaxLines(&settings), 2000);
        settings.set(QStringLiteral("ConsoleOverflowStop"), false);
        QVERIFY(!shouldStopOnConsoleOverflow(&settings));
    }
};

QTEST_GUILESS_MAIN(InstanceCommandsTest)

#include "InstanceCommands_test.moc"
