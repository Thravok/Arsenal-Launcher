#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include <Exception.h>
#include <java/JavaMetadata.h>
#include <minecraft/ParseUtils.h>

namespace {

QJsonObject objectFrom(const QByteArray& json)
{
    return QJsonDocument::fromJson(json).object();
}

const QByteArray kFullMeta = QByteArrayLiteral(
    "{"
    "\"name\":\"temurin-21\","
    "\"vendor\":\"eclipse\","
    "\"url\":\"https://example.test/java.tar.gz\","
    "\"releaseTime\":\"2024-01-15T12:00:00+00:00\","
    "\"downloadType\":\"manifest\","
    "\"packageType\":\"jre\","
    "\"runtimeOS\":\"linux\","
    "\"checksum\":{\"hash\":\"abc123\",\"type\":\"sha256\"},"
    "\"version\":{\"name\":\"21.0.2\",\"major\":21,\"minor\":0,\"security\":2,\"build\":7}"
    "}");

const QByteArray kMinimalMeta = QByteArrayLiteral("{\"name\":\"plain\"}");

const QByteArray kBadChecksum = QByteArrayLiteral("{\"name\":\"bad\",\"checksum\":\"not-an-object\"}");

}  // namespace

class JavaMetadataTest : public QObject {
    Q_OBJECT
   private slots:
    void test_parseDownloadType()
    {
        QCOMPARE(Java::parseDownloadType(QStringLiteral("manifest")), Java::DownloadType::Manifest);
        QCOMPARE(Java::parseDownloadType(QStringLiteral("archive")), Java::DownloadType::Archive);
        QCOMPARE(Java::parseDownloadType(QStringLiteral("bundle")), Java::DownloadType::Unknown);
        QCOMPARE(Java::parseDownloadType(QString()), Java::DownloadType::Unknown);
        QCOMPARE(Java::downloadTypeToString(Java::DownloadType::Manifest), QStringLiteral("manifest"));
        QCOMPARE(Java::downloadTypeToString(Java::DownloadType::Archive), QStringLiteral("archive"));
        QCOMPARE(Java::downloadTypeToString(Java::DownloadType::Unknown), QStringLiteral("unknown"));
    }

    void test_parseJavaMetaFull()
    {
        auto meta = Java::parseJavaMeta(objectFrom(kFullMeta));
        QVERIFY(meta);
        QCOMPARE(meta->m_name, QStringLiteral("temurin-21"));
        QCOMPARE(meta->vendor, QStringLiteral("eclipse"));
        QCOMPARE(meta->url, QStringLiteral("https://example.test/java.tar.gz"));
        QCOMPARE(meta->releaseTime, timeFromS3Time(QStringLiteral("2024-01-15T12:00:00+00:00")));
        QCOMPARE(meta->downloadType, Java::DownloadType::Manifest);
        QCOMPARE(meta->packageType, QStringLiteral("jre"));
        QCOMPARE(meta->runtimeOS, QStringLiteral("linux"));
        QCOMPARE(meta->checksumHash, QStringLiteral("abc123"));
        QCOMPARE(meta->checksumType, QStringLiteral("sha256"));
        QCOMPARE(meta->version.major(), 21);
        QCOMPARE(meta->version.minor(), 0);
        QCOMPARE(meta->version.security(), 2);
        QCOMPARE(meta->version.name(), QStringLiteral("21.0.2"));
        QCOMPARE(meta->descriptor(), meta->version.toString());
        QCOMPARE(meta->name(), QStringLiteral("temurin-21"));
        QCOMPARE(meta->typeString(), QStringLiteral("eclipse"));
    }

    void test_parseJavaMetaDefaults()
    {
        auto meta = Java::parseJavaMeta(objectFrom(kMinimalMeta));
        QVERIFY(meta);
        QCOMPARE(meta->m_name, QStringLiteral("plain"));
        QVERIFY(meta->vendor.isEmpty());
        QVERIFY(meta->url.isEmpty());
        QVERIFY(!meta->releaseTime.isValid());
        QCOMPARE(meta->downloadType, Java::DownloadType::Unknown);
        QVERIFY(meta->packageType.isEmpty());
        QCOMPARE(meta->runtimeOS, QStringLiteral("unknown"));
        QVERIFY(meta->checksumHash.isEmpty());
        QCOMPARE(meta->version.major(), 0);
    }

    void test_parseJavaMetaRejectsNonObjectChecksum()
    {
        QVERIFY_EXCEPTION_THROWN(Java::parseJavaMeta(objectFrom(kBadChecksum)), Exception);
    }

    void test_metadataOrdering()
    {
        auto older = Java::parseJavaMeta(objectFrom(kFullMeta));
        QJsonObject newerObj = objectFrom(kFullMeta);
        QJsonObject version = newerObj.value(QStringLiteral("version")).toObject();
        version.insert(QStringLiteral("major"), 22);
        newerObj.insert(QStringLiteral("version"), version);
        auto newer = Java::parseJavaMeta(newerObj);

        QVERIFY(*older < *newer);
        QVERIFY(*newer > *older);
        QVERIFY(!(*older == *newer));

        QJsonObject laterSameVersion = objectFrom(kFullMeta);
        laterSameVersion.insert(QStringLiteral("releaseTime"), QStringLiteral("2025-01-15T12:00:00+00:00"));
        laterSameVersion.insert(QStringLiteral("name"), QStringLiteral("temurin-21"));
        auto later = Java::parseJavaMeta(laterSameVersion);
        QVERIFY(*older < *later);

        QJsonObject sameVersionEarlierName = objectFrom(kFullMeta);
        sameVersionEarlierName.insert(QStringLiteral("name"), QStringLiteral("adoptium-21"));
        auto namedEarlier = Java::parseJavaMeta(sameVersionEarlierName);
        QVERIFY(*namedEarlier < *older);

        auto copy = Java::parseJavaMeta(objectFrom(kFullMeta));
        QVERIFY(*older == *copy);
    }
};

QTEST_GUILESS_MAIN(JavaMetadataTest)

#include "JavaMetadata_test.moc"
