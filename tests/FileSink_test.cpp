#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTemporaryDir>
#include <QTest>

#include <net/ByteArraySink.h>
#include <net/ChecksumValidator.h>
#include <net/FileSink.h>

namespace {

class FakeReply : public QNetworkReply {
   public:
    explicit FakeReply(int status, QObject* parent = nullptr) : QNetworkReply(parent)
    {
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, status);
        open(ReadOnly);
    }

    void abort() override {}

   protected:
    qint64 readData(char* /*data*/, qint64 /*maxlen*/) override { return -1; }
};

QString destIn(const QTemporaryDir& dir, const QString& name)
{
    return dir.filePath(name);
}

}  // namespace

class FileSinkTest : public QObject {
    Q_OBJECT

   private slots:
    void test_byteArraySinkPropagatesChecksumMismatch()
    {
        Net::ByteArraySink sink;
        QNetworkRequest request;
        QVERIFY(sink.init(request).has_value());

        const QByteArray payload = QByteArrayLiteral("payload");
        sink.addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha1, QByteArray::fromHex("deadbeef")));
        QVERIFY(sink.write(payload).has_value());
        QCOMPARE(*sink.output(), payload);

        FakeReply reply(200);
        const auto result = sink.finalize(reply);
        QVERIFY(!result);
        QVERIFY(result.error().contains(QStringLiteral("Checksum mismatch")));
    }

    void test_byteArraySinkAcceptsMatchingChecksum()
    {
        Net::ByteArraySink sink;
        QNetworkRequest request;
        sink.addValidator(
            new Net::ChecksumValidator(QCryptographicHash::Sha1, QByteArray::fromHex("7a85f4764bbd6daf1c3545efbbf0f279a6dc0beb")));
        QVERIFY(sink.init(request).has_value());
        QVERIFY(sink.write(QByteArrayLiteral("ok")).has_value());

        FakeReply reply(200);
        QVERIFY(sink.finalize(reply).has_value());
        QCOMPARE(*sink.output(), QByteArrayLiteral("ok"));
    }

    void test_fileSinkCommitsOnHttp200()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString dest = destIn(dir, QStringLiteral("ok.jar"));

        Net::FileSink sink(dest);
        QNetworkRequest request;
        QVERIFY(sink.init(request).has_value());
        QVERIFY(sink.write(QByteArrayLiteral("jar-bytes")).has_value());

        FakeReply reply(200);
        QVERIFY(sink.finalize(reply).has_value());

        QFile file(dest);
        QVERIFY(file.exists());
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArrayLiteral("jar-bytes"));
        QVERIFY(sink.hasLocalData());
    }

    void test_fileSinkDoesNotCommitOnValidatorFailure()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString dest = destIn(dir, QStringLiteral("bad.jar"));

        Net::FileSink sink(dest);
        sink.addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha1, QByteArray::fromHex("00")));
        QNetworkRequest request;
        QVERIFY(sink.init(request).has_value());
        QVERIFY(sink.write(QByteArrayLiteral("tampered")).has_value());

        FakeReply reply(200);
        const auto result = sink.finalize(reply);
        QVERIFY(!result);
        QVERIFY(result.error().contains(QStringLiteral("Checksum mismatch")));
        QVERIFY(!QFile::exists(dest));
        QVERIFY(!sink.hasLocalData());
    }

    void test_fileSinkAbortDiscardsPartialDownload()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString dest = destIn(dir, QStringLiteral("partial.jar"));

        Net::FileSink sink(dest);
        QNetworkRequest request;
        QVERIFY(sink.init(request).has_value());
        QVERIFY(sink.write(QByteArrayLiteral("partial")).has_value());
        sink.abort();

        QVERIFY(!QFile::exists(dest));
        QVERIFY(!sink.hasLocalData());
    }

    void test_fileSinkIgnoresNotModifiedWithNoBytes()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString dest = destIn(dir, QStringLiteral("cached.jar"));

        Net::FileSink sink(dest);
        QNetworkRequest request;
        QVERIFY(sink.init(request).has_value());

        FakeReply reply(304);
        QVERIFY(sink.finalize(reply).has_value());
        QVERIFY(!QFile::exists(dest));
        QVERIFY(!sink.hasLocalData());
    }
};

QTEST_GUILESS_MAIN(FileSinkTest)

#include "FileSink_test.moc"
