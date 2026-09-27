#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

#include <FileSystem.h>

class FileSystemIOTest : public QObject {
    Q_OBJECT
   private slots:
    void test_writeReadRoundTripCreatesParents()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString path = FS::PathCombine(temp.path(), QStringLiteral("nested"), QStringLiteral("dir"), QStringLiteral("data.txt"));

        FS::write(path, QByteArrayLiteral("hello"));
        QCOMPARE(FS::read(path), QByteArrayLiteral("hello"));
        QVERIFY(QFileInfo::exists(QFileInfo(path).path()));
    }

    void test_readMissingFileThrows()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY_EXCEPTION_THROWN(FS::read(FS::PathCombine(temp.path(), QStringLiteral("missing.bin"))), FS::FileSystemException);
    }

    void test_appendAndAppendSafe()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString path = FS::PathCombine(temp.path(), QStringLiteral("log.txt"));

        FS::appendSafe(path, QByteArrayLiteral("one"));
        QCOMPARE(FS::read(path), QByteArrayLiteral("one"));
        FS::appendSafe(path, QByteArrayLiteral("-two"));
        QCOMPARE(FS::read(path), QByteArrayLiteral("one-two"));

        FS::append(path, QByteArrayLiteral("-three"));
        QCOMPARE(FS::read(path), QByteArrayLiteral("one-two-three"));
    }

    void test_ensurePaths()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString filePath = FS::PathCombine(temp.path(), QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c.txt"));
        QVERIFY(FS::ensureFilePathExists(filePath));
        QVERIFY(QDir(FS::PathCombine(temp.path(), QStringLiteral("a"), QStringLiteral("b"))).exists());
        QVERIFY(!QFileInfo::exists(filePath));

        const QString folderPath = FS::PathCombine(temp.path(), QStringLiteral("created"), QStringLiteral("folder"));
        QVERIFY(FS::ensureFolderPathExists(folderPath));
        QVERIFY(QDir(folderPath).exists());
        QVERIFY(FS::ensureFolderPathExists(folderPath));
    }

    void test_moveAndDelete()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString src = FS::PathCombine(temp.path(), QStringLiteral("src.txt"));
        const QString dest = FS::PathCombine(temp.path(), QStringLiteral("nested"), QStringLiteral("dest.txt"));
        FS::write(src, QByteArrayLiteral("payload"));
        QVERIFY(FS::move(src, dest));
        QVERIFY(!QFileInfo::exists(src));
        QCOMPARE(FS::read(dest), QByteArrayLiteral("payload"));

        const QString folder = FS::PathCombine(temp.path(), QStringLiteral("tree"));
        FS::write(FS::PathCombine(folder, QStringLiteral("keep-parent"), QStringLiteral("child.txt")), QByteArrayLiteral("x"));
        QVERIFY(FS::deleteContents(folder));
        QVERIFY(QDir(folder).exists());
        QVERIFY(QDir(folder).entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty());

        QVERIFY(FS::deletePath(folder));
        QVERIFY(!QDir(folder).exists());
        QVERIFY(FS::deleteContents(FS::PathCombine(temp.path(), QStringLiteral("missing-dir"))));
        QVERIFY(!FS::deleteContents(dest));
    }
};

QTEST_GUILESS_MAIN(FileSystemIOTest)

#include "FileSystemIO_test.moc"
