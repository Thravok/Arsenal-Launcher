#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include <hackclients/HackClientInstanceDetect.h>

class HackClientInstanceDetectTest : public QObject {
    Q_OBJECT

   private slots:
    void test_meteorClientDetection()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        QVERIFY(!HackClients::modsFolderHasMeteorClient(dir.path()));

        QFile fabric(dir.filePath("fabric-api-0.1.jar"));
        QVERIFY(fabric.open(QIODevice::WriteOnly));
        fabric.write("x");
        fabric.close();
        QVERIFY(!HackClients::modsFolderHasMeteorClient(dir.path()));

        QFile meteor(dir.filePath("meteor-client-1.21.8-265.jar"));
        QVERIFY(meteor.open(QIODevice::WriteOnly));
        meteor.write("x");
        meteor.close();
        QVERIFY(HackClients::modsFolderHasMeteorClient(dir.path()));
    }

    void test_underscoreMeteorClientName()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QFile meteor(dir.filePath("meteor_client-dev.jar"));
        QVERIFY(meteor.open(QIODevice::WriteOnly));
        meteor.write("x");
        meteor.close();
        QVERIFY(HackClients::modsFolderHasMeteorClient(dir.path()));
    }

    void test_baritoneJarFileName()
    {
        QVERIFY(HackClients::isBaritoneJarFileName("baritone-meteor-1.21.8.jar"));
        QVERIFY(HackClients::isBaritoneJarFileName("baritone-fabric-1.21.8.jar"));
        QVERIFY(HackClients::isBaritoneJarFileName("Baritone-API-FABRIC.JAR"));
        QVERIFY(!HackClients::isBaritoneJarFileName("fabric-api-0.1.jar"));
        QVERIFY(!HackClients::isBaritoneJarFileName("baritone.cfg"));
        QVERIFY(!HackClients::isBaritoneJarFileName("meteor-client-1.21.8.jar"));
    }

    void test_replaceOtherBaritoneJarsKeepsNewInstall()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        auto writeFile = [&](const QString& name) -> bool {
            QFile f(dir.filePath(name));
            if (!f.open(QIODevice::WriteOnly))
                return false;
            f.write("x");
            f.close();
            return true;
        };

        QVERIFY(writeFile("meteor-client-1.21.8-265.jar"));
        QVERIFY(writeFile("baritone-meteor-1.21.8.jar"));
        QVERIFY(writeFile("fabric-api-0.1.jar"));
        QVERIFY(writeFile("baritone-fabric-1.21.8.jar"));

        const auto removed = HackClients::replaceOtherBaritoneJars(dir.path(), QStringLiteral("baritone-fabric-1.21.8.jar"));
        QCOMPARE(removed.size(), 1);
        QVERIFY(removed.contains(QStringLiteral("baritone-meteor-1.21.8.jar")));

        QVERIFY(QFile::exists(dir.filePath("baritone-fabric-1.21.8.jar")));
        QVERIFY(QFile::exists(dir.filePath("meteor-client-1.21.8-265.jar")));
        QVERIFY(QFile::exists(dir.filePath("fabric-api-0.1.jar")));
        QVERIFY(!QFile::exists(dir.filePath("baritone-meteor-1.21.8.jar")));
    }

    void test_replaceOtherBaritoneJarsDoesNothingWithoutTargetJars()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QFile fabric(dir.filePath("fabric-api-0.1.jar"));
        QVERIFY(fabric.open(QIODevice::WriteOnly));
        fabric.write("x");
        fabric.close();

        QVERIFY(HackClients::replaceOtherBaritoneJars(dir.path(), QStringLiteral("baritone-fabric-1.21.8.jar")).isEmpty());
        QVERIFY(QFile::exists(dir.filePath("fabric-api-0.1.jar")));
    }
};

QTEST_GUILESS_MAIN(HackClientInstanceDetectTest)

#include "HackClientInstanceDetect_test.moc"
