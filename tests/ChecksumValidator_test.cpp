#include <QCryptographicHash>
#include <QTest>

#include "net/ChecksumValidator.h"

class ChecksumValidatorTest : public QObject {
    Q_OBJECT
   private slots:
    void matchingSha1Succeeds()
    {
        const QByteArray payload = QByteArrayLiteral("hello");
        const QByteArray expected = QCryptographicHash::hash(payload, QCryptographicHash::Sha1);
        Net::ChecksumValidator validator(QCryptographicHash::Sha1, expected);
        validator.init();
        validator.write(payload);
        const auto result = validator.validate();
        QVERIFY(result.has_value());
        QCOMPARE(validator.hash(), expected);
    }

    void matchingHexConstructorSucceeds()
    {
        const QByteArray payload = QByteArrayLiteral("hello");
        Net::ChecksumValidator validator(QCryptographicHash::Sha1, QStringLiteral("aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d"));
        validator.init();
        validator.write(payload);
        QVERIFY(validator.validate().has_value());
    }

    void mismatchReturnsExpectedError()
    {
        const QByteArray expected = QCryptographicHash::hash(QByteArrayLiteral("hello"), QCryptographicHash::Sha1);
        Net::ChecksumValidator validator(QCryptographicHash::Sha1, expected);
        validator.init();
        validator.write(QByteArrayLiteral("goodbye"));
        const auto result = validator.validate();
        QVERIFY(!result.has_value());
        QVERIFY(result.error().contains(QStringLiteral("Checksum mismatch")));
        QVERIFY(result.error().contains(QString::fromLatin1(expected.toHex())));
        QVERIFY(result.error().contains(QString::fromLatin1(validator.hash().toHex())));
    }

    void emptyExpectedAlwaysSucceeds()
    {
        Net::ChecksumValidator validator(QCryptographicHash::Sha1);
        validator.init();
        validator.write(QByteArrayLiteral("anything"));
        QVERIFY(validator.validate().has_value());
    }

    void initAndAbortResetHash()
    {
        Net::ChecksumValidator validator(QCryptographicHash::Sha1, QStringLiteral("aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d"));
        validator.init();
        validator.write(QByteArrayLiteral("hel"));
        validator.abort();
        validator.init();
        validator.write(QByteArrayLiteral("hello"));
        QVERIFY(validator.validate().has_value());
    }

    void chunkedWritesMatchSingleWrite()
    {
        const QByteArray payload = QByteArrayLiteral("hello world");
        const QByteArray expected = QCryptographicHash::hash(payload, QCryptographicHash::Sha1);

        Net::ChecksumValidator whole(QCryptographicHash::Sha1, expected);
        whole.init();
        whole.write(payload);

        Net::ChecksumValidator parts(QCryptographicHash::Sha1, expected);
        parts.init();
        parts.write(QByteArrayLiteral("hello"));
        parts.write(QByteArrayLiteral(" world"));

        QCOMPARE(whole.hash(), parts.hash());
        QVERIFY(parts.validate().has_value());
    }
};

QTEST_GUILESS_MAIN(ChecksumValidatorTest)
#include "ChecksumValidator_test.moc"
