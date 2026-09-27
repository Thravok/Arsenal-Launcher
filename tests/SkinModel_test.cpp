#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QJsonObject>
#include <QSize>
#include <QTemporaryDir>
#include <QTest>

#include <FileSystem.h>
#include <minecraft/skins/SkinModel.h>

class SkinModelTest : public QObject {
    Q_OBJECT
    static bool writePng(const QString& path, int width, int height)
    {
        QImage image(width, height, QImage::Format_ARGB32);
        image.fill(Qt::red);
        return image.save(path, "PNG");
    }

   private slots:
    void test_jsonModelMapping()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        QJsonObject slim;
        slim.insert(QStringLiteral("name"), QStringLiteral("alex"));
        slim.insert(QStringLiteral("model"), QStringLiteral("SLIM"));
        slim.insert(QStringLiteral("capeId"), QStringLiteral("cape-1"));
        slim.insert(QStringLiteral("url"), QStringLiteral("https://example.test/alex.png"));
        SkinModel slimSkin(QDir(dir.path()), slim);
        QCOMPARE(slimSkin.name(), QStringLiteral("alex"));
        QCOMPARE(slimSkin.getModel(), SkinModel::SLIM);
        QCOMPARE(slimSkin.getModelString(), QStringLiteral("SLIM"));
        QCOMPARE(slimSkin.getCapeId(), QStringLiteral("cape-1"));
        QCOMPARE(slimSkin.getURL(), QStringLiteral("https://example.test/alex.png"));
        QVERIFY(!slimSkin.isValid());

        QJsonObject written = slimSkin.toJSON();
        QCOMPARE(written.value(QStringLiteral("name")).toString(), QStringLiteral("alex"));
        QCOMPARE(written.value(QStringLiteral("model")).toString(), QStringLiteral("SLIM"));
        QCOMPARE(written.value(QStringLiteral("capeId")).toString(), QStringLiteral("cape-1"));
        QCOMPARE(written.value(QStringLiteral("url")).toString(), QStringLiteral("https://example.test/alex.png"));

        QJsonObject classic;
        classic.insert(QStringLiteral("name"), QStringLiteral("steve"));
        SkinModel classicSkin(QDir(dir.path()), classic);
        QCOMPARE(classicSkin.getModel(), SkinModel::CLASSIC);
        QCOMPARE(classicSkin.getModelString(), QStringLiteral("CLASSIC"));
        QCOMPARE(classicSkin.toJSON().value(QStringLiteral("model")).toString(), QStringLiteral("CLASSIC"));
    }

    void test_validModernAndLegacySkins()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString modern = FS::PathCombine(dir.path(), QStringLiteral("modern.png"));
        QVERIFY(writePng(modern, 64, 64));
        SkinModel modernSkin(modern);
        QVERIFY(modernSkin.isValid());
        QCOMPARE(modernSkin.name(), QStringLiteral("modern"));
        QCOMPARE(modernSkin.getTexture().size(), QSize(64, 64));

        const QString legacy = FS::PathCombine(dir.path(), QStringLiteral("legacy.png"));
        QVERIFY(writePng(legacy, 64, 32));
        SkinModel legacySkin(legacy);
        QVERIFY(legacySkin.isValid());
        QCOMPARE(legacySkin.getTexture().size(), QSize(64, 64));

        const QString invalid = FS::PathCombine(dir.path(), QStringLiteral("tiny.png"));
        QVERIFY(writePng(invalid, 32, 32));
        SkinModel invalidSkin(invalid);
        QVERIFY(!invalidSkin.isValid());
    }

    void test_renameRejectsExistingTarget()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString first = FS::PathCombine(dir.path(), QStringLiteral("one.png"));
        const QString second = FS::PathCombine(dir.path(), QStringLiteral("two.png"));
        QVERIFY(writePng(first, 64, 64));
        QVERIFY(writePng(second, 64, 64));

        SkinModel skin(first);
        QVERIFY(!skin.rename(QStringLiteral("two")));
        QCOMPARE(skin.getPath(), first);

        QVERIFY(skin.rename(QStringLiteral("renamed")));
        QCOMPARE(QFileInfo(skin.getPath()).fileName(), QStringLiteral("renamed.png"));
        QVERIFY(QFileInfo::exists(skin.getPath()));
        QVERIFY(!QFileInfo::exists(first));
    }
};

QTEST_GUILESS_MAIN(SkinModelTest)

#include "SkinModel_test.moc"
