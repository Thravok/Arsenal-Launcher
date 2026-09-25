#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

#include <Application.h>
#include <FileSystem.h>

class ImportUrlTest : public QObject {
    Q_OBJECT
   private slots:
    void test_existingLocalFileBecomesAbsoluteFileUrl()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString path = FS::PathCombine(dir.path(), QStringLiteral("sub/pack.mrpack"));
        FS::write(path, QByteArrayLiteral("pack"));

        const QString dotted = FS::PathCombine(dir.path(), QStringLiteral("sub/../sub/pack.mrpack"));
        const QUrl url = Application::normalizeImportUrl(dotted);
        QVERIFY(url.isLocalFile());
        QCOMPARE(QFileInfo(url.toLocalFile()).canonicalFilePath(), QFileInfo(path).canonicalFilePath());
    }

    void test_httpsUrlIsPreserved()
    {
        const QUrl url = Application::normalizeImportUrl(QStringLiteral("https://modrinth.com/modpack/example"));
        QVERIFY(url.isValid());
        QCOMPARE(url.scheme(), QStringLiteral("https"));
        QCOMPARE(url.host(), QStringLiteral("modrinth.com"));
        QCOMPARE(url.path(), QStringLiteral("/modpack/example"));
    }

    void test_customSchemeFromUserInput()
    {
        const QUrl url = Application::normalizeImportUrl(QStringLiteral("curseforge://install?addonId=42"));
        QVERIFY(url.isValid());
        QCOMPARE(url.scheme(), QStringLiteral("curseforge"));
        QVERIFY(url.toString().contains(QStringLiteral("addonId=42")));
    }
};

QTEST_GUILESS_MAIN(ImportUrlTest)
#include "ImportUrl_test.moc"
