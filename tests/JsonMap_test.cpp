#include <QTest>

#include "Json.h"

class JsonMapTest : public QObject {
    Q_OBJECT
   private slots:
    void worldToolsRoundTrip()
    {
        QVariantMap tools;
        tools.insert(QStringLiteral("NBT"), QStringLiteral("nbted \"${WORLD_PATH}\""));
        tools.insert(QStringLiteral("MCA"), QStringLiteral("C:\\Program Files\\mca.exe ${WORLD_PATH}"));

        const QString stored = Json::fromMap(tools);
        QVERIFY(stored.startsWith('{'));
        const QVariantMap restored = Json::toMap(stored);
        QCOMPARE(restored.value(QStringLiteral("NBT")).toString(), QStringLiteral("nbted \"${WORLD_PATH}\""));
        QCOMPARE(restored.value(QStringLiteral("MCA")).toString(), QStringLiteral("C:\\Program Files\\mca.exe ${WORLD_PATH}"));
        QCOMPARE(Json::toMap(Json::fromMap(restored)), restored);
    }

    void invalidOrNonObjectJsonIsEmpty()
    {
        QVERIFY(Json::toMap(QString()).isEmpty());
        QVERIFY(Json::toMap(QStringLiteral("not json")).isEmpty());
        QVERIFY(Json::toMap(QStringLiteral("[1,2]")).isEmpty());
        QVERIFY(Json::toMap(QStringLiteral("null")).isEmpty());
        QCOMPARE(Json::fromMap({}), QStringLiteral("{}"));
        QVERIFY(Json::toMap(QStringLiteral("{}")).isEmpty());
    }
};

QTEST_GUILESS_MAIN(JsonMapTest)
#include "JsonMap_test.moc"
