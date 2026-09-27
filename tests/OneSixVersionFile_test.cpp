#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include <Json.h>
#include <minecraft/OneSixVersionFormat.h>
#include <minecraft/VersionFile.h>

namespace {

QJsonDocument docFrom(const QByteArray& json)
{
    return QJsonDocument::fromJson(json);
}

QJsonObject libraryNamed(const QString& name)
{
    QJsonObject lib;
    lib.insert(QStringLiteral("name"), name);
    return lib;
}

const QByteArray kBasic = QByteArrayLiteral(
    "{"
    "\"name\":\"Fabric Loader\","
    "\"uid\":\"net.fabricmc.fabric-loader\","
    "\"version\":\"0.16.9\""
    "}");

const QByteArray kInvalidFormat = QByteArrayLiteral("{\"formatVersion\":99,\"uid\":\"net.minecraft\"}");

const QByteArray kStringFormat = QByteArrayLiteral("{\"formatVersion\":\"1\",\"uid\":\"net.minecraft\"}");

}  // namespace

class OneSixVersionFileTest : public QObject {
    Q_OBJECT
   private slots:
    void test_rejectsEmptyOrNonObject()
    {
        QVERIFY_EXCEPTION_THROWN(OneSixVersionFormat::versionFileFromJson(QJsonDocument(), QStringLiteral("empty.json"), false),
                                 JSONValidationError);
        QVERIFY_EXCEPTION_THROWN(OneSixVersionFormat::versionFileFromJson(QJsonDocument(QJsonArray{}), QStringLiteral("array.json"), false),
                                 JSONValidationError);
    }

    void test_rejectsUnknownFormatVersion()
    {
        QVERIFY_EXCEPTION_THROWN(OneSixVersionFormat::versionFileFromJson(docFrom(kInvalidFormat), QStringLiteral("bad.json"), false),
                                 JSONValidationError);
        QVERIFY_EXCEPTION_THROWN(OneSixVersionFormat::versionFileFromJson(docFrom(kStringFormat), QStringLiteral("str.json"), false),
                                 JSONValidationError);
    }

    void test_parsesUidAndFallsBackToFileId()
    {
        auto file = OneSixVersionFormat::versionFileFromJson(docFrom(kBasic), QStringLiteral("loader.json"), false);
        QCOMPARE(file->name, QStringLiteral("Fabric Loader"));
        QCOMPARE(file->uid, QStringLiteral("net.fabricmc.fabric-loader"));
        QCOMPARE(file->version, QStringLiteral("0.16.9"));
        QCOMPARE(file->getProblemSeverity(), ProblemSeverity::None);

        QJsonObject root;
        root.insert(QStringLiteral("fileId"), QStringLiteral("net.minecraft"));
        root.insert(QStringLiteral("version"), QStringLiteral("1.21"));
        auto fallback = OneSixVersionFormat::versionFileFromJson(QJsonDocument(root), QStringLiteral("mc.json"), false);
        QCOMPARE(fallback->uid, QStringLiteral("net.minecraft"));
    }

    void test_illegalUidIsASecurityProblem()
    {
        QJsonObject root;
        root.insert(QStringLiteral("uid"), QStringLiteral("../escape"));
        auto file = OneSixVersionFormat::versionFileFromJson(QJsonDocument(root), QStringLiteral("evil.json"), false);
        QCOMPARE(file->getProblemSeverity(), ProblemSeverity::Error);
        QVERIFY(!file->getProblems().isEmpty());
        QVERIFY(file->getProblems().first().m_description.contains(QStringLiteral("illegal characters")));
    }

    void test_mcVersionAddsMinecraftRequire()
    {
        QJsonObject root;
        root.insert(QStringLiteral("uid"), QStringLiteral("net.fabricmc.fabric-loader"));
        root.insert(QStringLiteral("mcVersion"), QStringLiteral("1.21.1"));
        QJsonArray requires;
        QJsonObject fabricReq;
        fabricReq.insert(QStringLiteral("uid"), QStringLiteral("net.fabricmc.fabric-loader"));
        fabricReq.insert(QStringLiteral("equals"), QStringLiteral("0.16.9"));
        requires.append(fabricReq);
        root.insert(QStringLiteral("requires"), requires);

        auto file = OneSixVersionFormat::versionFileFromJson(QJsonDocument(root), QStringLiteral("req.json"), false);
        Meta::Require minecraft;
        minecraft.uid = QStringLiteral("net.minecraft");
        auto mcIt = file->m_requires.find(minecraft);
        QVERIFY(mcIt != file->m_requires.end());
        QCOMPARE(mcIt->equalsVersion, QStringLiteral("1.21.1"));

        Meta::Require fabric;
        fabric.uid = QStringLiteral("net.fabricmc.fabric-loader");
        QVERIFY(file->m_requires.find(fabric) != file->m_requires.end());
    }

    void test_mcVersionDoesNotDuplicateExistingMinecraftRequire()
    {
        QJsonObject root;
        root.insert(QStringLiteral("uid"), QStringLiteral("net.minecraftforge"));
        root.insert(QStringLiteral("mcVersion"), QStringLiteral("1.20.1"));
        QJsonArray requires;
        QJsonObject mcReq;
        mcReq.insert(QStringLiteral("uid"), QStringLiteral("net.minecraft"));
        mcReq.insert(QStringLiteral("equals"), QStringLiteral("1.20.1"));
        requires.append(mcReq);
        root.insert(QStringLiteral("requires"), requires);

        auto file = OneSixVersionFormat::versionFileFromJson(QJsonDocument(root), QStringLiteral("dup.json"), false);
        QCOMPARE(static_cast<int>(file->m_requires.size()), 1);
    }

    void test_parsesRuntimesAndAgents()
    {
        QJsonObject runtime;
        runtime.insert(QStringLiteral("name"), QStringLiteral("temurin-21"));
        runtime.insert(QStringLiteral("vendor"), QStringLiteral("eclipse"));
        runtime.insert(QStringLiteral("downloadType"), QStringLiteral("archive"));
        QJsonObject version;
        version.insert(QStringLiteral("major"), 21);
        version.insert(QStringLiteral("minor"), 0);
        version.insert(QStringLiteral("security"), 2);
        version.insert(QStringLiteral("build"), 0);
        version.insert(QStringLiteral("name"), QStringLiteral("21.0.2"));
        runtime.insert(QStringLiteral("version"), version);

        QJsonObject agent = libraryNamed(QStringLiteral("org.example:agent:1.2.3"));
        agent.insert(QStringLiteral("argument"), QStringLiteral("debug=true"));

        QJsonObject root;
        root.insert(QStringLiteral("uid"), QStringLiteral("net.minecraft"));
        root.insert(QStringLiteral("runtimes"), QJsonArray{ runtime });
        root.insert(QStringLiteral("+agents"), QJsonArray{ agent });
        root.insert(QStringLiteral("+traits"), QJsonArray{ QStringLiteral("FirstThreadOnMacOS") });
        root.insert(QStringLiteral("+jvmArgs"), QJsonArray{ QStringLiteral("-Xmx2G") });
        root.insert(QStringLiteral("+tweakers"), QJsonArray{ QStringLiteral("org.example.Tweaker") });

        auto file = OneSixVersionFormat::versionFileFromJson(QJsonDocument(root), QStringLiteral("rt.json"), false);
        QCOMPARE(file->runtimes.size(), 1);
        QCOMPARE(file->runtimes[0]->m_name, QStringLiteral("temurin-21"));
        QCOMPARE(file->runtimes[0]->downloadType, Java::DownloadType::Archive);
        QCOMPARE(file->runtimes[0]->version.major(), 21);
        QCOMPARE(file->agents.size(), 1);
        QCOMPARE(file->agents[0].library->rawName().serialize(), QStringLiteral("org.example:agent:1.2.3"));
        QCOMPARE(file->agents[0].argument, QStringLiteral("debug=true"));
        QVERIFY(file->traits.contains(QStringLiteral("FirstThreadOnMacOS")));
        QCOMPARE(file->addnJvmArguments, QStringList{ QStringLiteral("-Xmx2G") });
        QCOMPARE(file->addTweakers, QStringList{ QStringLiteral("org.example.Tweaker") });
    }

    void test_unsupportedElementsAreErrors()
    {
        const QStringList keys = { QStringLiteral("tweakers"), QStringLiteral("-libraries"), QStringLiteral("-tweakers"),
                                   QStringLiteral("-minecraftArguments"), QStringLiteral("+minecraftArguments") };
        for (const auto& key : keys) {
            QJsonObject root;
            root.insert(QStringLiteral("uid"), QStringLiteral("net.minecraft"));
            root.insert(key, QJsonArray{});
            auto file = OneSixVersionFormat::versionFileFromJson(QJsonDocument(root), key + QStringLiteral(".json"), false);
            QCOMPARE(file->getProblemSeverity(), ProblemSeverity::Error);
            QVERIFY(file->getProblems().constFirst().m_description.contains(key));
        }
    }

    void test_bothLibraryKeysWarnAndMerge()
    {
        QJsonObject root;
        root.insert(QStringLiteral("uid"), QStringLiteral("net.minecraft"));
        root.insert(QStringLiteral("libraries"), QJsonArray{ libraryNamed(QStringLiteral("org.example:a:1")) });
        root.insert(QStringLiteral("+libraries"), QJsonArray{ libraryNamed(QStringLiteral("org.example:b:2")) });

        auto file = OneSixVersionFormat::versionFileFromJson(QJsonDocument(root), QStringLiteral("libs.json"), false);
        QCOMPARE(file->getProblemSeverity(), ProblemSeverity::Warning);
        QCOMPARE(file->libraries.size(), 2);
        QCOMPARE(file->libraries[0]->rawName().serialize(), QStringLiteral("org.example:a:1"));
        QCOMPARE(file->libraries[1]->rawName().serialize(), QStringLiteral("org.example:b:2"));
    }

    void test_missingClientDownloadIsAnErrorWhenReconstructingMainJar()
    {
        QJsonObject root;
        root.insert(QStringLiteral("uid"), QStringLiteral("net.minecraft"));
        root.insert(QStringLiteral("id"), QStringLiteral("1.21.1"));

        auto file = OneSixVersionFormat::versionFileFromJson(QJsonDocument(root), QStringLiteral("nojars.json"), false);
        QVERIFY(file->mainJar);
        QCOMPARE(file->mainJar->rawName().serialize(), QStringLiteral("com.mojang:minecraft:1.21.1:client"));
        QCOMPARE(file->getProblemSeverity(), ProblemSeverity::Error);
        QVERIFY(file->getProblems().constFirst().m_description.contains(QStringLiteral("main jar")));
    }
};

QTEST_GUILESS_MAIN(OneSixVersionFileTest)

#include "OneSixVersionFile_test.moc"
