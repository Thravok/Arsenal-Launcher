#include <QTest>

#include <MessageLevel.h>

class MessageLevelLineTest : public QObject {
    Q_OBJECT
   private slots:
    void test_takeFromLine()
    {
        QString warning = QStringLiteral("!![WARNING]! failed to download");
        QCOMPARE(MessageLevel::takeFromLine(warning), MessageLevel::Warning);
        QCOMPARE(warning, QStringLiteral(" failed to download"));

        QString warnAlias = QStringLiteral("!![WARN]! almost");
        QCOMPARE(MessageLevel::takeFromLine(warnAlias), MessageLevel::Warning);
        QCOMPARE(warnAlias, QStringLiteral(" almost"));

        QString error = QStringLiteral("!![ERROR]! boom");
        QCOMPARE(MessageLevel::takeFromLine(error), MessageLevel::Error);
        QCOMPARE(error, QStringLiteral(" boom"));

        QString untouched = QStringLiteral("plain log line");
        QCOMPARE(MessageLevel::takeFromLine(untouched), MessageLevel::Unknown);
        QCOMPARE(untouched, QStringLiteral("plain log line"));

        QString malformed = QStringLiteral("!![ERROR] missing bang");
        QCOMPARE(MessageLevel::takeFromLine(malformed), MessageLevel::Unknown);
        QCOMPARE(malformed, QStringLiteral("!![ERROR] missing bang"));
    }

    void test_takeFromLauncherLine()
    {
        QString line = QStringLiteral("12.34 WARNING: something happened");
        QCOMPARE(MessageLevel::takeFromLauncherLine(line), MessageLevel::Warning);
        QCOMPARE(line, QStringLiteral("something happened"));

        QString error = QStringLiteral("  1 ERROR: exploded");
        QCOMPARE(MessageLevel::takeFromLauncherLine(error), MessageLevel::Error);
        QCOMPARE(error, QStringLiteral("exploded"));

        QString noLevel = QStringLiteral("just text");
        QCOMPARE(MessageLevel::takeFromLauncherLine(noLevel), MessageLevel::Unknown);
        QCOMPARE(noLevel, QStringLiteral("just text"));
    }

    void test_fromQtMsgType()
    {
        QCOMPARE(MessageLevel::fromQtMsgType(QtDebugMsg), MessageLevel::Debug);
        QCOMPARE(MessageLevel::fromQtMsgType(QtInfoMsg), MessageLevel::Info);
        QCOMPARE(MessageLevel::fromQtMsgType(QtWarningMsg), MessageLevel::Warning);
        QCOMPARE(MessageLevel::fromQtMsgType(QtCriticalMsg), MessageLevel::Error);
        QCOMPARE(MessageLevel::fromQtMsgType(QtFatalMsg), MessageLevel::Fatal);
    }
};

QTEST_GUILESS_MAIN(MessageLevelLineTest)
#include "MessageLevelLine_test.moc"
