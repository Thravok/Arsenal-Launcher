#include <QJsonObject>
#include <QTest>

#include <RuntimeContext.h>
#include <minecraft/Rule.h>

class RuntimeContextTest : public QObject {
    Q_OBJECT

    static RuntimeContext context(const QString& system, const QString& realArch)
    {
        RuntimeContext runtime;
        runtime.system = system;
        runtime.javaRealArchitecture = realArch;
        return runtime;
    }

   private slots:
    void test_mappedJavaRealArchitecture()
    {
        QCOMPARE(context(QStringLiteral("linux"), QStringLiteral("amd64")).mappedJavaRealArchitecture(), QStringLiteral("x86_64"));
        QCOMPARE(context(QStringLiteral("linux"), QStringLiteral("i386")).mappedJavaRealArchitecture(), QStringLiteral("x86"));
        QCOMPARE(context(QStringLiteral("linux"), QStringLiteral("i686")).mappedJavaRealArchitecture(), QStringLiteral("x86"));
        QCOMPARE(context(QStringLiteral("linux"), QStringLiteral("aarch64")).mappedJavaRealArchitecture(), QStringLiteral("arm64"));
        QCOMPARE(context(QStringLiteral("linux"), QStringLiteral("arm")).mappedJavaRealArchitecture(), QStringLiteral("arm32"));
        QCOMPARE(context(QStringLiteral("linux"), QStringLiteral("armhf")).mappedJavaRealArchitecture(), QStringLiteral("arm32"));
        QCOMPARE(context(QStringLiteral("linux"), QStringLiteral("riscv64")).mappedJavaRealArchitecture(), QStringLiteral("riscv64"));
    }

    void test_classifierAndLegacyArch()
    {
        const auto amd64 = context(QStringLiteral("linux"), QStringLiteral("amd64"));
        QCOMPARE(amd64.getClassifier(), QStringLiteral("linux-x86_64"));
        QVERIFY(amd64.isLegacyArch());
        QVERIFY(amd64.classifierMatches(QStringLiteral("linux-x86_64")));
        QVERIFY(amd64.classifierMatches(QStringLiteral("linux")));
        QVERIFY(!amd64.classifierMatches(QStringLiteral("windows")));

        const auto arm64 = context(QStringLiteral("linux"), QStringLiteral("aarch64"));
        QCOMPARE(arm64.getClassifier(), QStringLiteral("linux-arm64"));
        QVERIFY(!arm64.isLegacyArch());
        QVERIFY(arm64.classifierMatches(QStringLiteral("linux-arm64")));
        QVERIFY(!arm64.classifierMatches(QStringLiteral("linux")));

        const auto osx = context(QStringLiteral("osx"), QStringLiteral("x86_64"));
        QVERIFY(osx.classifierMatches(QStringLiteral("osx")));
        QVERIFY(osx.classifierMatches(QStringLiteral("osx-x86_64")));
    }

    void test_ruleApply()
    {
        const auto linuxAmd64 = context(QStringLiteral("linux"), QStringLiteral("amd64"));
        const auto linuxArm64 = context(QStringLiteral("linux"), QStringLiteral("aarch64"));
        const auto windows = context(QStringLiteral("windows"), QStringLiteral("amd64"));

        auto allowLinux = Rule::fromJson(QJsonObject{ { QStringLiteral("action"), QStringLiteral("allow") },
                                                      { QStringLiteral("os"), QJsonObject{ { QStringLiteral("name"), QStringLiteral("linux") } } } });
        QCOMPARE(allowLinux.apply(linuxAmd64), Rule::Allow);
        QCOMPARE(allowLinux.apply(linuxArm64), Rule::Defer);
        QCOMPARE(allowLinux.apply(windows), Rule::Defer);

        auto allowLinuxArm = Rule::fromJson(
            QJsonObject{ { QStringLiteral("action"), QStringLiteral("allow") },
                         { QStringLiteral("os"), QJsonObject{ { QStringLiteral("name"), QStringLiteral("linux-arm64") } } } });
        QCOMPARE(allowLinuxArm.apply(linuxArm64), Rule::Allow);
        QCOMPARE(allowLinuxArm.apply(linuxAmd64), Rule::Defer);

        auto disallowWindows =
            Rule::fromJson(QJsonObject{ { QStringLiteral("action"), QStringLiteral("disallow") },
                                        { QStringLiteral("os"), QJsonObject{ { QStringLiteral("name"), QStringLiteral("windows") } } } });
        QCOMPARE(disallowWindows.apply(windows), Rule::Disallow);
        QCOMPARE(disallowWindows.apply(linuxAmd64), Rule::Defer);

        auto allowAll = Rule::fromJson(QJsonObject{ { QStringLiteral("action"), QStringLiteral("allow") } });
        QCOMPARE(allowAll.apply(linuxArm64), Rule::Allow);
        QCOMPARE(allowAll.apply(windows), Rule::Allow);

        auto unknown = Rule::fromJson(QJsonObject{ { QStringLiteral("action"), QStringLiteral("maybe") } });
        QCOMPARE(unknown.apply(linuxAmd64), Rule::Defer);
    }
};

QTEST_GUILESS_MAIN(RuntimeContextTest)
#include "RuntimeContext_test.moc"
