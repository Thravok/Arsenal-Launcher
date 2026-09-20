#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include "minecraft/launch/TheAlteningAuthlibProxy.h"

class TheAlteningAuthlibTest : public QObject {
    Q_OBJECT
   private slots:
    void extraJvmArgsDisableDummyProfileKey()
    {
        const QStringList args = TheAlteningAuthlibProxy::extraJvmArgs();

        QVERIFY(args.contains(QStringLiteral("-Dauthlibinjector.profileKey=disabled")));
        QVERIFY(!args.contains(QStringLiteral("-Dauthlibinjector.profileKey=enabled")));
        QVERIFY(args.contains(QStringLiteral("-Dauthlibinjector.usernameCheck=disabled")));

        bool hasPrefetched = false;
        for (const QString& arg : args) {
            if (arg.startsWith(QStringLiteral("-Dauthlibinjector.yggdrasil.prefetched="))) {
                hasPrefetched = true;
                QVERIFY(arg.size() > QStringLiteral("-Dauthlibinjector.yggdrasil.prefetched=").size());
            }
        }
        QVERIFY(hasPrefetched);
    }

    void prefetchedMetadataOmitsProfileKeyFeature()
    {
        const QByteArray json = QByteArray::fromBase64(TheAlteningAuthlibProxy::prefetchedMetadataBase64());
        const QJsonDocument doc = QJsonDocument::fromJson(json);
        QVERIFY(doc.isObject());

        const QJsonObject root = doc.object();
        const QJsonObject meta = root.value(QStringLiteral("meta")).toObject();
        QVERIFY(!meta.value(QStringLiteral("feature.enable_profile_key")).toBool(false));
        QVERIFY(!root.value(QStringLiteral("feature")).toObject().value(QStringLiteral("enable_profile_key")).toBool(false));
    }
};

QTEST_GUILESS_MAIN(TheAlteningAuthlibTest)
#include "TheAlteningAuthlib_test.moc"
