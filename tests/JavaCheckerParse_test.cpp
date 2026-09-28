#include <QTest>

#include <java/JavaChecker.h>

class JavaCheckerParseTest : public QObject {
    Q_OBJECT
   private slots:
    void test_parse64BitAmd64()
    {
        JavaChecker::Result result;
        QVERIFY(JavaChecker::parseCheckerOutput(
            QStringLiteral("os.arch=amd64\njava.version=21.0.2\njava.vendor=Eclipse Adoptium\n"), result));
        QCOMPARE(result.validity, JavaChecker::Result::Validity::Valid);
        QVERIFY(result.is_64bit);
        QCOMPARE(result.mojangPlatform, QStringLiteral("64"));
        QCOMPARE(result.realPlatform, QStringLiteral("amd64"));
        QCOMPARE(result.javaVersion.toString(), QStringLiteral("21.0.2"));
        QCOMPARE(result.javaVendor, QStringLiteral("Eclipse Adoptium"));
    }

    void test_parse32BitAndArm64AndBedrockGarbage()
    {
        JavaChecker::Result x86;
        QVERIFY(JavaChecker::parseCheckerOutput(QStringLiteral("os.arch=x86\njava.version=8.0.1\njava.vendor=Oracle\n"), x86));
        QVERIFY(!x86.is_64bit);
        QCOMPARE(x86.mojangPlatform, QStringLiteral("32"));

        JavaChecker::Result arm;
        QVERIFY(JavaChecker::parseCheckerOutput(QStringLiteral("os.arch=aarch64\njava.version=17\njava.vendor=Azul\n"), arm));
        QVERIFY(arm.is_64bit);

        JavaChecker::Result bedrock;
        QVERIFY(JavaChecker::parseCheckerOutput(QStringLiteral("/opt/bedrock/strata/noise\n"
                                                              "os.arch=x86_64\n"
                                                              "java.version=11.0.1\n"
                                                              "java.vendor=Debian\n"),
                                                bedrock));
        QVERIFY(bedrock.is_64bit);
        QCOMPARE(bedrock.realPlatform, QStringLiteral("x86_64"));
    }

    void test_parseRejectsIncompleteAndMalformedLines()
    {
        JavaChecker::Result missing;
        QVERIFY(!JavaChecker::parseCheckerOutput(QStringLiteral("os.arch=amd64\njava.version=21\n"), missing));
        QCOMPARE(missing.validity, JavaChecker::Result::Validity::ReturnedInvalidData);

        JavaChecker::Result extraEquals;
        // Values containing '=' are ignored (split yields more than two parts).
        QVERIFY(!JavaChecker::parseCheckerOutput(
            QStringLiteral("os.arch=amd64\njava.version=21=ea\njava.vendor=Oracle\n"), extraEquals));

        JavaChecker::Result empty;
        QVERIFY(!JavaChecker::parseCheckerOutput(QString(), empty));
    }
};

QTEST_GUILESS_MAIN(JavaCheckerParseTest)
#include "JavaCheckerParse_test.moc"
