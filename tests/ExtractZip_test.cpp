#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

#include <FileSystem.h>
#include <archive/ArchiveWriter.h>
#include <archive/ExtractZipTask.h>
#include <tasks/Task.h>

namespace {

bool writeZip(const QString& zipPath, const QList<QPair<QString, QByteArray>>& files)
{
    MMCZip::ArchiveWriter writer(zipPath);
    if (!writer.open()) {
        return false;
    }
    for (const auto& [name, data] : files) {
        if (!writer.addFile(name, data)) {
            return false;
        }
    }
    return writer.close();
}

bool runExtract(MMCZip::ExtractZipTask& task, QString* failReason = nullptr)
{
    QEventLoop loop;
    bool succeeded = false;
    QString reason;

    QObject::connect(&task, &Task::succeeded, &loop, [&] {
        succeeded = true;
        loop.quit();
    });
    QObject::connect(&task, &Task::failed, &loop, [&](const QString& error) {
        reason = error;
        loop.quit();
    });
    QObject::connect(&task, &Task::aborted, &loop, &QEventLoop::quit);

    QTimer expire;
    expire.setSingleShot(true);
    QObject::connect(&expire, &QTimer::timeout, &loop, &QEventLoop::quit);
    expire.start(10000);

    task.start();
    loop.exec();

    if (failReason) {
        *failReason = reason;
    }
    return succeeded && expire.isActive();
}

}  // namespace

class ExtractZipTest : public QObject {
    Q_OBJECT
   private slots:
    void test_extractsNestedFile()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString zipPath = FS::PathCombine(dir.path(), QStringLiteral("ok.zip"));
        QVERIFY(writeZip(zipPath, { { QStringLiteral("foo/bar.txt"), QByteArrayLiteral("hello") } }));

        const QString out = FS::PathCombine(dir.path(), QStringLiteral("out"));
        QVERIFY(FS::ensureFolderPathExists(out));

        MMCZip::ExtractZipTask task(zipPath, QDir(out));
        QVERIFY(runExtract(task));
        QCOMPARE(QString::fromUtf8(FS::read(FS::PathCombine(out, QStringLiteral("foo/bar.txt")))), QStringLiteral("hello"));
    }

    void test_rejectsPathTraversal()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString zipPath = FS::PathCombine(dir.path(), QStringLiteral("evil.zip"));
        QVERIFY(writeZip(zipPath, { { QStringLiteral("../evil.txt"), QByteArrayLiteral("pwned") } }));

        const QString out = FS::PathCombine(dir.path(), QStringLiteral("out"));
        QVERIFY(FS::ensureFolderPathExists(out));

        QString reason;
        MMCZip::ExtractZipTask task(zipPath, QDir(out));
        QVERIFY(!runExtract(task, &reason));
        QVERIFY(reason.contains(QStringLiteral("outside of the target path")) || reason.contains(QStringLiteral("Failed")));
        QVERIFY(!QFile::exists(FS::PathCombine(dir.path(), QStringLiteral("evil.txt"))));
        QVERIFY(!QFile::exists(FS::PathCombine(out, QStringLiteral("evil.txt"))));
    }

    void test_rejectsNestedPathTraversal()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString zipPath = FS::PathCombine(dir.path(), QStringLiteral("nested-evil.zip"));
        QVERIFY(writeZip(zipPath, { { QStringLiteral("foo/../../evil.txt"), QByteArrayLiteral("pwned") } }));

        const QString out = FS::PathCombine(dir.path(), QStringLiteral("out"));
        QVERIFY(FS::ensureFolderPathExists(out));

        MMCZip::ExtractZipTask task(zipPath, QDir(out));
        QVERIFY(!runExtract(task));
        QVERIFY(!QFile::exists(FS::PathCombine(dir.path(), QStringLiteral("evil.txt"))));
    }

    void test_subdirectoryFilterSkipsOtherEntries()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString zipPath = FS::PathCombine(dir.path(), QStringLiteral("sub.zip"));
        QVERIFY(writeZip(zipPath, { { QStringLiteral("keep/a.txt"), QByteArrayLiteral("A") },
                                    { QStringLiteral("drop/b.txt"), QByteArrayLiteral("B") } }));

        const QString out = FS::PathCombine(dir.path(), QStringLiteral("out"));
        QVERIFY(FS::ensureFolderPathExists(out));

        MMCZip::ExtractZipTask task(zipPath, QDir(out), QStringLiteral("keep/"));
        QVERIFY(runExtract(task));
        QCOMPARE(QString::fromUtf8(FS::read(FS::PathCombine(out, QStringLiteral("a.txt")))), QStringLiteral("A"));
        QVERIFY(!QFile::exists(FS::PathCombine(out, QStringLiteral("drop/b.txt"))));
        QVERIFY(!QFile::exists(FS::PathCombine(out, QStringLiteral("b.txt"))));
    }

    void test_emptyArchiveSucceeds()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString zipPath = FS::PathCombine(dir.path(), QStringLiteral("empty.zip"));
        QVERIFY(writeZip(zipPath, {}));

        const QString out = FS::PathCombine(dir.path(), QStringLiteral("out"));
        QVERIFY(FS::ensureFolderPathExists(out));

        MMCZip::ExtractZipTask task(zipPath, QDir(out));
        QVERIFY(runExtract(task));
    }
};

QTEST_GUILESS_MAIN(ExtractZipTest)
#include "ExtractZip_test.moc"
