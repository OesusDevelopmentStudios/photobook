/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <memory>

#include <QNetworkReply>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>

#include "NetworkAccess.hpp"

namespace pictorio::network_access
{

namespace
{

// An HTTP server on this computer that answers every request with the same thumbnail, and remembers
// the requests it got.
class ThumbnailServer : public QTcpServer
{
public:
    ThumbnailServer()
    {
        connect(this, &QTcpServer::newConnection, this, [this]() {
            QTcpSocket* socket = nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, socket, [this, socket]() {
                m_partialRequest += socket->readAll();
                if (!m_partialRequest.contains("\r\n\r\n"))
                {
                    return;  // the rest of the request is still on its way
                }

                requests.append(std::exchange(m_partialRequest, {}));
                const QByteArray body = "webp bytes";
                socket->write("HTTP/1.1 200 OK\r\nContent-Type: image/webp\r\n"
                              "Cache-Control: private, max-age=31536000, immutable\r\nETag: \"aa-256\"\r\n"
                              "Content-Length: " +
                              QByteArray::number(body.size()) + "\r\n\r\n" + body);
            });
        });
    }

    QUrl url(const QString& path) const
    {
        return QUrl(QStringLiteral("http://127.0.0.1:%1%2").arg(serverPort()).arg(path));
    }

    QList<QByteArray> requests;

private:
    QByteArray m_partialRequest;
};

QByteArray lowerCase(const QByteArray& request)
{
    return request.toLower();
}

}  // namespace

class TestNetworkAccess : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();

    void givesTheTokenOnlyForTheServersAddresses_data();
    void givesTheTokenOnlyForTheServersAddresses();
    void sendsTheTokenWithTheRequest();
    void sendsNoTokenAfterLogout();
    void loadsAThumbnailFromDiskTheSecondTime();
    void keepsFullImagesOffTheDisk();

private:
    QNetworkReply* get(QNetworkAccessManager& networkAccess, const QUrl& url);

    std::unique_ptr<QTemporaryDir> m_cacheFolder;
    std::shared_ptr<ServerCredentials> m_credentials;
    std::unique_ptr<ThumbnailServer> m_server;
};

void TestNetworkAccess::init()
{
    m_cacheFolder = std::make_unique<QTemporaryDir>();
    m_credentials = std::make_shared<ServerCredentials>();
    m_server = std::make_unique<ThumbnailServer>();
    QVERIFY(m_server->listen(QHostAddress::LocalHost));
}

void TestNetworkAccess::givesTheTokenOnlyForTheServersAddresses_data()
{
    QTest::addColumn<QString>("serverUrl");
    QTest::addColumn<QString>("url");
    QTest::addColumn<bool>("getsToken");

    const QString server = QStringLiteral("https://photos.example.org");
    const QString thumbnail = QStringLiteral("/api/v1/images/5/thumbnail?size=256");
    QTest::newRow("the server") << server << server + thumbnail << true;
    QTest::newRow("host in capitals") << server << QStringLiteral("https://Photos.Example.org") + thumbnail << true;
    QTest::newRow("default port written out")
        << server << QStringLiteral("https://photos.example.org:443") + thumbnail << true;
    QTest::newRow("plain http") << server << QStringLiteral("http://photos.example.org") + thumbnail << false;
    QTest::newRow("another host") << server << QStringLiteral("https://example.org") + thumbnail << false;
    QTest::newRow("another port") << server << QStringLiteral("https://photos.example.org:8443") + thumbnail << false;

    const QString proxied = QStringLiteral("https://example.org/pictorio/");
    QTest::newRow("below the server's path")
        << proxied << QStringLiteral("https://example.org/pictorio") + thumbnail << true;
    QTest::newRow("beside the server's path")
        << proxied << QStringLiteral("https://example.org/pictorio-old") + thumbnail << false;
    QTest::newRow("outside the server's path") << proxied << QStringLiteral("https://example.org/other") << false;
}

void TestNetworkAccess::givesTheTokenOnlyForTheServersAddresses()
{
    QFETCH(QString, serverUrl);
    QFETCH(QString, url);
    QFETCH(bool, getsToken);
    m_credentials->setLogin(QUrl(serverUrl), QStringLiteral("pk_secret"));

    QCOMPARE(m_credentials->tokenFor(QUrl(url)), getsToken ? QStringLiteral("pk_secret") : QString());
}

void TestNetworkAccess::sendsTheTokenWithTheRequest()
{
    m_credentials->setLogin(m_server->url({}), QStringLiteral("pk_secret"));
    QNetworkAccessManager* networkAccess = createNetworkAccessManager(m_credentials, m_cacheFolder->path(), this);

    QNetworkReply* reply = get(*networkAccess, m_server->url(QStringLiteral("/api/v1/images/5/thumbnail")));

    QCOMPARE(reply->readAll(), QByteArray("webp bytes"));
    QCOMPARE(m_server->requests.size(), 1);
    QVERIFY(lowerCase(m_server->requests.first()).contains("authorization: bearer pk_secret"));
}

void TestNetworkAccess::sendsNoTokenAfterLogout()
{
    m_credentials->setLogin(m_server->url({}), QStringLiteral("pk_secret"));
    m_credentials->clear();
    QNetworkAccessManager* networkAccess = createNetworkAccessManager(m_credentials, m_cacheFolder->path(), this);

    get(*networkAccess, m_server->url(QStringLiteral("/api/v1/images/5/thumbnail")));

    QCOMPARE(m_server->requests.size(), 1);
    QVERIFY(!lowerCase(m_server->requests.first()).contains("authorization"));
}

// The server marks thumbnails as never changing: after the first time, they come from the disk cache,
// also after a restart (a new network access over the same cache folder).
void TestNetworkAccess::loadsAThumbnailFromDiskTheSecondTime()
{
    m_credentials->setLogin(m_server->url({}), QStringLiteral("pk_secret"));
    const QUrl thumbnail = m_server->url(QStringLiteral("/api/v1/images/5/thumbnail"));
    {
        QNetworkAccessManager* firstRun = createNetworkAccessManager(m_credentials, m_cacheFolder->path(), this);
        QNetworkReply* reply = get(*firstRun, thumbnail);
        QVERIFY(!reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool());
        delete firstRun;
    }

    QNetworkAccessManager* secondRun = createNetworkAccessManager(m_credentials, m_cacheFolder->path(), this);
    QNetworkReply* reply = get(*secondRun, thumbnail);

    QCOMPARE(reply->readAll(), QByteArray("webp bytes"));
    QVERIFY(reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool());
    QCOMPARE(m_server->requests.size(), 1);  // the server was asked only once
}

// Even when the server marks it as never changing, a full image is loaded again each time it is opened.
void TestNetworkAccess::keepsFullImagesOffTheDisk()
{
    m_credentials->setLogin(m_server->url({}), QStringLiteral("pk_secret"));
    QNetworkAccessManager* networkAccess = createNetworkAccessManager(m_credentials, m_cacheFolder->path(), this);
    const QUrl file = m_server->url(QStringLiteral("/api/v1/images/5/file"));

    get(*networkAccess, file);
    QNetworkReply* reply = get(*networkAccess, file);

    QCOMPARE(reply->readAll(), QByteArray("webp bytes"));
    QVERIFY(!reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool());
    QCOMPARE(m_server->requests.size(), 2);
}

QNetworkReply* TestNetworkAccess::get(QNetworkAccessManager& networkAccess, const QUrl& url)
{
    QNetworkReply* reply = networkAccess.get(QNetworkRequest(url));
    reply->setParent(this);  // kept until the test ends, so it can be read after its manager is gone
    if (!reply->isFinished())
    {
        QSignalSpy finished(reply, &QNetworkReply::finished);
        finished.wait(5000);
    }

    return reply;
}

}  // namespace pictorio::network_access

QTEST_GUILESS_MAIN(pictorio::network_access::TestNetworkAccess)
#include "TestNetworkAccess.moc"
