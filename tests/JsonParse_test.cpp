#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QTest>
#include <QUrl>
#include <QUuid>

#include <Json.h>

class JsonParseTest : public QObject {
    Q_OBJECT
   private slots:
    void test_requireDocumentObjectAndArray()
    {
        const auto objectDoc = Json::requireDocument(QByteArrayLiteral("{\"a\":1}"));
        QVERIFY(objectDoc.isObject());
        QCOMPARE(Json::requireObject(objectDoc).value(QStringLiteral("a")).toInt(), 1);

        const auto arrayDoc = Json::requireDocument(QByteArrayLiteral("[1,2]"));
        QVERIFY(arrayDoc.isArray());
        QCOMPARE(Json::requireArray(arrayDoc).size(), 2);

        QVERIFY_THROWS_EXCEPTION(Json::JsonException, Json::requireDocument(QByteArrayLiteral("{bad")));
        QVERIFY_THROWS_EXCEPTION(Json::JsonException, Json::requireObject(arrayDoc));
        QVERIFY_THROWS_EXCEPTION(Json::JsonException, Json::requireArray(objectDoc));
    }

    void test_requireTypedValues()
    {
        QJsonObject obj;
        obj.insert(QStringLiteral("name"), QStringLiteral("pack"));
        obj.insert(QStringLiteral("count"), 3);
        obj.insert(QStringLiteral("ok"), true);
        obj.insert(QStringLiteral("nested"), QJsonObject{ { QStringLiteral("x"), 1 } });

        QCOMPARE(Json::requireString(obj, QStringLiteral("name")), QStringLiteral("pack"));
        QCOMPARE(Json::requireInteger(obj, QStringLiteral("count")), 3);
        QCOMPARE(Json::requireBoolean(obj, QStringLiteral("ok")), true);
        QCOMPARE(Json::requireObject(obj, QStringLiteral("nested")).value(QStringLiteral("x")).toInt(), 1);

        QVERIFY_THROWS_EXCEPTION(Json::JsonException, Json::requireString(obj, QStringLiteral("missing")));
        QVERIFY_THROWS_EXCEPTION(Json::JsonException, Json::requireInteger(QJsonValue(1.5)));
        QVERIFY_THROWS_EXCEPTION(Json::JsonException, Json::requireBoolean(QJsonValue(QStringLiteral("true"))));
    }

    void test_requireUrlAndUuid()
    {
        QCOMPARE(Json::requireUrl(QJsonValue(QStringLiteral("https://example.com/a"))), QUrl(QStringLiteral("https://example.com/a")));
        QVERIFY(Json::requireUrl(QJsonValue(QStringLiteral(""))).isEmpty());
        QVERIFY_THROWS_EXCEPTION(Json::JsonException, Json::requireUrl(QJsonValue(QStringLiteral("not a url"))));

        const QUuid uuid = QUuid::createUuid();
        QCOMPARE(Json::requireUuid(QJsonValue(uuid.toString())), uuid);
        QVERIFY_THROWS_EXCEPTION(Json::JsonException, Json::requireUuid(QJsonValue(QStringLiteral("not-a-uuid"))));
    }

    void test_parseUntilGarbageKeepsValidPrefix()
    {
        QJsonParseError error{};
        QString garbage;
        const auto doc = Json::parseUntilGarbage(QByteArrayLiteral("{\"pack\":{\"pack_format\":15}}trailing comment"), &error, &garbage);

        QVERIFY(error.error == QJsonParseError::NoError);
        QVERIFY(doc.isObject());
        QCOMPARE(doc.object().value(QStringLiteral("pack")).toObject().value(QStringLiteral("pack_format")).toInt(), 15);
        QCOMPARE(garbage, QStringLiteral("trailing comment"));
    }

    void test_parseUntilGarbageCleanDocument()
    {
        QJsonParseError error{};
        QString garbage;
        const auto doc = Json::parseUntilGarbage(QByteArrayLiteral("{\"ok\":true}"), &error, &garbage);
        QVERIFY(error.error == QJsonParseError::NoError);
        QVERIFY(doc.object().value(QStringLiteral("ok")).toBool());
        QVERIFY(garbage.isEmpty());
    }

    void test_writeStringHelpersSkipEmpty()
    {
        QJsonObject obj;
        Json::writeString(obj, QStringLiteral("name"), QStringLiteral("value"));
        Json::writeString(obj, QStringLiteral("empty"), QString());
        Json::writeStringList(obj, QStringLiteral("tags"), QStringList{ QStringLiteral("a"), QStringLiteral("b") });
        Json::writeStringList(obj, QStringLiteral("none"), {});

        QCOMPARE(obj.value(QStringLiteral("name")).toString(), QStringLiteral("value"));
        QVERIFY(!obj.contains(QStringLiteral("empty")));
        QCOMPARE(obj.value(QStringLiteral("tags")).toArray().size(), 2);
        QVERIFY(!obj.contains(QStringLiteral("none")));
    }
};

QTEST_GUILESS_MAIN(JsonParseTest)
#include "JsonParse_test.moc"
