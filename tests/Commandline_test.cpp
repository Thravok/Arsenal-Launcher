#include <QProcess>
#include <QProcessEnvironment>
#include <QTest>

#include <Commandline.h>

class CommandlineTest : public QObject {
    Q_OBJECT
   private slots:
    void test_splitArgs_data()
    {
        QTest::addColumn<QString>("args");
        QTest::addColumn<QStringList>("expected");

        QTest::newRow("plain arguments") << "a b c" << QStringList{ "a", "b", "c" };
        QTest::newRow("quoted argument with spaces") << "a \"b c\" d" << QStringList{ "a", "b c", "d" };
        QTest::newRow("windows path without quotes") << "-Dorg.lwjgl.glfw.libname=C:\\Users\\user\\glfw3.dll"
                                                     << QStringList{ "-Dorg.lwjgl.glfw.libname=C:\\Users\\user\\glfw3.dll" };
        QTest::newRow("windows path in quotes keeps backslashes")
            << "-Dorg.lwjgl.glfw.libname=\"C:\\Users\\test user\\glfw3.dll\""
            << QStringList{ "-Dorg.lwjgl.glfw.libname=C:\\Users\\test user\\glfw3.dll" };
        QTest::newRow("fully quoted argument keeps backslashes")
            << "\"-Dorg.lwjgl.glfw.libname=C:\\Users\\test user\\glfw3.dll\""
            << QStringList{ "-Dorg.lwjgl.glfw.libname=C:\\Users\\test user\\glfw3.dll" };
        QTest::newRow("windows path in single quotes keeps backslashes")
            << "-Dfoo='C:\\Users\\test user\\glfw3.dll'" << QStringList{ "-Dfoo=C:\\Users\\test user\\glfw3.dll" };
        QTest::newRow("escaped quotes") << "-Dfoo=\"say \\\"hi\\\" now\"" << QStringList{ "-Dfoo=say \"hi\" now" };
        QTest::newRow("double backslash in quotes collapses")
            << "-Dfoo=\"C:\\\\path\\\\file.dll\"" << QStringList{ "-Dfoo=C:\\path\\file.dll" };
    }
    void test_splitArgs()
    {
        QFETCH(QString, args);
        QFETCH(QStringList, expected);

        QCOMPARE(Commandline::splitArgs(args), expected);
    }

    void test_expandVariables_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QString>("expected");

        QTest::newRow("brace form") << "open ${WORLD_PATH}" << "open /tmp/saves/New World";
        QTest::newRow("bare form") << "open $WORLD_PATH" << "open /tmp/saves/New World";
        QTest::newRow("missing variable stays") << "keep ${MISSING}" << "keep ${MISSING}";
        QTest::newRow("empty variable stays") << "keep ${EMPTY}" << "keep ${EMPTY}";
        QTest::newRow("dollar without name") << "price is $5" << "price is $5";
        QTest::newRow("unclosed brace stays") << "keep ${WORLD_PATH" << "keep ${WORLD_PATH";
        QTest::newRow("adjacent vars") << "${WORLD_PATH}-$INST" << "/tmp/saves/New World-demo";
        QTest::newRow("windows path value") << "-Dlib=${LIB}" << "-Dlib=C:\\Users\\test user\\glfw3.dll";
    }
    void test_expandVariables()
    {
        QFETCH(QString, input);
        QFETCH(QString, expected);

        QProcessEnvironment env;
        env.insert("WORLD_PATH", "/tmp/saves/New World");
        env.insert("INST", "demo");
        env.insert("EMPTY", "");
        env.insert("LIB", "C:\\Users\\test user\\glfw3.dll");

        QCOMPARE(Commandline::expandVariables(input, env), expected);
    }

    void test_process_expands_after_split()
    {
        QProcessEnvironment env;
        env.insert("WORLD_PATH", "/tmp/saves/New World");
        env.insert("LIB", "C:\\Users\\test user\\glfw3.dll");

        QCOMPARE(Commandline::process("nbted ${WORLD_PATH}", env), (QStringList{ "nbted", "/tmp/saves/New World" }));
        QCOMPARE(Commandline::process("\"C:\\Program Files\\nbted.exe\" \"${WORLD_PATH}\"", env),
                 (QStringList{ "C:\\Program Files\\nbted.exe", "/tmp/saves/New World" }));
        QCOMPARE(Commandline::process("-Dlib=\"${LIB}\"", env), (QStringList{ "-Dlib=C:\\Users\\test user\\glfw3.dll" }));
    }

    void test_quoteForSplitCommand_data()
    {
        QTest::addColumn<QString>("input");

        QTest::newRow("no spaces") << "nbted";
        QTest::newRow("windows path with spaces") << "C:\\Program Files\\nbted.exe";
        QTest::newRow("embedded quotes") << "say \"hi\" now";
        QTest::newRow("world path") << "/tmp/saves/New World";
    }
    void test_quoteForSplitCommand()
    {
        QFETCH(QString, input);

        const QString quoted = Commandline::quoteForSplitCommand(input);
        QCOMPARE(QProcess::splitCommand(quoted), QStringList{ input });
        if (!input.contains(' ')) {
            QCOMPARE(quoted, input);
        } else {
            QVERIFY(quoted.startsWith('"'));
            QVERIFY(quoted.endsWith('"'));
        }
    }
};

QTEST_GUILESS_MAIN(CommandlineTest)
#include "Commandline_test.moc"
