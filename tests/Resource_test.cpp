#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTest>

#include <FileSystem.h>
#include <minecraft/mod/MetadataHandler.h>
#include <minecraft/mod/Resource.h>

class ResourceTest : public QObject {
    Q_OBJECT
   private:
    static QString writeFile(const QTemporaryDir& dir, const QString& name)
    {
        const QString path = QDir(dir.path()).filePath(name);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write("x") != 1) {
            return {};
        }
        return path;
    }

   private slots:
    void test_parseFileTypes()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString jarPath = writeFile(dir, QStringLiteral("sodium.jar"));
        QVERIFY(!jarPath.isEmpty());
        Resource jar(jarPath);
        QCOMPARE(jar.type(), ResourceType::ZIPFILE);
        QVERIFY(jar.enabled());
        QCOMPARE(jar.name(), QStringLiteral("sodium"));
        QCOMPARE(jar.getOriginalFileName(), QStringLiteral("sodium.jar"));

        Resource zip(writeFile(dir, QStringLiteral("pack.zip")));
        QCOMPARE(zip.type(), ResourceType::ZIPFILE);
        QCOMPARE(zip.name(), QStringLiteral("pack"));

        Resource nilmod(writeFile(dir, QStringLiteral("legacy.nilmod")));
        QCOMPARE(nilmod.type(), ResourceType::ZIPFILE);
        QCOMPARE(nilmod.name(), QStringLiteral("legacy"));

        Resource litemod(writeFile(dir, QStringLiteral("optifine.litemod")));
        QCOMPARE(litemod.type(), ResourceType::LITEMOD);
        QCOMPARE(litemod.name(), QStringLiteral("optifine"));

        Resource single(writeFile(dir, QStringLiteral("readme.txt")));
        QCOMPARE(single.type(), ResourceType::SINGLEFILE);
        QCOMPARE(single.name(), QStringLiteral("readme.txt"));

        Resource disabled(writeFile(dir, QStringLiteral("sodium.jar.disabled")));
        QCOMPARE(disabled.type(), ResourceType::ZIPFILE);
        QVERIFY(!disabled.enabled());
        QCOMPARE(disabled.name(), QStringLiteral("sodium"));
        QCOMPARE(disabled.getOriginalFileName(), QStringLiteral("sodium.jar"));

        QVERIFY(QDir(dir.path()).mkdir(QStringLiteral("resourcepack")));
        Resource folder(QDir(dir.path()).filePath(QStringLiteral("resourcepack")));
        QCOMPARE(folder.type(), ResourceType::FOLDER);
        QVERIFY(folder.enabled());
        QVERIFY(!folder.enable(EnableAction::DISABLE));

        Resource missing(QStringLiteral("/no/such/mod.jar"));
        QCOMPARE(missing.type(), ResourceType::UNKNOWN);
        QVERIFY(!missing.valid());
        QVERIFY(!missing.enable(EnableAction::TOGGLE));
    }

    void test_enableDisableAndCollision()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString jarPath = writeFile(dir, QStringLiteral("mod.jar"));
        QVERIFY(!jarPath.isEmpty());
        Resource resource(jarPath);
        QVERIFY(resource.enable(EnableAction::DISABLE));
        QVERIFY(!resource.enabled());
        QVERIFY(resource.fileinfo().fileName().endsWith(QStringLiteral(".disabled")));
        QVERIFY(!resource.enable(EnableAction::DISABLE));

        QVERIFY(resource.enable(EnableAction::ENABLE));
        QVERIFY(resource.enabled());
        QCOMPARE(resource.fileinfo().fileName(), QStringLiteral("mod.jar"));
        QVERIFY(!resource.enable(EnableAction::ENABLE));

        QVERIFY(resource.enable(EnableAction::TOGGLE));
        QVERIFY(!resource.enabled());
        QVERIFY(resource.enable(EnableAction::TOGGLE));
        QVERIFY(resource.enabled());

        writeFile(dir, QStringLiteral("mod.jar.disabled"));
        QVERIFY(resource.enable(EnableAction::DISABLE));
        QVERIFY(resource.fileinfo().fileName().contains(QStringLiteral("duplicate")));
        QVERIFY(QFile::exists(jarPath));
    }

    void test_filterAndMetadataStatus()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        Resource resource(writeFile(dir, QStringLiteral("the sodium.jar")));
        QVERIFY(resource.applyFilter(QRegularExpression(QStringLiteral("sodium"))));
        QVERIFY(resource.applyFilter(QRegularExpression(QStringLiteral("the sodium\\.jar"))));
        QVERIFY(!resource.applyFilter(QRegularExpression(QStringLiteral("iris"))));

        Resource other(writeFile(dir, QStringLiteral("iris.jar")));
        QVERIFY(resource.compare(other, SortType::Name) > 0);

        QCOMPARE(resource.status(), ResourceStatus::Unknown);
        Metadata::ModStruct meta;
        meta.name = QStringLiteral("Sodium");
        meta.provider = ModPlatform::ResourceProvider::MODRINTH;
        resource.setMetadata(meta);
        QCOMPARE(resource.status(), ResourceStatus::Unknown);
        QCOMPARE(resource.name(), QStringLiteral("Sodium"));

        resource.setStatus(ResourceStatus::NoMetadata);
        resource.setMetadata(meta);
        QCOMPARE(resource.status(), ResourceStatus::Installed);
    }

    void test_uniqueDisabledResourceName()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString enabled = writeFile(dir, QStringLiteral("mod.jar"));
        QVERIFY(!enabled.isEmpty());
        QCOMPARE(FS::getUniqueResourceName(enabled), enabled);

        const QString disabledOnly = QDir(dir.path()).filePath(QStringLiteral("other.jar.disabled"));
        QCOMPARE(FS::getUniqueResourceName(disabledOnly), disabledOnly);

        const QString colliding = QDir(dir.path()).filePath(QStringLiteral("mod.jar.disabled"));
        const QString unique = FS::getUniqueResourceName(colliding);
        QCOMPARE(unique, QDir(dir.path()).filePath(QStringLiteral("mod.jar.duplicate")));
        QVERIFY(!QFile::exists(unique));

        writeFile(dir, QStringLiteral("mod.jar.duplicate"));
        QCOMPARE(FS::getUniqueResourceName(colliding), QDir(dir.path()).filePath(QStringLiteral("mod.jar.duplicate2")));
    }
};

QTEST_GUILESS_MAIN(ResourceTest)

#include "Resource_test.moc"
