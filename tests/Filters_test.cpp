#include <QTest>

#include <Filter.h>

class FiltersTest : public QObject {
    Q_OBJECT
   private slots:
    void test_equalsFamily()
    {
        QVERIFY(Filters::equals(QStringLiteral("1.21.1"))(QStringLiteral("1.21.1")));
        QVERIFY(!Filters::equals(QStringLiteral("1.21.1"))(QStringLiteral("1.21")));
        QVERIFY(Filters::equalsOrEmpty(QStringLiteral("fabric"))(QString()));
        QVERIFY(Filters::equalsOrEmpty(QStringLiteral("fabric"))(QStringLiteral("fabric")));
        QVERIFY(!Filters::equalsOrEmpty(QStringLiteral("fabric"))(QStringLiteral("forge")));

        QVERIFY(Filters::equalsAny({})(QStringLiteral("anything")));
        QVERIFY(Filters::equalsAny({ QStringLiteral("fabric"), QStringLiteral("quilt") })(QStringLiteral("quilt")));
        QVERIFY(!Filters::equalsAny({ QStringLiteral("fabric"), QStringLiteral("quilt") })(QStringLiteral("forge")));
    }

    void test_containsStartsWithAndRegexp()
    {
        QVERIFY(Filters::contains(QStringLiteral("sodium"))(QStringLiteral("sodium-extra")));
        QVERIFY(!Filters::contains(QStringLiteral("sodium"))(QStringLiteral("lithium")));
        QVERIFY(Filters::startsWith(QStringLiteral("net.fabricmc"))(QStringLiteral("net.fabricmc.fabric-loader")));
        QVERIFY(!Filters::startsWith(QStringLiteral("net.fabricmc"))(QStringLiteral("net.minecraftforge")));
        QVERIFY(Filters::regexp(QRegularExpression(QStringLiteral("^1\\.21(\\.\\d+)?$")))(QStringLiteral("1.21.1")));
        QVERIFY(!Filters::regexp(QRegularExpression(QStringLiteral("^1\\.21(\\.\\d+)?$")))(QStringLiteral("1.20.1")));
    }

    void test_inverseAndAny()
    {
        auto notSnapshot = Filters::inverse(Filters::contains(QStringLiteral("w")));
        QVERIFY(notSnapshot(QStringLiteral("1.21.1")));
        QVERIFY(!notSnapshot(QStringLiteral("24w14a")));

        auto fabricOrQuilt = Filters::any({ Filters::equals(QStringLiteral("fabric")), Filters::equals(QStringLiteral("quilt")) });
        QVERIFY(fabricOrQuilt(QStringLiteral("fabric")));
        QVERIFY(fabricOrQuilt(QStringLiteral("quilt")));
        QVERIFY(!fabricOrQuilt(QStringLiteral("forge")));
        QVERIFY(!Filters::any({})(QStringLiteral("x")));
    }
};

QTEST_GUILESS_MAIN(FiltersTest)
#include "Filters_test.moc"
