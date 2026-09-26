#include <QTest>

#include <minecraft/launch/MinecraftTarget.h>

class MinecraftTargetTest : public QObject {
    Q_OBJECT
   private slots:
    void test_parse_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<bool>("useWorld");
        QTest::addColumn<QString>("address");
        QTest::addColumn<int>("port");
        QTest::addColumn<QString>("world");

        QTest::newRow("hostname default port") << "mc.hypixel.net" << false << "mc.hypixel.net" << 25565 << QString();
        QTest::newRow("hostname custom port") << "mc.hypixel.net:25566" << false << "mc.hypixel.net" << 25566 << QString();
        QTest::newRow("invalid port falls back") << "mc.hypixel.net:notaport" << false << "mc.hypixel.net" << 25565 << QString();
        QTest::newRow("empty port falls back") << "mc.hypixel.net:" << false << "mc.hypixel.net" << 25565 << QString();
        QTest::newRow("port zero") << "localhost:0" << false << "localhost" << 0 << QString();
        QTest::newRow("port max") << "localhost:65535" << false << "localhost" << 65535 << QString();
        QTest::newRow("empty address") << "" << false << "" << 25565 << QString();
        QTest::newRow("ipv6 default port") << "[::1]" << false << "::1" << 25565 << QString();
        QTest::newRow("ipv6 with port") << "[2001:db8::1]:25567" << false << "2001:db8::1" << 25567 << QString();
        QTest::newRow("ipv6 empty host") << "[]:25565" << false << "" << 25565 << QString();
        QTest::newRow("unbracketed extra colons keep raw address") << "a:b:c" << false << "a:b:c" << 25565 << QString();
        QTest::newRow("world mode ignores host syntax") << "My World:1" << true << QString() << 0 << "My World:1";
    }
    void test_parse()
    {
        QFETCH(QString, input);
        QFETCH(bool, useWorld);
        QFETCH(QString, address);
        QFETCH(int, port);
        QFETCH(QString, world);

        const auto target = MinecraftTarget::parse(input, useWorld);
        QCOMPARE(target.world, world);
        if (useWorld) {
            QVERIFY(target.address.isEmpty());
            return;
        }
        QCOMPARE(target.address, address);
        QCOMPARE(target.port, static_cast<quint16>(port));
    }
};

QTEST_GUILESS_MAIN(MinecraftTargetTest)

#include "MinecraftTarget_test.moc"
