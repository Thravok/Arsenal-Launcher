#include <QTest>

#include <java/JavaUtils.h>

class JavaEnvTest : public QObject {
    Q_OBJECT
   private slots:
    void test_stripVariableEntries()
    {
#ifdef Q_OS_WIN32
        const QChar delim = QLatin1Char(';');
#else
        const QChar delim = QLatin1Char(':');
#endif
        const QString path = QStringList{ QStringLiteral("/opt/a"), QStringLiteral("/opt/b"), QStringLiteral("/opt/c") }.join(delim);

        QCOMPARE(stripVariableEntries(QStringLiteral("PATH"), path, QStringLiteral("/opt/b")),
                 QStringList{ QStringLiteral("/opt/a"), QStringLiteral("/opt/c") }.join(delim));
        QCOMPARE(stripVariableEntries(QStringLiteral("PATH"), path, QString()), path);
        QCOMPARE(stripVariableEntries(QStringLiteral("PATH"), QString(), QStringLiteral("/opt/b")), QString());
        QCOMPARE(stripVariableEntries(QStringLiteral("PATH"), path, QStringLiteral("/missing")), path);
        QCOMPARE(stripVariableEntries(QStringLiteral("PATH"), path,
                                      QStringList{ QStringLiteral("/opt/a"), QStringLiteral("/opt/c") }.join(delim)),
                 QStringLiteral("/opt/b"));
    }
};

QTEST_GUILESS_MAIN(JavaEnvTest)

#include "JavaEnv_test.moc"
