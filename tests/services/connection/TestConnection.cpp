/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <memory>

#include <QHash>
#include <QSet>
#include <QTemporaryDir>
#include <QTest>

#include "Connection.hpp"
#include "HttpTransport.hpp"
#include "ServerClientFactory.hpp"
#include "SettingsFactory.hpp"
#include "TokenStore.hpp"

namespace pictorio::connection
{

namespace
{

using server_client::HttpRequest;
using server_client::HttpResponse;
using token_store::StoreResult;
using token_store::TokenStoreError;

const QString serverUrl = QStringLiteral("https://photos.example.org");

const QByteArray serverInfoJson = R"({"name":"pictorio-db","version":"0.1.0","api_version":1,"setup_required":false})";
const QByteArray loginJson = R"({"token_id":3,"token":"pk_new","user":{"id":2,"username":"kid","role":"user"}})";
const QByteArray currentUserJson = R"({"id":2,"username":"kid","role":"user","restricted_tags":["rating:nsfw"]})";
const QByteArray devicesJson = R"([{"token_id":3,"device_name":"laptop","created_at":"2026-11-15T19:22:03Z",
    "last_used_at":"2026-11-15T19:22:04Z","expires_at":null,"is_current":true}])";

HttpResponse response(int status, const QByteArray& body)
{
    HttpResponse answer;
    answer.status = status;
    answer.body = body;
    return answer;
}

HttpResponse problem(int status, const QByteArray& code)
{
    return response(status,
                    R"({"type":"t","title":"Problem","status":)" + QByteArray::number(status) + R"(,"code":)" +
                        ('"' + code + '"') + "}");
}

// A pictorio-db server that answers with canned responses, by method and URL. Addresses can be made
// unreachable, as if the server were off.
class FakeServer : public server_client::HttpTransport
{
public:
    QFuture<HttpResponse> send(const HttpRequest& request) override
    {
        requests.append(request);
        if (unreachableOrigins.contains(request.url.adjusted(QUrl::RemovePath | QUrl::RemoveQuery).toString()))
        {
            HttpResponse noAnswer;
            noAnswer.networkError = QStringLiteral("Connection refused");
            return QtFuture::makeReadyValueFuture(noAnswer);
        }

        const QString key = QString::fromLatin1(request.method) + QLatin1Char(' ') + request.url.toString();
        return QtFuture::makeReadyValueFuture(answers.value(key, problem(404, "not_found")));
    }

    void answer(const char* method, const QString& url, const HttpResponse& answer)
    {
        answers.insert(QString::fromLatin1(method) + QLatin1Char(' ') + url, answer);
    }

    QList<HttpRequest> requestsTo(const char* method, const QString& url) const
    {
        QList<HttpRequest> found;
        for (const HttpRequest& request : requests)
        {
            if (request.method == method && request.url.toString() == url)
            {
                found.append(request);
            }
        }

        return found;
    }

    QHash<QString, HttpResponse> answers;
    QSet<QString> unreachableOrigins;  // e.g. "https://raspberrypi.local:8737"
    QList<HttpRequest> requests;
};

// The keyring: tokens by key, or none at all on this system.
struct Keyring
{
    QHash<QString, QString> tokens;
    bool available = true;
};

class FakeTokenStore : public token_store::TokenStore
{
public:
    explicit FakeTokenStore(std::shared_ptr<Keyring> keyring)
        : m_keyring(std::move(keyring))
    {}

    QFuture<StoreResult<QString>> readToken(const QString& serverKey) override
    {
        if (!m_keyring->available)
        {
            return QtFuture::makeReadyValueFuture(StoreResult<QString>(std::unexpected(unavailable())));
        }

        return QtFuture::makeReadyValueFuture(StoreResult<QString>(m_keyring->tokens.value(serverKey)));
    }

    QFuture<StoreResult<void>> writeToken(const QString& serverKey, const QString& token) override
    {
        if (!m_keyring->available)
        {
            return QtFuture::makeReadyValueFuture(StoreResult<void>(std::unexpected(unavailable())));
        }

        m_keyring->tokens.insert(serverKey, token);
        return QtFuture::makeReadyValueFuture(StoreResult<void>());
    }

    QFuture<StoreResult<void>> deleteToken(const QString& serverKey) override
    {
        m_keyring->tokens.remove(serverKey);
        return QtFuture::makeReadyValueFuture(StoreResult<void>());
    }

private:
    static TokenStoreError unavailable()
    {
        return {TokenStoreError::Kind::Unavailable, QStringLiteral("No keyring")};
    }

    std::shared_ptr<Keyring> m_keyring;
};

QByteArray authorizationOf(const HttpRequest& request)
{
    for (const auto& [name, value] : request.headers)
    {
        if (name == "Authorization")
        {
            return value;
        }
    }

    return {};
}

}  // namespace

class TestConnection : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();

    void findsTheServer();
    void triesPlainHttpWhenHttpsDoesNotAnswer();
    void asksForAnAddress();
    void refusesWhatIsNotAPictorioServer();
    void pointsToTheSetupPageOfANewServer();

    void logsInAndKeepsTheLogin();
    void staysOnTheLoginAfterAWrongPassword();
    void logsInWithoutAKeyring();

    void startsWithoutServerIfNoneIsRemembered();
    void restoresTheLoginAtStart();
    void asksToLogInAgainWhenTheLoginHasEnded();
    void staysOfflineWhenTheServerDoesNotAnswer();

    void logsOutAndEndsThisDevicesLogin();

private:
    std::unique_ptr<Connection> createConnection();
    void logInAsKid(Connection& connection);

    std::unique_ptr<QTemporaryDir> m_folder;
    std::unique_ptr<settings::Settings> m_settings;
    std::shared_ptr<FakeServer> m_server;
    std::shared_ptr<Keyring> m_keyring;
};

void TestConnection::init()
{
    m_folder = std::make_unique<QTemporaryDir>();
    m_settings = settings::createSettings(m_folder->filePath(QStringLiteral("pictoriorc")));
    m_server = std::make_shared<FakeServer>();
    m_server->answer("GET", serverUrl + QStringLiteral("/api/v1/server/info"), response(200, serverInfoJson));
    m_server->answer("POST", serverUrl + QStringLiteral("/api/v1/auth/tokens"), response(201, loginJson));
    m_server->answer("GET", serverUrl + QStringLiteral("/api/v1/auth/me"), response(200, currentUserJson));
    m_server->answer("GET", serverUrl + QStringLiteral("/api/v1/auth/tokens"), response(200, devicesJson));
    m_server->answer("DELETE", serverUrl + QStringLiteral("/api/v1/auth/tokens/3"), response(204, {}));
    m_keyring = std::make_shared<Keyring>();
}

std::unique_ptr<Connection> TestConnection::createConnection()
{
    auto serverClientFactory = [server = m_server](const QUrl& url) {
        return server_client::createServerClient(url, server);
    };
    return std::make_unique<Connection>(
        serverClientFactory, std::make_unique<FakeTokenStore>(m_keyring), m_settings.get());
}

void TestConnection::logInAsKid(Connection& connection)
{
    connection.checkServer(serverUrl);
    QTRY_COMPARE(connection.state(), Connection::State::ServerFound);
    connection.logIn(QStringLiteral("kid"), QStringLiteral("correct horse"));
    QTRY_COMPARE(connection.state(), Connection::State::LoggedIn);
}

void TestConnection::findsTheServer()
{
    const auto connection = createConnection();
    QList<Connection::State> states;
    connect(connection.get(), &Connection::stateChanged, this, [&states, &connection]() {
        states.append(connection->state());
    });

    connection->checkServer(QStringLiteral(" https://photos.example.org "));

    QTRY_COMPARE(connection->state(), Connection::State::ServerFound);
    QCOMPARE(states, QList<Connection::State>({Connection::State::CheckingServer, Connection::State::ServerFound}));
    QCOMPARE(connection->serverAddress(), serverUrl);
    QCOMPARE(connection->serverDescription(), QStringLiteral("pictorio-db 0.1.0"));
    QVERIFY(!connection->isPlainHttp());
    QVERIFY(connection->errorMessage().isEmpty());
}

void TestConnection::triesPlainHttpWhenHttpsDoesNotAnswer()
{
    m_server->unreachableOrigins.insert(QStringLiteral("https://raspberrypi.local:8737"));
    m_server->answer(
        "GET", QStringLiteral("http://raspberrypi.local:8737/api/v1/server/info"), response(200, serverInfoJson));
    const auto connection = createConnection();

    connection->checkServer(QStringLiteral("raspberrypi.local:8737"));

    QTRY_COMPARE(connection->state(), Connection::State::ServerFound);
    QCOMPARE(connection->serverAddress(), QStringLiteral("http://raspberrypi.local:8737"));
    QVERIFY(connection->isPlainHttp());  // the page warns about it
}

void TestConnection::asksForAnAddress()
{
    const auto connection = createConnection();

    connection->checkServer(QStringLiteral("  "));

    QCOMPARE(connection->state(), Connection::State::NoServer);
    QVERIFY(connection->errorMessage().contains(QStringLiteral("Enter the server's address")));
    QVERIFY(m_server->requests.isEmpty());
}

void TestConnection::refusesWhatIsNotAPictorioServer()
{
    m_server->answer(
        "GET", QStringLiteral("https://example.org/api/v1/server/info"), response(404, "<html>Not Found</html>"));
    const auto connection = createConnection();

    connection->checkServer(QStringLiteral("https://example.org"));

    QTRY_COMPARE(connection->state(), Connection::State::NoServer);
    QVERIFY(connection->errorMessage().contains(QStringLiteral("is not a pictorio-db server")));
}

void TestConnection::pointsToTheSetupPageOfANewServer()
{
    m_server->answer(
        "GET",
        serverUrl + QStringLiteral("/api/v1/server/info"),
        response(200, R"({"name":"pictorio-db","version":"0.1.0","api_version":1,"setup_required":true})"));
    const auto connection = createConnection();

    connection->checkServer(serverUrl);

    QTRY_COMPARE(connection->state(), Connection::State::NoServer);
    QVERIFY(connection->errorMessage().contains(QStringLiteral("https://photos.example.org/admin/setup")));
}

void TestConnection::logsInAndKeepsTheLogin()
{
    const auto connection = createConnection();

    logInAsKid(*connection);

    QCOMPARE(connection->username(), QStringLiteral("kid"));
    QCOMPARE(m_keyring->tokens.value(serverUrl), QStringLiteral("pk_new"));
    QCOMPARE(m_settings->serverUrl(), QUrl(serverUrl));
    QCOMPARE(m_settings->serverUsername(), QStringLiteral("kid"));
    QTRY_COMPARE(connection->restrictedTags(), QStringList({QStringLiteral("rating:nsfw")}));
    const QList<HttpRequest> meRequests = m_server->requestsTo("GET", serverUrl + QStringLiteral("/api/v1/auth/me"));
    QCOMPARE(authorizationOf(meRequests.last()), QByteArray("Bearer pk_new"));
    QVERIFY(connection->keyringWarning().isEmpty());
}

void TestConnection::staysOnTheLoginAfterAWrongPassword()
{
    m_server->answer("POST", serverUrl + QStringLiteral("/api/v1/auth/tokens"), problem(401, "not_authenticated"));
    const auto connection = createConnection();
    connection->checkServer(serverUrl);
    QTRY_COMPARE(connection->state(), Connection::State::ServerFound);

    connection->logIn(QStringLiteral("kid"), QStringLiteral("wrong"));

    QTRY_COMPARE(connection->errorMessage(), QStringLiteral("Wrong username or password."));
    QCOMPARE(connection->state(), Connection::State::ServerFound);
    QVERIFY(m_keyring->tokens.isEmpty());
}

void TestConnection::logsInWithoutAKeyring()
{
    m_keyring->available = false;
    const auto connection = createConnection();

    logInAsKid(*connection);

    QTRY_VERIFY(connection->keyringWarning().contains(QStringLiteral("log in again")));
    QCOMPARE(connection->state(), Connection::State::LoggedIn);  // until the app ends
}

void TestConnection::startsWithoutServerIfNoneIsRemembered()
{
    const auto connection = createConnection();

    connection->restoreLogin();

    QCOMPARE(connection->state(), Connection::State::NoServer);
}

void TestConnection::restoresTheLoginAtStart()
{
    m_settings->setServerUrl(QUrl(serverUrl));
    m_settings->setServerUsername(QStringLiteral("kid"));
    m_keyring->tokens.insert(serverUrl, QStringLiteral("pk_kept"));
    const auto connection = createConnection();

    connection->restoreLogin();

    QTRY_COMPARE(connection->state(), Connection::State::LoggedIn);
    QCOMPARE(connection->restrictedTags(), QStringList({QStringLiteral("rating:nsfw")}));
    QVERIFY(m_server->requestsTo("POST", serverUrl + QStringLiteral("/api/v1/auth/tokens")).isEmpty());  // no password
    const QList<HttpRequest> meRequests = m_server->requestsTo("GET", serverUrl + QStringLiteral("/api/v1/auth/me"));
    QCOMPARE(authorizationOf(meRequests.last()), QByteArray("Bearer pk_kept"));
}

void TestConnection::asksToLogInAgainWhenTheLoginHasEnded()
{
    m_settings->setServerUrl(QUrl(serverUrl));
    m_settings->setServerUsername(QStringLiteral("kid"));
    m_keyring->tokens.insert(serverUrl, QStringLiteral("pk_revoked"));
    m_server->answer("GET", serverUrl + QStringLiteral("/api/v1/auth/me"), problem(401, "not_authenticated"));
    const auto connection = createConnection();

    connection->restoreLogin();

    QTRY_COMPARE(connection->state(), Connection::State::ServerFound);
    QCOMPARE(connection->errorMessage(), QStringLiteral("Your login has ended. Log in again."));
    QCOMPARE(connection->username(), QStringLiteral("kid"));  // filled in for the next login
    QVERIFY(m_keyring->tokens.isEmpty());
}

void TestConnection::staysOfflineWhenTheServerDoesNotAnswer()
{
    m_settings->setServerUrl(QUrl(serverUrl));
    m_keyring->tokens.insert(serverUrl, QStringLiteral("pk_kept"));
    m_server->unreachableOrigins.insert(serverUrl);
    const auto connection = createConnection();

    connection->restoreLogin();

    QTRY_COMPARE(connection->state(), Connection::State::Offline);
    QVERIFY(connection->errorMessage().contains(QStringLiteral("Connection refused")));
    QCOMPARE(m_keyring->tokens.value(serverUrl), QStringLiteral("pk_kept"));  // for when it answers again
}

void TestConnection::logsOutAndEndsThisDevicesLogin()
{
    const auto connection = createConnection();
    logInAsKid(*connection);

    connection->logOut();

    QCOMPARE(connection->state(), Connection::State::ServerFound);
    QVERIFY(m_keyring->tokens.isEmpty());
    QVERIFY(connection->restrictedTags().isEmpty());
    const QString revokeUrl = serverUrl + QStringLiteral("/api/v1/auth/tokens/3");
    QTRY_COMPARE(m_server->requestsTo("DELETE", revokeUrl).size(), 1);
    QCOMPARE(authorizationOf(m_server->requestsTo("DELETE", revokeUrl).first()), QByteArray("Bearer pk_new"));
}

}  // namespace pictorio::connection

QTEST_GUILESS_MAIN(pictorio::connection::TestConnection)
#include "TestConnection.moc"
