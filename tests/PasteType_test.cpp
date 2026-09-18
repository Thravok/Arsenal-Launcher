#include <QTest>

#include "net/PasteUpload.h"

class PasteTypeTest : public QObject {
    Q_OBJECT
   private slots:
    void fromStringRoundTrip_data()
    {
        QTest::addColumn<QString>("name");
        QTest::addColumn<int>("value");
        QTest::addColumn<QString>("defaultBase");
        QTest::addColumn<QString>("endpointPath");

        QTest::newRow("0x0.st") << "0x0.st" << static_cast<int>(PasteUpload::Type::NullPointer) << "https://0x0.st" << "";
        QTest::newRow("hastebin") << "hastebin" << static_cast<int>(PasteUpload::Type::Hastebin) << "https://hst.sh" << "/documents";
        QTest::newRow("paste.gg") << "paste.gg" << static_cast<int>(PasteUpload::Type::PasteGG) << "https://paste.gg" << "/api/v1/pastes";
        QTest::newRow("mclo.gs") << "mclo.gs" << static_cast<int>(PasteUpload::Type::Mclogs) << "https://api.mclo.gs" << "/1/log";
    }
    void fromStringRoundTrip()
    {
        QFETCH(QString, name);
        QFETCH(int, value);
        QFETCH(QString, defaultBase);
        QFETCH(QString, endpointPath);

        const auto parsed = PasteUpload::Type::fromString(name);
        QVERIFY(parsed.isValid());
        QCOMPARE(static_cast<int>(parsed.value()), value);
        QCOMPARE(parsed.toString(), name);
        QCOMPARE(parsed.toInt(), value);
        QCOMPARE(parsed.defaultBase(), defaultBase);
        QCOMPARE(parsed.endpointPath(), endpointPath);
        QCOMPARE(PasteUpload::Type(value).toString(), name);
    }

    void invalidValuesAreRejected()
    {
        QVERIFY(!PasteUpload::Type::fromString(QStringLiteral("unknown")).isValid());
        QVERIFY(!PasteUpload::Type::fromString(QString()).isValid());
        QVERIFY(!PasteUpload::Type(-1).isValid());
        QVERIFY(!PasteUpload::Type(99).isValid());
        QVERIFY(!PasteUpload::Type(static_cast<int>(PasteUpload::Type::Invalid)).isValid());
        QVERIFY(PasteUpload::Type(0).isValid());
        QVERIFY(PasteUpload::Type(3).isValid());
        QVERIFY(!PasteUpload::Type(4).isValid());
    }

    void storedIntMatchesSettingsContract()
    {
        QCOMPARE(static_cast<int>(PasteUpload::Type::Mclogs), 3);
        QCOMPARE(static_cast<int>(PasteUpload::Type(3).value()), static_cast<int>(PasteUpload::Type::Mclogs));
        QVERIFY(PasteUpload::Type(3).isValid());
        QCOMPARE(PasteUpload::Type(PasteUpload::Type::NullPointer).toInt(), 0);
    }
};

QTEST_GUILESS_MAIN(PasteTypeTest)
#include "PasteType_test.moc"
