#include <QTest>

#include <hackclients/BaritoneMaven.h>
#include <hackclients/GradleProperties.h>

class HackClientsInstallTest : public QObject {
    Q_OBJECT
   private slots:
    void test_gradleProperty_data()
    {
        QTest::addColumn<QString>("text");
        QTest::addColumn<QString>("key");
        QTest::addColumn<QString>("expected");

        const QString wurst = QStringLiteral(
            "# Wurst gradle.properties\n"
            "org.gradle.jvmargs=-Xmx2G\n"
            "loader_version=0.16.9 \n"
            "fabric_api_version=0.158.0+26.2\n"
            "# loader_version=ignored\n"
            "minecraft_version=26.2\n"
            "loader_version_extra=nope\n");

        QTest::newRow("wurst loader") << wurst << "loader_version" << "0.16.9";
        QTest::newRow("wurst fabric api") << wurst << "fabric_api_version" << "0.158.0+26.2";
        QTest::newRow("wurst minecraft") << wurst << "minecraft_version" << "26.2";
        QTest::newRow("missing") << wurst << "kotlinVersion" << "";
        QTest::newRow("comment only") << QStringLiteral("# loader_version=1.0\n") << "loader_version" << "";
        QTest::newRow("empty file") << QString() << "loader_version" << "";

        const QString lambda = QStringLiteral(
            "fabricLoaderVersion=0.16.9\n"
            "fabricApiVersion=0.110.0\n"
            "kotlinFabricVersion=1.13.8+kotlin\n"
            "kotlinVersion=2.3.0\n"
            "baritoneVersion=1.14.0\n"
            "minecraftVersion=1.21.11\n");
        QTest::newRow("lambda kotlin") << lambda << "kotlinVersion" << "2.3.0";
        QTest::newRow("lambda baritone") << lambda << "baritoneVersion" << "1.14.0";
        QTest::newRow("lambda minecraft") << lambda << "minecraftVersion" << "1.21.11";
    }
    void test_gradleProperty()
    {
        QFETCH(QString, text);
        QFETCH(QString, key);
        QFETCH(QString, expected);

        QCOMPARE(HackClients::gradleProperty(text, key), expected);
    }

    void test_fabricJarNameFromMetadata()
    {
        const QByteArray snapshotXml = R"(
<metadata>
  <versioning>
    <snapshotVersions>
      <snapshotVersion>
        <extension>pom</extension>
        <value>1.21.11-20250101.120000-1</value>
      </snapshotVersion>
      <snapshotVersion>
        <extension>jar</extension>
        <value>1.21.11-20250101.120000-1</value>
      </snapshotVersion>
    </snapshotVersions>
  </versioning>
</metadata>
)";
        QCOMPARE(HackClients::BaritoneMaven::fabricJarNameFromMetadata(snapshotXml, QStringLiteral("1.21.11")),
                 QStringLiteral("baritone-fabric-1.21.11-20250101.120000-1.jar"));

        const QByteArray pomOnly = R"(<extension>pom</extension><value>1.21.11-20250101.120000-1</value>)";
        QCOMPARE(HackClients::BaritoneMaven::fabricJarNameFromMetadata(pomOnly, QStringLiteral("1.21.11")),
                 QStringLiteral("baritone-fabric-1.21.11.jar"));

        QCOMPARE(HackClients::BaritoneMaven::fabricJarNameFromMetadata(QByteArray(), QStringLiteral("1.20.1")),
                 QStringLiteral("baritone-fabric-1.20.1.jar"));
    }
};

QTEST_GUILESS_MAIN(HackClientsInstallTest)
#include "HackClientsInstall_test.moc"
