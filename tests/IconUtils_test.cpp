#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include "icons/IconUtils.h"

class IconUtilsTest : public QObject {
    Q_OBJECT
   private:
    static bool writeFile(const QString& path)
    {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            return false;
        }
        file.write("x");
        return true;
    }

   private slots:
    void isIconSuffix()
    {
        QVERIFY(IconUtils::isIconSuffix(QStringLiteral("png")));
        QVERIFY(IconUtils::isIconSuffix(QStringLiteral("svg")));
        QVERIFY(IconUtils::isIconSuffix(QStringLiteral("webp")));
        QVERIFY(!IconUtils::isIconSuffix(QStringLiteral("txt")));
        QVERIFY(!IconUtils::isIconSuffix(QStringLiteral("PNG")));
        QVERIFY(!IconUtils::isIconSuffix(QString()));
        QVERIFY(IconUtils::getIconFilter().contains(QStringLiteral("*.png")));
    }

    void findBestIconInMatchesKey()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(writeFile(dir.filePath(QStringLiteral("grass.png"))));
        QVERIFY(writeFile(dir.filePath(QStringLiteral("dirt.txt"))));

        QCOMPARE(IconUtils::findBestIconIn(dir.path(), QStringLiteral("grass")), dir.filePath(QStringLiteral("grass.png")));
        QVERIFY(IconUtils::findBestIconIn(dir.path(), QStringLiteral("dirt")).isEmpty());
        QVERIFY(IconUtils::findBestIconIn(dir.path(), QStringLiteral("missing")).isEmpty());
    }

    void listIconFilesIncludesRootAndImmediateSubdirsOnly()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QDir root(dir.path());
        QVERIFY(root.mkpath(QStringLiteral("packs/nested")));

        const QString rootIcon = dir.filePath(QStringLiteral("root.png"));
        const QString subIcon = dir.filePath(QStringLiteral("packs/pack.png"));
        const QString nestedIcon = dir.filePath(QStringLiteral("packs/nested/deep.png"));
        QVERIFY(writeFile(rootIcon));
        QVERIFY(writeFile(subIcon));
        QVERIFY(writeFile(nestedIcon));

        const QStringList files = IconUtils::listIconFiles(QDir(dir.path()));
        QCOMPARE(files.size(), 2);
        QVERIFY(files.contains(QFileInfo(rootIcon).absoluteFilePath()));
        QVERIFY(files.contains(QFileInfo(subIcon).absoluteFilePath()));
        QVERIFY(!files.contains(QFileInfo(nestedIcon).absoluteFilePath()));
    }
};

QTEST_GUILESS_MAIN(IconUtilsTest)
#include "IconUtils_test.moc"
