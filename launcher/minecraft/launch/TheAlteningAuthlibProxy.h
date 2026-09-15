#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHash>
#include <QByteArray>
#include <QUrl>

#include "QObjectPtr.h"

/**
 * Local HTTP shim that presents an authlib-injector API root and proxies
 * /authserver/ and /sessionserver/ paths to The Altening's split hosts.
 * Lives for the duration of a launch (owned by the LaunchTask).
 */
class TheAlteningAuthlibProxy : public QObject {
    Q_OBJECT
public:
    using Ptr = shared_qobject_ptr<TheAlteningAuthlibProxy>;

    explicit TheAlteningAuthlibProxy(QObject *parent = nullptr);
    ~TheAlteningAuthlibProxy() override;

    bool start();
    QString baseUrl() const;
    static QByteArray prefetchedMetadataBase64();

private slots:
    void onNewConnection();
    void onClientReadyRead();
    void onClientDisconnected();
    void onUpstreamFinished();

private:
    struct ClientState {
        QByteArray buffer;
        bool headersComplete = false;
        QByteArray method;
        QByteArray path;
        QByteArray body;
        int contentLength = 0;
        QTcpSocket *upstream = nullptr;
    };

    void handleClientRequest(QTcpSocket *client);
    void respondMetadata(QTcpSocket *client);
    void proxyRequest(QTcpSocket *client, const QUrl &target, const QByteArray &method, const QByteArray &pathAndQuery, const QByteArray &body);
    static QByteArray metadataJson();

    QTcpServer m_server;
    QHash<QTcpSocket *, ClientState> m_clients;
};
