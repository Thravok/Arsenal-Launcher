#include <QTest>

#include <minecraft/auth/AuthSession.h>

class AuthSessionTest : public QObject {
    Q_OBJECT
   private slots:
    void test_makeOffline()
    {
        AuthSession session;
        session.access_token = QStringLiteral("stale");
        session.session = QStringLiteral("token:stale:id");
        QVERIFY(session.MakeOffline(QStringLiteral("LocalPlayer")));
        QCOMPARE(session.player_name, QStringLiteral("LocalPlayer"));
        QCOMPARE(session.access_token, QStringLiteral("0"));
        QCOMPARE(session.session, QStringLiteral("-"));
    }

    void test_makeDemo()
    {
        AuthSession session;
        session.MakeDemo(QStringLiteral("DemoUser"), QStringLiteral("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
        QCOMPARE(session.player_name, QStringLiteral("DemoUser"));
        QCOMPARE(session.uuid, QStringLiteral("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
        QCOMPARE(session.access_token, QStringLiteral("0"));
        QCOMPARE(session.session, QStringLiteral("-"));
        QCOMPARE(session.launchMode, LaunchMode::Demo);
    }

    void test_serializeUserPropertiesIsEmptyObject()
    {
        AuthSession session;
        QCOMPARE(session.serializeUserProperties(), QStringLiteral("{}"));
    }
};

QTEST_GUILESS_MAIN(AuthSessionTest)
#include "AuthSession_test.moc"
