#include <QTest>

#include <minecraft/Component.h>
#include <modplatform/ModIndex.h>

class ModloaderMapTest : public QObject {
    Q_OBJECT
   private slots:
    void test_knownLoadersAndConflicts()
    {
        const auto& loaders = Component::KNOWN_MODLOADERS;

        QCOMPARE(loaders.value(QStringLiteral("net.neoforged")).type, ModPlatform::NeoForge);
        QCOMPARE(loaders.value(QStringLiteral("net.minecraftforge")).type, ModPlatform::Forge);
        QCOMPARE(loaders.value(QStringLiteral("net.fabricmc.fabric-loader")).type, ModPlatform::Fabric);
        QCOMPARE(loaders.value(QStringLiteral("org.quiltmc.quilt-loader")).type, ModPlatform::Quilt);
        QCOMPARE(loaders.value(QStringLiteral("com.mumfrey.liteloader")).type, ModPlatform::LiteLoader);

        const QStringList neoConflicts = loaders.value(QStringLiteral("net.neoforged")).knownConflictingComponents;
        QVERIFY(neoConflicts.contains(QStringLiteral("net.minecraftforge")));
        QVERIFY(neoConflicts.contains(QStringLiteral("net.fabricmc.fabric-loader")));
        QVERIFY(neoConflicts.contains(QStringLiteral("org.quiltmc.quilt-loader")));
        QVERIFY(!neoConflicts.contains(QStringLiteral("net.neoforged")));

        const QStringList forgeConflicts = loaders.value(QStringLiteral("net.minecraftforge")).knownConflictingComponents;
        QVERIFY(forgeConflicts.contains(QStringLiteral("net.neoforged")));
        QVERIFY(forgeConflicts.contains(QStringLiteral("net.fabricmc.fabric-loader")));
        QVERIFY(forgeConflicts.contains(QStringLiteral("org.quiltmc.quilt-loader")));

        const QStringList fabricConflicts = loaders.value(QStringLiteral("net.fabricmc.fabric-loader")).knownConflictingComponents;
        QVERIFY(fabricConflicts.contains(QStringLiteral("net.minecraftforge")));
        QVERIFY(fabricConflicts.contains(QStringLiteral("net.neoforged")));
        QVERIFY(fabricConflicts.contains(QStringLiteral("org.quiltmc.quilt-loader")));

        const QStringList quiltConflicts = loaders.value(QStringLiteral("org.quiltmc.quilt-loader")).knownConflictingComponents;
        QVERIFY(quiltConflicts.contains(QStringLiteral("net.fabricmc.fabric-loader")));
        QVERIFY(quiltConflicts.contains(QStringLiteral("net.minecraftforge")));
        QVERIFY(quiltConflicts.contains(QStringLiteral("net.neoforged")));

        QVERIFY(loaders.value(QStringLiteral("com.mumfrey.liteloader")).knownConflictingComponents.isEmpty());
        QVERIFY(!loaders.contains(QStringLiteral("net.minecraft")));
    }
};

QTEST_GUILESS_MAIN(ModloaderMapTest)

#include "ModloaderMap_test.moc"
