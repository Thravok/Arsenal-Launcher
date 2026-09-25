#include <QRegularExpression>
#include <QTest>
#include <QUrl>

#include <StringUtils.h>

class StringUtilsTest : public QObject {
    Q_OBJECT
   private slots:
    void test_naturalCompareOrdersNumbers()
    {
        QCOMPARE(StringUtils::naturalCompare(QStringLiteral("mod2"), QStringLiteral("mod10"), Qt::CaseSensitive), -1);
        QCOMPARE(StringUtils::naturalCompare(QStringLiteral("mod10"), QStringLiteral("mod2"), Qt::CaseSensitive), 1);
        QCOMPARE(StringUtils::naturalCompare(QStringLiteral("mod2"), QStringLiteral("mod2"), Qt::CaseSensitive), 0);
        QCOMPARE(StringUtils::naturalCompare(QStringLiteral("File2"), QStringLiteral("file10"), Qt::CaseInsensitive), -1);
    }

    void test_humanReadableFileSize()
    {
        QCOMPARE(StringUtils::humanReadableFileSize(1024.0, false), QStringLiteral("1.00 KiB"));
        QCOMPARE(StringUtils::humanReadableFileSize(1536.0, false), QStringLiteral("1.50 KiB"));
        QCOMPARE(StringUtils::humanReadableFileSize(1000.0, true), QStringLiteral("1.00 KB"));
        QCOMPARE(StringUtils::humanReadableFileSize(1000000.0, true), QStringLiteral("1.00 MB"));
        QCOMPARE(StringUtils::humanReadableFileSize(1048576.0, false), QStringLiteral("1.00 MiB"));
    }

    void test_splitFirstStringSeparator()
    {
        const auto found = StringUtils::splitFirst(QStringLiteral("key=value=extra"), QStringLiteral("="));
        QCOMPARE(found.first, QStringLiteral("key"));
        QCOMPARE(found.second, QStringLiteral("value=extra"));

        const auto missing = StringUtils::splitFirst(QStringLiteral("hello"), QStringLiteral(","));
        QCOMPARE(missing.first, QStringLiteral("hello"));
        QCOMPARE(missing.second, QStringLiteral("hello"));
    }

    void test_splitFirstCharAndRegex()
    {
        const auto byChar = StringUtils::splitFirst(QStringLiteral("left:right:more"), QChar(':'));
        QCOMPARE(byChar.first, QStringLiteral("left"));
        QCOMPARE(byChar.second, QStringLiteral("right:more"));

        const auto missingChar = StringUtils::splitFirst(QStringLiteral("hello"), QChar(','));
        QCOMPARE(missingChar.first, QStringLiteral("hello"));
        QCOMPARE(missingChar.second, QString());

        const auto byRegex = StringUtils::splitFirst(QStringLiteral("one--two--three"), QRegularExpression(QStringLiteral("-+")));
        QCOMPARE(byRegex.first, QStringLiteral("one"));
        QCOMPARE(byRegex.second, QStringLiteral("two--three"));

        const auto missingRegex = StringUtils::splitFirst(QStringLiteral("hello"), QRegularExpression(QStringLiteral(",")));
        QCOMPARE(missingRegex.first, QStringLiteral("hello"));
        QCOMPARE(missingRegex.second, QString());
    }

    void test_htmlListPatchInsertsBreakBeforeImage()
    {
        QCOMPARE(StringUtils::htmlListPatch(QStringLiteral("</ul><img src=\"x\">")), QStringLiteral("</ul><br><img src=\"x\">"));
        QCOMPARE(StringUtils::htmlListPatch(QStringLiteral("</ul>  \n<img src=\"x\">")), QStringLiteral("</ul><br>  \n<img src=\"x\">"));
        QCOMPARE(StringUtils::htmlListPatch(QStringLiteral("</ul>hello<img src=\"x\">")), QStringLiteral("</ul>hello<img src=\"x\">"));
        QCOMPARE(StringUtils::htmlListPatch(QStringLiteral("<ul><li>a</li></ul>")), QStringLiteral("<ul><li>a</li></ul>"));
    }

    void test_truncateUrlKeepsHostAndFilename()
    {
        QUrl url(QStringLiteral("https://cdn.example.com/very/long/path/to/file.jar"));
        const QString compact = StringUtils::truncateUrlHumanFriendly(url, 48);
        QVERIFY(compact.startsWith(QStringLiteral("https://cdn.example.com/")));
        QVERIFY(compact.contains(QStringLiteral("...")));
        QVERIFY(compact.endsWith(QStringLiteral("file.jar")));
        QVERIFY(compact.length() <= 48 || compact.contains(QStringLiteral("...")));

        QUrl shortUrl(QStringLiteral("https://example.com/a.jar"));
        QCOMPARE(StringUtils::truncateUrlHumanFriendly(shortUrl, 80), QStringLiteral("https://example.com/a.jar"));
    }
};

QTEST_GUILESS_MAIN(StringUtilsTest)
#include "StringUtils_test.moc"
