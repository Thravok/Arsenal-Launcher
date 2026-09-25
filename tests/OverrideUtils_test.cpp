#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include <FileSystem.h>
#include <modplatform/helpers/OverrideUtils.h>

class OverrideUtilsTest : public QObject {
    Q_OBJECT
   private slots:
    void test_missingOverridesFileIsEmpty()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(Override::readOverrides(QStringLiteral("overrides"), dir.path()).isEmpty());
    }

    void test_createThenReadRelativeOverridePaths()
    {
        QTemporaryDir parent;
        QVERIFY(parent.isValid());

        const QString overrideRoot = FS::PathCombine(parent.path(), QStringLiteral("overrides"));
        FS::write(FS::PathCombine(overrideRoot, QStringLiteral("config/foo.toml")), QByteArrayLiteral("a=1"));
        FS::write(FS::PathCombine(overrideRoot, QStringLiteral("options.txt")), QByteArrayLiteral("b=2"));

        Override::createOverrides(QStringLiteral("overrides"), parent.path(), overrideRoot);

        QVERIFY(QFile::exists(FS::PathCombine(parent.path(), QStringLiteral("overrides.txt"))));

        const QStringList entries = Override::readOverrides(QStringLiteral("overrides"), parent.path());
        QVERIFY(entries.contains(QStringLiteral("config/foo.toml")));
        QVERIFY(entries.contains(QStringLiteral("options.txt")));
        // The reader keeps the empty EOF line produced by the write loop.
        QCOMPARE(entries.last(), QString());
    }

    void test_createOverridesReplacesPreviousFile()
    {
        QTemporaryDir parent;
        QVERIFY(parent.isValid());

        const QString firstRoot = FS::PathCombine(parent.path(), QStringLiteral("overrides"));
        FS::write(FS::PathCombine(firstRoot, QStringLiteral("old.txt")), QByteArrayLiteral("old"));
        Override::createOverrides(QStringLiteral("overrides"), parent.path(), firstRoot);

        FS::deletePath(FS::PathCombine(firstRoot, QStringLiteral("old.txt")));
        FS::write(FS::PathCombine(firstRoot, QStringLiteral("new.txt")), QByteArrayLiteral("new"));
        Override::createOverrides(QStringLiteral("overrides"), parent.path(), firstRoot);

        const QStringList entries = Override::readOverrides(QStringLiteral("overrides"), parent.path());
        QVERIFY(entries.contains(QStringLiteral("new.txt")));
        QVERIFY(!entries.contains(QStringLiteral("old.txt")));
    }
};

QTEST_GUILESS_MAIN(OverrideUtilsTest)
#include "OverrideUtils_test.moc"
