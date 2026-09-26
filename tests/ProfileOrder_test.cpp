#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

#include <minecraft/ProfileUtils.h>

class ProfileOrderTest : public QObject {
    Q_OBJECT
   private slots:
    void test_missingAndInvalidOrders()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        ProfileUtils::PatchOrder order{ QStringLiteral("keep-me") };
        QVERIFY(!ProfileUtils::readOverrideOrders(QDir(dir.path()).filePath(QStringLiteral("missing.json")), order));
        QCOMPARE(order, ProfileUtils::PatchOrder{ QStringLiteral("keep-me") });

        const QString badJson = QDir(dir.path()).filePath(QStringLiteral("bad.json"));
        {
            QFile file(badJson);
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("{");
        }
        QVERIFY(!ProfileUtils::readOverrideOrders(badJson, order));

        const QString wrongVersion = QDir(dir.path()).filePath(QStringLiteral("version.json"));
        {
            QJsonObject obj;
            obj.insert(QStringLiteral("version"), 99);
            obj.insert(QStringLiteral("order"), QJsonArray{ QStringLiteral("net.minecraft") });
            QVERIFY(ProfileUtils::saveJsonFile(QJsonDocument(obj), wrongVersion));
        }
        order = { QStringLiteral("stale") };
        QVERIFY(!ProfileUtils::readOverrideOrders(wrongVersion, order));
        QVERIFY(order.isEmpty());
    }

    void test_validOrderRoundTrip()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString path = QDir(dir.path()).filePath(QStringLiteral("order.json"));
        QJsonObject obj;
        obj.insert(QStringLiteral("version"), 1);
        obj.insert(QStringLiteral("order"), QJsonArray{ QStringLiteral("net.minecraft"), QStringLiteral("net.fabricmc.fabric-loader"),
                                                        QStringLiteral("custom.patch") });
        QVERIFY(ProfileUtils::saveJsonFile(QJsonDocument(obj), path));

        ProfileUtils::PatchOrder order;
        QVERIFY(ProfileUtils::readOverrideOrders(path, order));
        QCOMPARE(order, (ProfileUtils::PatchOrder{ QStringLiteral("net.minecraft"), QStringLiteral("net.fabricmc.fabric-loader"),
                                                   QStringLiteral("custom.patch") }));
    }

    void test_saveJsonFileRejectsDirectory()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(!ProfileUtils::saveJsonFile(QJsonDocument(QJsonObject{}), dir.path()));
    }
};

QTEST_GUILESS_MAIN(ProfileOrderTest)

#include "ProfileOrder_test.moc"
