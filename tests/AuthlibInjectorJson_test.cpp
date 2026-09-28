#include <QTest>

#include <minecraft/launch/ConfigureAuthlibInjector.h>

namespace {

const QByteArray kValid = R"({
  "download_url": "https://authlib-injector.yushi.moe/artifact/authlib-injector-1.2.5.jar",
  "checksums": { "sha256": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" }
})";

}  // namespace

class AuthlibInjectorJsonTest : public QObject {
    Q_OBJECT
   private slots:
    void test_parseLatestJson()
    {
        AuthlibInjector::LatestArtifact artifact;
        QString error;
        QVERIFY(AuthlibInjector::parseLatestJson(kValid, artifact, &error));
        QCOMPARE(artifact.downloadUrl, QStringLiteral("https://authlib-injector.yushi.moe/artifact/authlib-injector-1.2.5.jar"));
        QCOMPARE(artifact.sha256, QByteArray::fromHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
    }

    void test_parseLatestJsonRejectsInvalid()
    {
        AuthlibInjector::LatestArtifact artifact;
        QString error;
        QVERIFY(!AuthlibInjector::parseLatestJson(QByteArrayLiteral("not json"), artifact, &error));
        QVERIFY(error.contains(QStringLiteral("parse"), Qt::CaseInsensitive));

        QVERIFY(!AuthlibInjector::parseLatestJson(QByteArrayLiteral("[]"), artifact, &error));
        QVERIFY(error.contains(QStringLiteral("object"), Qt::CaseInsensitive));

        QVERIFY(!AuthlibInjector::parseLatestJson(QByteArrayLiteral("{\"checksums\":{\"sha256\":\"aa\"}}"), artifact, &error));
        QVERIFY(error.contains(QStringLiteral("download url"), Qt::CaseInsensitive));

        QVERIFY(!AuthlibInjector::parseLatestJson(
            QByteArrayLiteral("{\"download_url\":\"https://example.test/a.jar\"}"), artifact, &error));
        QVERIFY(error.contains(QStringLiteral("sha256"), Qt::CaseInsensitive));

        QVERIFY(!AuthlibInjector::parseLatestJson(
            QByteArrayLiteral("{\"download_url\":\"https://example.test/a.jar\",\"checksums\":{\"sha256\":\"zz\"}}"), artifact, &error));
        QVERIFY(error.contains(QStringLiteral("sha256"), Qt::CaseInsensitive));
    }
};

QTEST_GUILESS_MAIN(AuthlibInjectorJsonTest)
#include "AuthlibInjectorJson_test.moc"
