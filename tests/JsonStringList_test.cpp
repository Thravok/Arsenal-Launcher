#include <QTest>

#include <Json.h>

class JsonStringListTest : public QObject {
    Q_OBJECT
   private slots:
    void test_roundTrip()
    {
        const QStringList original{ QStringLiteral("alpha"), QStringLiteral("beta"), QStringLiteral("with space") };
        const QString stored = Json::fromStringList(original);
        QCOMPARE(Json::toStringList(stored), original);
        QCOMPARE(Json::toStringList(Json::fromStringList(original)), original);
    }

    void test_fromStringListCompactJson()
    {
        QCOMPARE(Json::fromStringList({}), QStringLiteral("[]"));
        QCOMPARE(Json::fromStringList({ QStringLiteral("one") }), QStringLiteral("[\"one\"]"));
        QCOMPARE(Json::fromStringList({ QStringLiteral("a"), QStringLiteral("b") }), QStringLiteral("[\"a\",\"b\"]"));
    }

    void test_toStringListRejectsInvalidPayloads()
    {
        QVERIFY(Json::toStringList(QString()).isEmpty());
        QVERIFY(Json::toStringList(QStringLiteral("not json")).isEmpty());
        QVERIFY(Json::toStringList(QStringLiteral("{}")).isEmpty());
        QVERIFY(Json::toStringList(QStringLiteral("null")).isEmpty());
        QVERIFY(Json::toStringList(QStringLiteral("[1,2]")).isEmpty());
        QVERIFY(Json::toStringList(QStringLiteral("[\"ok\",1]")).isEmpty());
        QCOMPARE(Json::toStringList(QStringLiteral("[]")), QStringList{});
        QCOMPARE(Json::toStringList(QStringLiteral("[\"only\"]")), QStringList{ QStringLiteral("only") });
    }
};

QTEST_GUILESS_MAIN(JsonStringListTest)
#include "JsonStringList_test.moc"
