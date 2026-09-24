#include <QTest>

#include <net/NetUtils.h>

class NetUtilsTest : public QObject {
    Q_OBJECT
   private slots:
    void test_applicationVersusServerErrors()
    {
        using E = QNetworkReply::NetworkError;

        QVERIFY(Net::isApplicationError(E::ContentNotFoundError));
        QVERIFY(!Net::isServerError(E::ContentNotFoundError));

        QVERIFY(Net::isApplicationError(E::AuthenticationRequiredError));
        QVERIFY(!Net::isServerError(E::AuthenticationRequiredError));

        QVERIFY(Net::isApplicationError(E::ContentGoneError));
        QVERIFY(!Net::isServerError(E::ContentGoneError));

        QVERIFY(Net::isApplicationError(E::InternalServerError));
        QVERIFY(Net::isServerError(E::InternalServerError));

        QVERIFY(Net::isApplicationError(E::ServiceUnavailableError));
        QVERIFY(Net::isServerError(E::ServiceUnavailableError));

        QVERIFY(Net::isApplicationError(E::UnknownServerError));
        QVERIFY(Net::isServerError(E::UnknownServerError));

        QVERIFY(!Net::isApplicationError(E::TimeoutError));
        QVERIFY(!Net::isServerError(E::TimeoutError));
        QVERIFY(!Net::isApplicationError(E::ConnectionRefusedError));
        QVERIFY(!Net::isServerError(E::ConnectionRefusedError));
        QVERIFY(!Net::isApplicationError(E::NoError));
        QVERIFY(!Net::isServerError(E::NoError));
    }
};

QTEST_GUILESS_MAIN(NetUtilsTest)
#include "NetUtils_test.moc"
