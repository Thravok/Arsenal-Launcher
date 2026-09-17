#include "TheAlteningAuthlibProxy.h"
#include "minecraft/auth/TheAlteningConfig.h"

#include <QDebug>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariant>

TheAlteningAuthlibProxy::TheAlteningAuthlibProxy(QObject* parent) : QObject(parent)
{
    connect(&m_server, &QTcpServer::newConnection, this, &TheAlteningAuthlibProxy::onNewConnection);
}

TheAlteningAuthlibProxy::~TheAlteningAuthlibProxy()
{
    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        if (it->upstream) {
            it->upstream->disconnect(this);
            it->upstream->abort();
            it->upstream->deleteLater();
        }
        it.key()->disconnect(this);
        it.key()->abort();
        it.key()->deleteLater();
    }
    m_clients.clear();
    m_server.close();
}

bool TheAlteningAuthlibProxy::start()
{
    if (m_server.isListening()) {
        return true;
    }
    return m_server.listen(QHostAddress::LocalHost, 0);
}

QString TheAlteningAuthlibProxy::baseUrl() const
{
    return QStringLiteral("http://127.0.0.1:%1/").arg(m_server.serverPort());
}

QByteArray TheAlteningAuthlibProxy::metadataJson()
{
    QJsonObject meta;
    meta.insert(QStringLiteral("serverName"), QStringLiteral("The Altening"));
    meta.insert(QStringLiteral("implementationName"), QStringLiteral("thealtening-proxy"));
    meta.insert(QStringLiteral("implementationVersion"), QStringLiteral("1.0.0"));

    QJsonObject root;
    root.insert(QStringLiteral("meta"), meta);
    root.insert(QStringLiteral("skinDomains"), QJsonArray{ QStringLiteral("cdn.thealtening.com"), QStringLiteral(".thealtening.com"),
                                                           QStringLiteral("textures.minecraft.net"), QStringLiteral(".minecraft.net") });

    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

QByteArray TheAlteningAuthlibProxy::prefetchedMetadataBase64()
{
    return metadataJson().toBase64();
}

TheAlteningAuthlibProxy::ProxyRoute TheAlteningAuthlibProxy::resolveProxyRoute(const QByteArray& path)
{
    const int qmark = path.indexOf('?');
    const QByteArray pathOnly = qmark >= 0 ? path.left(qmark) : path;

    ProxyRoute route;
    if (pathOnly == "/" || pathOnly.isEmpty() || pathOnly == "/index.json") {
        route.kind = ProxyRouteKind::Metadata;
        return route;
    }

    if (pathOnly.startsWith("/authserver")) {
        route.kind = ProxyRouteKind::AuthServer;
        QByteArray upstreamPathAndQuery = path.mid(QByteArray("/authserver").size());
        if (upstreamPathAndQuery.isEmpty() || upstreamPathAndQuery.at(0) != '/')
            upstreamPathAndQuery.prepend('/');
        route.upstreamPathAndQuery = upstreamPathAndQuery;
        return route;
    }

    if (pathOnly.startsWith("/sessionserver")) {
        route.kind = ProxyRouteKind::SessionServer;
        QByteArray upstreamPathAndQuery = path.mid(QByteArray("/sessionserver").size());
        if (upstreamPathAndQuery.isEmpty() || upstreamPathAndQuery.at(0) != '/')
            upstreamPathAndQuery.prepend('/');
        route.upstreamPathAndQuery = upstreamPathAndQuery;
        return route;
    }

    route.kind = ProxyRouteKind::NotFound;
    return route;
}

void TheAlteningAuthlibProxy::onNewConnection()
{
    while (m_server.hasPendingConnections()) {
        QTcpSocket* client = m_server.nextPendingConnection();
        client->setParent(this);
        m_clients.insert(client, ClientState{});
        connect(client, &QTcpSocket::readyRead, this, &TheAlteningAuthlibProxy::onClientReadyRead);
        connect(client, &QTcpSocket::disconnected, this, &TheAlteningAuthlibProxy::onClientDisconnected);
    }
}

void TheAlteningAuthlibProxy::onClientDisconnected()
{
    auto* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) {
        return;
    }
    auto it = m_clients.find(client);
    if (it != m_clients.end()) {
        if (it->upstream) {
            it->upstream->disconnect(this);
            it->upstream->abort();
            it->upstream->deleteLater();
        }
        m_clients.erase(it);
    }
    client->deleteLater();
}

void TheAlteningAuthlibProxy::onClientReadyRead()
{
    auto* client = qobject_cast<QTcpSocket*>(sender());
    if (!client || !m_clients.contains(client)) {
        return;
    }

    auto& state = m_clients[client];
    state.buffer.append(client->readAll());

    if (!state.headersComplete) {
        const int headerEnd = state.buffer.indexOf("\r\n\r\n");
        if (headerEnd < 0) {
            return;
        }

        const QByteArray headerBlock = state.buffer.left(headerEnd);
        state.buffer.remove(0, headerEnd + 4);

        const QList<QByteArray> lines = headerBlock.split('\n');
        if (lines.isEmpty()) {
            client->disconnectFromHost();
            return;
        }

        const QList<QByteArray> requestLine = lines.first().trimmed().split(' ');
        if (requestLine.size() < 2) {
            client->disconnectFromHost();
            return;
        }
        state.method = requestLine[0].trimmed();
        state.path = requestLine[1].trimmed();
        state.contentLength = 0;

        for (int i = 1; i < lines.size(); ++i) {
            const QByteArray line = lines[i].trimmed();
            const int colon = line.indexOf(':');
            if (colon <= 0) {
                continue;
            }
            const QByteArray name = line.left(colon).trimmed().toLower();
            const QByteArray value = line.mid(colon + 1).trimmed();
            if (name == "content-length") {
                state.contentLength = value.toInt();
            }
        }
        state.headersComplete = true;
    }

    if (state.buffer.size() < state.contentLength) {
        return;
    }

    state.body = state.buffer.left(state.contentLength);
    state.buffer.clear();
    handleClientRequest(client);
}

void TheAlteningAuthlibProxy::handleClientRequest(QTcpSocket* client)
{
    auto& state = m_clients[client];
    const ProxyRoute route = resolveProxyRoute(state.path);

    if (route.kind == ProxyRouteKind::Metadata) {
        respondMetadata(client);
        return;
    }

    if (route.kind == ProxyRouteKind::AuthServer) {
        proxyRequest(client, QUrl(TheAltening::AuthServerUrl), state.method, route.upstreamPathAndQuery, state.body);
        return;
    }

    if (route.kind == ProxyRouteKind::SessionServer) {
        proxyRequest(client, QUrl(TheAltening::SessionServerUrl), state.method, route.upstreamPathAndQuery, state.body);
        return;
    }

    const QByteArray response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
    client->write(response);
    client->disconnectFromHost();
}

void TheAlteningAuthlibProxy::respondMetadata(QTcpSocket* client)
{
    const QByteArray body = metadataJson();
    QByteArray response;
    response += "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: application/json\r\n";
    response += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    response += "Connection: close\r\n\r\n";
    response += body;
    client->write(response);
    client->disconnectFromHost();
}

void TheAlteningAuthlibProxy::proxyRequest(QTcpSocket* client,
                                           const QUrl& target,
                                           const QByteArray& method,
                                           const QByteArray& pathAndQuery,
                                           const QByteArray& body)
{
    auto& state = m_clients[client];
    if (state.upstream) {
        state.upstream->disconnect(this);
        state.upstream->abort();
        state.upstream->deleteLater();
        state.upstream = nullptr;
    }

    auto* upstream = new QTcpSocket(this);
    state.upstream = upstream;
    upstream->setProperty("client", QVariant::fromValue(static_cast<void*>(client)));

    connect(upstream, &QTcpSocket::connected, this, [upstream, client, method, pathAndQuery, body, target, this]() {
        if (!m_clients.contains(client)) {
            upstream->abort();
            return;
        }
        QByteArray request;
        request += method + ' ' + pathAndQuery + " HTTP/1.1\r\n";
        request += "Host: " + target.host().toUtf8() + "\r\n";
        request += "Connection: close\r\n";
        request += "User-Agent: Arsenal-TheAltening-Proxy\r\n";
        if (!body.isEmpty()) {
            request += "Content-Type: application/json\r\n";
            request += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
        }
        request += "\r\n";
        request += body;
        upstream->write(request);
    });

    connect(upstream, &QTcpSocket::readyRead, this, &TheAlteningAuthlibProxy::onUpstreamFinished);

    auto failUpstream = [this, upstream, client]() {
        if (m_clients.contains(client)) {
            const QByteArray response = "HTTP/1.1 502 Bad Gateway\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
            client->write(response);
            client->disconnectFromHost();
            m_clients[client].upstream = nullptr;
        }
        upstream->deleteLater();
    };

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(upstream, &QAbstractSocket::errorOccurred, this, [failUpstream](QAbstractSocket::SocketError) { failUpstream(); });
#else
    connect(upstream, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this,
            [failUpstream](QAbstractSocket::SocketError) { failUpstream(); });
#endif

    connect(upstream, &QTcpSocket::disconnected, this, [this, upstream, client]() {
        if (m_clients.contains(client)) {
            // Flush any remaining data
            const QByteArray rest = upstream->readAll();
            if (!rest.isEmpty()) {
                client->write(rest);
            }
            client->disconnectFromHost();
            m_clients[client].upstream = nullptr;
        }
        upstream->deleteLater();
    });

    const quint16 port = target.port(target.scheme() == QStringLiteral("https") ? 443 : 80);
    upstream->connectToHost(target.host(), port);
}

void TheAlteningAuthlibProxy::onUpstreamFinished()
{
    auto* upstream = qobject_cast<QTcpSocket*>(sender());
    if (!upstream) {
        return;
    }

    QTcpSocket* client = static_cast<QTcpSocket*>(upstream->property("client").value<void*>());
    if (!client || !m_clients.contains(client)) {
        return;
    }

    const QByteArray data = upstream->readAll();
    if (!data.isEmpty()) {
        client->write(data);
    }
}
