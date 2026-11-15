/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <memory>

#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>

#include "HttpTransport.hpp"
#include "ServerClient.hpp"
#include "ServerClientFactory.hpp"

namespace pictorio::server_client
{

namespace
{

// Records the requests and answers each with the response set before; no network involved.
class FakeTransport : public HttpTransport
{
public:
    QFuture<HttpResponse> send(const HttpRequest& request) override
    {
        requests.append(request);
        return QtFuture::makeReadyValueFuture(nextResponse);
    }

    HttpRequest lastRequest() const
    {
        return requests.last();
    }

    HttpResponse nextResponse;
    QList<HttpRequest> requests;
};

HttpResponse answer(int status, const QByteArray& body = {})
{
    HttpResponse response;
    response.status = status;
    response.body = body;
    return response;
}

QByteArray headerOf(const HttpRequest& request, const QByteArray& name)
{
    for (const auto& [headerName, value] : request.headers)
    {
        if (headerName == name)
        {
            return value;
        }
    }

    return {};
}

QJsonObject jsonBodyOf(const HttpRequest& request)
{
    return QJsonDocument::fromJson(request.body).object();
}

// The future's value; the fake transport answers at once, so it is ready.
template <typename T> T valueOf(const QFuture<T>& future)
{
    Q_ASSERT(future.isFinished());
    return future.result();
}

}  // namespace

class TestServerClient : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();

    void buildsUrlsUnderTheServerUrl_data();
    void buildsUrlsUnderTheServerUrl();
    void sendsTheTokenOnceItIsSet();

    void readsTheServerInfo();
    void logsInAndReadsTheToken();
    void readsTheCurrentUser();
    void changesTheOwnPassword();
    void readsTheDevices();
    void revokesADevice();

    void turnsProblemDetailsIntoErrors();
    void keepsFieldErrorsAndExtraFields();
    void reportsNetworkErrors();
    void reportsAnswersWithoutProblemDetails();
    void reportsAnswersThatAreNotJson();
    void namesTheFieldThatIsMissing();
    void refusesAnUnknownRole();

    void realTransportReadsAnAnswer();
    void realTransportReportsARefusedConnection();

private:
    std::shared_ptr<FakeTransport> m_transport;
    std::unique_ptr<ServerClient> m_client;
};

void TestServerClient::init()
{
    m_transport = std::make_shared<FakeTransport>();
    m_client = createServerClient(QUrl(QStringLiteral("http://raspberrypi.local:8737")), m_transport);
}

void TestServerClient::buildsUrlsUnderTheServerUrl_data()
{
    QTest::addColumn<QString>("serverUrl");
    QTest::addColumn<QString>("expectedUrl");

    QTest::newRow("host and port") << "http://raspberrypi.local:8737"
                                   << "http://raspberrypi.local:8737/api/v1/server/info";
    QTest::newRow("trailing slash") << "https://photos.example.org/"
                                    << "https://photos.example.org/api/v1/server/info";
    QTest::newRow("behind a proxy path") << "https://example.org/pictorio/"
                                         << "https://example.org/pictorio/api/v1/server/info";
}

void TestServerClient::buildsUrlsUnderTheServerUrl()
{
    QFETCH(QString, serverUrl);
    QFETCH(QString, expectedUrl);
    const auto client = createServerClient(QUrl(serverUrl), m_transport);
    m_transport->nextResponse = answer(200, R"({"name":"pictorio-db","version":"0.1.0","api_version":1,
                                                "setup_required":false})");

    QVERIFY(valueOf(client->serverInfo()).has_value());

    QCOMPARE(m_transport->lastRequest().url.toString(), expectedUrl);
}

void TestServerClient::sendsTheTokenOnceItIsSet()
{
    m_transport->nextResponse = answer(204);

    QVERIFY(valueOf(m_client->revokeDevice(1)).has_value());
    m_client->setToken(QStringLiteral("pk_secret"));
    QVERIFY(valueOf(m_client->revokeDevice(1)).has_value());

    QCOMPARE(headerOf(m_transport->requests.at(0), "Authorization"), QByteArray());
    QCOMPARE(headerOf(m_transport->requests.at(1), "Authorization"), QByteArray("Bearer pk_secret"));
}

void TestServerClient::readsTheServerInfo()
{
    m_transport->nextResponse = answer(200, R"({"name":"pictorio-db","version":"0.1.0","api_version":1,
                                                "setup_required":true})");

    const Result<ServerInfo> info = valueOf(m_client->serverInfo());

    QVERIFY(info.has_value());
    QCOMPARE(m_transport->lastRequest().method, QByteArray("GET"));
    QCOMPARE(info->name, QStringLiteral("pictorio-db"));
    QCOMPARE(info->version, QStringLiteral("0.1.0"));
    QCOMPARE(info->apiVersion, 1);
    QVERIFY(info->setupRequired);
}

void TestServerClient::logsInAndReadsTheToken()
{
    m_transport->nextResponse = answer(201, R"({"token_id":3,"token":"pk_new",
                                                "user":{"id":2,"username":"kid","role":"user"}})");

    const Result<Login> login =
        valueOf(m_client->logIn(QStringLiteral("kid"), QStringLiteral("correct horse"), QStringLiteral("laptop")));

    const HttpRequest request = m_transport->lastRequest();
    QCOMPARE(request.method, QByteArray("POST"));
    QCOMPARE(request.url.path(), QStringLiteral("/api/v1/auth/tokens"));
    QCOMPARE(headerOf(request, "Content-Type"), QByteArray("application/json"));
    QCOMPARE(jsonBodyOf(request),
             QJsonObject({{QStringLiteral("username"), QStringLiteral("kid")},
                          {QStringLiteral("password"), QStringLiteral("correct horse")},
                          {QStringLiteral("device_name"), QStringLiteral("laptop")}}));
    QVERIFY(login.has_value());
    QCOMPARE(login->tokenId, 3);
    QCOMPARE(login->token, QStringLiteral("pk_new"));
    QCOMPARE(login->user.id, 2);
    QCOMPARE(login->user.username, QStringLiteral("kid"));
    QVERIFY(login->user.role == Role::User);
}

void TestServerClient::readsTheCurrentUser()
{
    m_transport->nextResponse = answer(200, R"({"id":1,"username":"apo","role":"admin",
                                                "restricted_tags":["rating:nsfw","character:*"]})");

    const Result<CurrentUser> currentUser = valueOf(m_client->currentUser());

    QCOMPARE(m_transport->lastRequest().url.path(), QStringLiteral("/api/v1/auth/me"));
    QVERIFY(currentUser.has_value());
    QCOMPARE(currentUser->user.username, QStringLiteral("apo"));
    QVERIFY(currentUser->user.role == Role::Admin);
    QCOMPARE(currentUser->restrictedTags, QStringList({QStringLiteral("rating:nsfw"), QStringLiteral("character:*")}));
}

void TestServerClient::changesTheOwnPassword()
{
    m_transport->nextResponse = answer(200, R"({"id":2,"username":"kid","role":"user","restricted_tags":[]})");

    const Result<CurrentUser> currentUser =
        valueOf(m_client->changeOwnPassword(QStringLiteral("old password"), QStringLiteral("new password")));

    const HttpRequest request = m_transport->lastRequest();
    QCOMPARE(request.method, QByteArray("PATCH"));
    QCOMPARE(request.url.path(), QStringLiteral("/api/v1/auth/me"));
    QCOMPARE(jsonBodyOf(request),
             QJsonObject({{QStringLiteral("current_password"), QStringLiteral("old password")},
                          {QStringLiteral("new_password"), QStringLiteral("new password")}}));
    QVERIFY(currentUser.has_value());
}

void TestServerClient::readsTheDevices()
{
    m_transport->nextResponse = answer(200, R"([
        {"token_id":1,"device_name":"laptop","created_at":"2026-11-15T19:22:03.724229Z",
         "last_used_at":"2026-11-15T19:22:04Z","expires_at":null,"is_current":true},
        {"token_id":2,"device_name":"Web admin","created_at":"2026-11-15T08:00:00Z",
         "last_used_at":"2026-11-15T09:00:00Z","expires_at":"2026-11-15T20:00:00Z","is_current":false}])");

    const Result<QList<DeviceToken>> devices = valueOf(m_client->devices());

    QCOMPARE(m_transport->lastRequest().url.path(), QStringLiteral("/api/v1/auth/tokens"));
    QVERIFY(devices.has_value());
    QCOMPARE(devices->size(), 2);
    const DeviceToken laptop = devices->at(0);
    QCOMPARE(laptop.deviceName, QStringLiteral("laptop"));
    QCOMPARE(laptop.createdAt, QDateTime(QDate(2026, 11, 15), QTime(19, 22, 3, 724), QTimeZone::UTC));
    QVERIFY(!laptop.expiresAt.has_value());
    QVERIFY(laptop.isCurrent);
    QCOMPARE(devices->at(1).expiresAt.value(), QDateTime(QDate(2026, 11, 15), QTime(20, 0), QTimeZone::UTC));
}

void TestServerClient::revokesADevice()
{
    m_transport->nextResponse = answer(204);

    const Result<void> revoked = valueOf(m_client->revokeDevice(7));

    QCOMPARE(m_transport->lastRequest().method, QByteArray("DELETE"));
    QCOMPARE(m_transport->lastRequest().url.path(), QStringLiteral("/api/v1/auth/tokens/7"));
    QVERIFY(revoked.has_value());
}

void TestServerClient::turnsProblemDetailsIntoErrors()
{
    m_transport->nextResponse = answer(403, R"({"type":"https://pictorio.invalid/problems/wrong-password",
        "title":"Wrong password","status":403,"code":"wrong_password","detail":"The current password is not right."})");

    const Result<CurrentUser> result =
        valueOf(m_client->changeOwnPassword(QStringLiteral("typo"), QStringLiteral("new password")));

    QVERIFY(!result.has_value());
    QCOMPARE(result.error().httpStatus, 403);
    QCOMPARE(result.error().code, QStringLiteral("wrong_password"));
    QCOMPARE(result.error().title, QStringLiteral("Wrong password"));
    QCOMPARE(result.error().detail, QStringLiteral("The current password is not right."));
}

void TestServerClient::keepsFieldErrorsAndExtraFields()
{
    m_transport->nextResponse = answer(422, R"({"type":"t","title":"Invalid request","status":422,
        "code":"validation_error","errors":[{"field":"password","message":"8 to 1024 characters"}],"tag":"a|b"})");

    const Result<CurrentUser> result =
        valueOf(m_client->changeOwnPassword(QStringLiteral("old password"), QStringLiteral("short")));

    QVERIFY(!result.has_value());
    QCOMPARE(result.error().fieldErrors.size(), 1);
    QCOMPARE(result.error().fieldErrors.at(0).field, QStringLiteral("password"));
    QCOMPARE(result.error().fieldErrors.at(0).message, QStringLiteral("8 to 1024 characters"));
    QCOMPARE(result.error().problem.value(QStringLiteral("tag")).toString(), QStringLiteral("a|b"));
}

void TestServerClient::reportsNetworkErrors()
{
    m_transport->nextResponse.networkError = QStringLiteral("Connection refused");

    const Result<ServerInfo> result = valueOf(m_client->serverInfo());

    QVERIFY(!result.has_value());
    QCOMPARE(result.error().code, networkErrorCode);
    QCOMPARE(result.error().httpStatus, 0);
    QCOMPARE(result.error().detail, QStringLiteral("Connection refused"));
}

void TestServerClient::reportsAnswersWithoutProblemDetails()
{
    m_transport->nextResponse = answer(502, "<html>Bad Gateway</html>");  // e.g. from a reverse proxy

    const Result<ServerInfo> result = valueOf(m_client->serverInfo());

    QVERIFY(!result.has_value());
    QCOMPARE(result.error().code, QStringLiteral("http_502"));
    QCOMPARE(result.error().httpStatus, 502);
}

void TestServerClient::reportsAnswersThatAreNotJson()
{
    m_transport->nextResponse = answer(200, "<html>a captive portal</html>");

    const Result<ServerInfo> result = valueOf(m_client->serverInfo());

    QVERIFY(!result.has_value());
    QCOMPARE(result.error().code, invalidResponseCode);
}

void TestServerClient::namesTheFieldThatIsMissing()
{
    m_transport->nextResponse = answer(201, R"({"token_id":3,"token":"pk_new","user":{"id":2,"username":"kid"}})");

    const Result<Login> result =
        valueOf(m_client->logIn(QStringLiteral("kid"), QStringLiteral("correct horse"), QStringLiteral("laptop")));

    QVERIFY(!result.has_value());
    QCOMPARE(result.error().code, invalidResponseCode);
    QCOMPARE(result.error().detail, QStringLiteral("user.role: missing"));
}

void TestServerClient::refusesAnUnknownRole()
{
    m_transport->nextResponse = answer(200, R"({"id":1,"username":"apo","role":"owner","restricted_tags":[]})");

    const Result<CurrentUser> result = valueOf(m_client->currentUser());

    QVERIFY(!result.has_value());
    QCOMPARE(result.error().detail, QStringLiteral("role: unknown role owner"));
}

// The real network transport, against a one-answer HTTP server on this computer.
void TestServerClient::realTransportReadsAnAnswer()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    connect(&server, &QTcpServer::newConnection, &server, [&server]() {
        QTcpSocket* socket = server.nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, socket, [socket]() {
            socket->readAll();
            const QByteArray body =
                R"({"name":"pictorio-db","version":"0.1.0","api_version":1,"setup_required":false})";
            socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: " +
                          QByteArray::number(body.size()) + "\r\n\r\n" + body);
            socket->disconnectFromHost();
        });
    });
    const auto client = createServerClient(QUrl(QStringLiteral("http://127.0.0.1:%1").arg(server.serverPort())));

    const QFuture<Result<ServerInfo>> future = client->serverInfo();

    QTRY_VERIFY(future.isFinished());
    QVERIFY(future.result().has_value());
    QCOMPARE(future.result()->name, QStringLiteral("pictorio-db"));
}

void TestServerClient::realTransportReportsARefusedConnection()
{
    QTcpServer server;  // takes a free port and gives it back, so nothing listens there
    QVERIFY(server.listen(QHostAddress::LocalHost));
    const quint16 closedPort = server.serverPort();
    server.close();
    const auto client = createServerClient(QUrl(QStringLiteral("http://127.0.0.1:%1").arg(closedPort)));

    const QFuture<Result<ServerInfo>> future = client->serverInfo();

    QTRY_VERIFY(future.isFinished());
    QVERIFY(!future.result().has_value());
    QCOMPARE(future.result().error().code, networkErrorCode);
}

}  // namespace pictorio::server_client

QTEST_GUILESS_MAIN(pictorio::server_client::TestServerClient)
#include "TestServerClient.moc"
