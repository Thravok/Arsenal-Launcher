#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTest>
#include <QTimer>
#include <QUrl>

#include <minecraft/launch/TheAlteningAuthlibProxy.h>

class TheAlteningAuthlibProxyTest : public QObject {
    Q_OBJECT

    static int httpGet(const QUrl& url, QByteArray* bodyOut)
    {
        QNetworkAccessManager nam;
        QEventLoop loop;
        QNetworkReply* reply = nam.get(QNetworkRequest(url));
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QTimer::singleShot(3000, &loop, &QEventLoop::quit);
        loop.exec();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (bodyOut) {
            *bodyOut = reply->readAll();
        }
        reply->deleteLater();
        return status;
    }

   private slots:
    void test_metadataJson()
    {
        const QByteArray raw = TheAlteningAuthlibProxy::metadataJson();
        const auto doc = QJsonDocument::fromJson(raw);
        QVERIFY(doc.isObject());

        const auto root = doc.object();
        const auto meta = root.value(QStringLiteral("meta")).toObject();
        QCOMPARE(meta.value(QStringLiteral("serverName")).toString(), QStringLiteral("The Altening"));
        QCOMPARE(meta.value(QStringLiteral("implementationName")).toString(), QStringLiteral("thealtening-proxy"));

        const auto domains = root.value(QStringLiteral("skinDomains")).toArray();
        QVERIFY(domains.contains(QStringLiteral("cdn.thealtening.com")));
        QVERIFY(domains.contains(QStringLiteral(".thealtening.com")));
        QVERIFY(domains.contains(QStringLiteral("textures.minecraft.net")));

        QCOMPARE(TheAlteningAuthlibProxy::prefetchedMetadataBase64(), raw.toBase64());
    }

    void test_resolveProxyRoute_data()
    {
        QTest::addColumn<QByteArray>("path");
        QTest::addColumn<int>("kind");
        QTest::addColumn<QByteArray>("upstream");

        const int metadata = static_cast<int>(TheAlteningAuthlibProxy::ProxyRouteKind::Metadata);
        const int auth = static_cast<int>(TheAlteningAuthlibProxy::ProxyRouteKind::AuthServer);
        const int session = static_cast<int>(TheAlteningAuthlibProxy::ProxyRouteKind::SessionServer);
        const int notFound = static_cast<int>(TheAlteningAuthlibProxy::ProxyRouteKind::NotFound);

        QTest::newRow("root") << QByteArray("/") << metadata << QByteArray();
        QTest::newRow("empty") << QByteArray() << metadata << QByteArray();
        QTest::newRow("index") << QByteArray("/index.json") << metadata << QByteArray();
        QTest::newRow("index query") << QByteArray("/index.json?v=1") << metadata << QByteArray();
        QTest::newRow("auth authenticate") << QByteArray("/authserver/authenticate") << auth << QByteArray("/authenticate");
        QTest::newRow("auth with query") << QByteArray("/authserver/authenticate?client=1") << auth << QByteArray("/authenticate?client=1");
        QTest::newRow("auth root") << QByteArray("/authserver") << auth << QByteArray("/");
        QTest::newRow("session profile") << QByteArray("/sessionserver/session/minecraft/profile/abcd") << session
                                         << QByteArray("/session/minecraft/profile/abcd");
        QTest::newRow("session root") << QByteArray("/sessionserver") << session << QByteArray("/");
        QTest::newRow("unknown") << QByteArray("/session/minecraft/profile/abcd") << notFound << QByteArray();
        QTest::newRow("skins") << QByteArray("/skins/abc.png") << notFound << QByteArray();
    }
    void test_resolveProxyRoute()
    {
        QFETCH(QByteArray, path);
        QFETCH(int, kind);
        QFETCH(QByteArray, upstream);

        const auto route = TheAlteningAuthlibProxy::resolveProxyRoute(path);
        QCOMPARE(static_cast<int>(route.kind), kind);
        QCOMPARE(route.upstreamPathAndQuery, upstream);
    }

    void test_localHttpMetadataAnd404()
    {
        TheAlteningAuthlibProxy proxy;
        QVERIFY(proxy.start());
        QVERIFY(proxy.start());

        const QUrl base(proxy.baseUrl());
        QVERIFY(base.port() > 0);

        QByteArray body;
        QCOMPARE(httpGet(base, &body), 200);
        const auto doc = QJsonDocument::fromJson(body);
        QVERIFY(doc.isObject());
        QCOMPARE(doc.object().value(QStringLiteral("meta")).toObject().value(QStringLiteral("serverName")).toString(),
                 QStringLiteral("The Altening"));
        QVERIFY(doc.object().value(QStringLiteral("skinDomains")).toArray().contains(QStringLiteral("cdn.thealtening.com")));

        QByteArray indexBody;
        QCOMPARE(httpGet(base.resolved(QUrl(QStringLiteral("index.json"))), &indexBody), 200);
        QCOMPARE(QJsonDocument::fromJson(indexBody), doc);

        QByteArray missingBody;
        QCOMPARE(httpGet(base.resolved(QUrl(QStringLiteral("not-a-route"))), &missingBody), 404);
        QVERIFY(missingBody.isEmpty());
    }
};

QTEST_GUILESS_MAIN(TheAlteningAuthlibProxyTest)
#include "TheAlteningAuthlibProxy_test.moc"
