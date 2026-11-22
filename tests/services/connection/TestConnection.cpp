/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <memory>

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrlQuery>

#include "Connection.hpp"
#include "ServerClientFactory.hpp"
#include "SettingsFactory.hpp"
#include "fakes/FakeServer.hpp"
#include "fakes/FakeTokenStore.hpp"

namespace photobook::connection
{

namespace
{

using fakes::FakeServer;
using fakes::FakeTokenStore;
using fakes::Keyring;
using fakes::problem;
using fakes::response;
using server_client::HttpRequest;

const QString serverUrl = QStringLiteral("https://photos.example.org");
const QString localUrl = serverUrl + QStringLiteral("/api/v1/server/local");

const QByteArray serverInfoJson = R"({"name":"photobook-db","version":"0.1.0","api_version":1,"setup_required":false})";
const QByteArray loginJson = R"({"token_id":3,"token":"pk_new","user":{"id":2,"username":"restrictedUser","role":"user"}})";
const QByteArray currentUserJson = R"({"id":2,"username":"restrictedUser","role":"user","restricted_tags":["rating:private"]})";
const QByteArray devicesJson = R"([{"token_id":3,"device_name":"laptop","created_at":"2026-11-15T19:22:03Z",
    "last_used_at":"2026-11-15T19:22:04Z","expires_at":null,"is_current":true}])";

}  // namespace

class TestConnection : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();

    void findsTheServer();
    void triesPlainHttpWhenHttpsDoesNotAnswer();
    void asksForAnAddress();
    void refusesWhatIsNotAPhotobookServer();
    void pointsToTheSetupPageOfANewServer();
    void linksToTheWebAdmin();
    void linksToTheWebAdminUnderTheServersPath();

    void logsInAndKeepsTheLogin();
    void staysOnTheLoginAfterAWrongPassword();
    void logsInWithoutAKeyring();

    void startsWithoutServerIfNoneIsRemembered();
    void restoresTheLoginAtStart();
    void asksToLogInAgainWhenTheLoginHasEnded();
    void staysOfflineWhenTheServerDoesNotAnswer();

    void logsOutAndEndsThisDevicesLogin();

    void learnsWhereTheLibraryIsWhenTheServerRunsHere();
    void learnsItWhenTheLoginIsRestored();
    void knowsNoFolderOfAServerElsewhere();
    void doesNotAskWithoutAMachineId();
    void forgetsTheFolderOnLogout();

private:
    // machineId: this computer's app-specific ID; empty: none known
    std::unique_ptr<Connection> createConnection(const QString& machineId = {});
    void logInAsrestrictedUser(Connection& connection);

    std::unique_ptr<QTemporaryDir> m_folder;
    std::unique_ptr<settings::Settings> m_settings;
    std::shared_ptr<FakeServer> m_server;
    std::shared_ptr<Keyring> m_keyring;
};

void TestConnection::init()
{
    m_folder = std::make_unique<QTemporaryDir>();
    m_settings = settings::createSettings(m_folder->filePath(QStringLiteral("photobookrc")));
    m_server = std::make_shared<FakeServer>();
    m_server->answer("GET", serverUrl + QStringLiteral("/api/v1/server/info"), response(200, serverInfoJson));
    m_server->answer("POST", serverUrl + QStringLiteral("/api/v1/auth/tokens"), response(201, loginJson));
    m_server->answer("GET", serverUrl + QStringLiteral("/api/v1/auth/me"), response(200, currentUserJson));
    m_server->answer("GET", serverUrl + QStringLiteral("/api/v1/auth/tokens"), response(200, devicesJson));
    m_server->answer("DELETE", serverUrl + QStringLiteral("/api/v1/auth/tokens/3"), response(204, {}));
    m_keyring = std::make_shared<Keyring>();
}

std::unique_ptr<Connection> TestConnection::createConnection(const QString& machineId)
{
    auto serverClientFactory = [server = m_server](const QUrl& url) {
        return server_client::createServerClient(url, server);
    };
    return std::make_unique<Connection>(
        serverClientFactory, std::make_unique<FakeTokenStore>(m_keyring), m_settings.get(), machineId);
}

void TestConnection::logInAsrestrictedUser(Connection& connection)
{
    connection.checkServer(serverUrl);
    QTRY_COMPARE(connection.state(), Connection::State::ServerFound);
    connection.logIn(QStringLiteral("restrictedUser"), QStringLiteral("correct horse"));
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
    QCOMPARE(connection->serverDescription(), QStringLiteral("photobook-db 0.1.0"));
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

void TestConnection::refusesWhatIsNotAPhotobookServer()
{
    m_server->answer(
        "GET", QStringLiteral("https://example.org/api/v1/server/info"), response(404, "<html>Not Found</html>"));
    const auto connection = createConnection();

    connection->checkServer(QStringLiteral("https://example.org"));

    QTRY_COMPARE(connection->state(), Connection::State::NoServer);
    QVERIFY(connection->errorMessage().contains(QStringLiteral("is not a photobook-db server")));
}

void TestConnection::pointsToTheSetupPageOfANewServer()
{
    m_server->answer(
        "GET",
        serverUrl + QStringLiteral("/api/v1/server/info"),
        response(200, R"({"name":"photobook-db","version":"0.1.0","api_version":1,"setup_required":true})"));
    const auto connection = createConnection();

    connection->checkServer(serverUrl);

    QTRY_COMPARE(connection->state(), Connection::State::NoServer);
    QVERIFY(connection->errorMessage().contains(QStringLiteral("https://photos.example.org/admin/setup")));
}

void TestConnection::linksToTheWebAdmin()
{
    const auto connection = createConnection();
    QVERIFY(connection->webAdminUrl().isEmpty());  // no server yet

    connection->checkServer(serverUrl);

    QTRY_COMPARE(connection->state(), Connection::State::ServerFound);
    QCOMPARE(connection->webAdminUrl(), QUrl(QStringLiteral("https://photos.example.org/admin/login")));
}

// Behind a reverse proxy the server may live under a path of its own.
void TestConnection::linksToTheWebAdminUnderTheServersPath()
{
    const QString proxiedUrl = QStringLiteral("https://example.org/photobook");
    m_server->answer("GET", proxiedUrl + QStringLiteral("/api/v1/server/info"), response(200, serverInfoJson));
    const auto connection = createConnection();

    connection->checkServer(proxiedUrl + QLatin1Char('/'));

    QTRY_COMPARE(connection->state(), Connection::State::ServerFound);
    QCOMPARE(connection->webAdminUrl(), QUrl(QStringLiteral("https://example.org/photobook/admin/login")));
}

void TestConnection::logsInAndKeepsTheLogin()
{
    const auto connection = createConnection();

    logInAsrestrictedUser(*connection);

    QCOMPARE(connection->username(), QStringLiteral("restrictedUser"));
    QCOMPARE(m_keyring->tokens.value(serverUrl), QStringLiteral("pk_new"));
    QCOMPARE(connection->token(), QStringLiteral("pk_new"));
    QCOMPARE(m_settings->serverUrl(), QUrl(serverUrl));
    QCOMPARE(m_settings->serverUsername(), QStringLiteral("restrictedUser"));
    QTRY_COMPARE(connection->restrictedTags(), QStringList({QStringLiteral("rating:private")}));
    const QList<HttpRequest> meRequests = m_server->requestsTo("GET", serverUrl + QStringLiteral("/api/v1/auth/me"));
    QCOMPARE(fakes::authorizationOf(meRequests.last()), QByteArray("Bearer pk_new"));
    QVERIFY(connection->keyringWarning().isEmpty());
}

void TestConnection::staysOnTheLoginAfterAWrongPassword()
{
    m_server->answer("POST", serverUrl + QStringLiteral("/api/v1/auth/tokens"), problem(401, "not_authenticated"));
    const auto connection = createConnection();
    connection->checkServer(serverUrl);
    QTRY_COMPARE(connection->state(), Connection::State::ServerFound);

    connection->logIn(QStringLiteral("restrictedUser"), QStringLiteral("wrong"));

    QTRY_COMPARE(connection->errorMessage(), QStringLiteral("Wrong username or password."));
    QCOMPARE(connection->state(), Connection::State::ServerFound);
    QVERIFY(m_keyring->tokens.isEmpty());
}

void TestConnection::logsInWithoutAKeyring()
{
    m_keyring->available = false;
    const auto connection = createConnection();

    logInAsrestrictedUser(*connection);

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
    m_settings->setServerUsername(QStringLiteral("restrictedUser"));
    m_keyring->tokens.insert(serverUrl, QStringLiteral("pk_kept"));
    const auto connection = createConnection();

    connection->restoreLogin();

    QTRY_COMPARE(connection->state(), Connection::State::LoggedIn);
    QCOMPARE(connection->restrictedTags(), QStringList({QStringLiteral("rating:private")}));
    QVERIFY(m_server->requestsTo("POST", serverUrl + QStringLiteral("/api/v1/auth/tokens")).isEmpty());  // no password
    const QList<HttpRequest> meRequests = m_server->requestsTo("GET", serverUrl + QStringLiteral("/api/v1/auth/me"));
    QCOMPARE(fakes::authorizationOf(meRequests.last()), QByteArray("Bearer pk_kept"));
}

void TestConnection::asksToLogInAgainWhenTheLoginHasEnded()
{
    m_settings->setServerUrl(QUrl(serverUrl));
    m_settings->setServerUsername(QStringLiteral("restrictedUser"));
    m_keyring->tokens.insert(serverUrl, QStringLiteral("pk_revoked"));
    m_server->answer("GET", serverUrl + QStringLiteral("/api/v1/auth/me"), problem(401, "not_authenticated"));
    const auto connection = createConnection();

    connection->restoreLogin();

    QTRY_COMPARE(connection->state(), Connection::State::ServerFound);
    QCOMPARE(connection->errorMessage(), QStringLiteral("Your login has ended. Log in again."));
    QCOMPARE(connection->username(), QStringLiteral("restrictedUser"));  // filled in for the next login
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
    logInAsrestrictedUser(*connection);

    connection->logOut();

    QCOMPARE(connection->state(), Connection::State::ServerFound);
    QVERIFY(m_keyring->tokens.isEmpty());
    QVERIFY(connection->restrictedTags().isEmpty());
    QVERIFY(connection->token().isEmpty());
    const QString revokeUrl = serverUrl + QStringLiteral("/api/v1/auth/tokens/3");
    QTRY_COMPARE(m_server->requestsTo("DELETE", revokeUrl).size(), 1);
    QCOMPARE(fakes::authorizationOf(m_server->requestsTo("DELETE", revokeUrl).first()), QByteArray("Bearer pk_new"));
}

void TestConnection::learnsWhereTheLibraryIsWhenTheServerRunsHere()
{
    m_server->answer("GET", localUrl, response(200, R"({"library_dir":"/home/restrictedUser/Pictures/photobook-library"})"));
    const auto connection = createConnection(QStringLiteral("9d623a36"));
    QSignalSpy changed(connection.get(), &Connection::localLibraryDirChanged);

    logInAsrestrictedUser(*connection);

    QTRY_COMPARE(connection->localLibraryDir(), QStringLiteral("/home/restrictedUser/Pictures/photobook-library"));
    QCOMPARE(changed.size(), 1);
    const QUrl asked = m_server->requestsTo("GET", localUrl).first().url;
    QCOMPARE(QUrlQuery(asked).queryItemValue(QStringLiteral("machine")), QStringLiteral("9d623a36"));
}

void TestConnection::learnsItWhenTheLoginIsRestored()
{
    m_server->answer("GET", localUrl, response(200, R"({"library_dir":"/home/restrictedUser/Pictures/photobook-library"})"));
    m_settings->setServerUrl(QUrl(serverUrl));
    m_keyring->tokens.insert(serverUrl, QStringLiteral("pk_kept"));
    const auto connection = createConnection(QStringLiteral("9d623a36"));

    connection->restoreLogin();

    QTRY_COMPARE(connection->localLibraryDir(), QStringLiteral("/home/restrictedUser/Pictures/photobook-library"));
}

void TestConnection::knowsNoFolderOfAServerElsewhere()
{
    m_server->answer("GET", localUrl, problem(404, "not_same_machine"));
    const auto connection = createConnection(QStringLiteral("9d623a36"));

    logInAsrestrictedUser(*connection);

    QTRY_COMPARE(m_server->requestsTo("GET", localUrl).size(), 1);
    QTest::qWait(50);  // the answer arrives after the request
    QVERIFY(connection->localLibraryDir().isEmpty());
    QCOMPARE(connection->state(), Connection::State::LoggedIn);  // not an error
}

void TestConnection::doesNotAskWithoutAMachineId()
{
    const auto connection = createConnection();

    logInAsrestrictedUser(*connection);

    QTest::qWait(50);
    QVERIFY(m_server->requestsTo("GET", localUrl).isEmpty());
    QVERIFY(connection->localLibraryDir().isEmpty());
}

void TestConnection::forgetsTheFolderOnLogout()
{
    m_server->answer("GET", localUrl, response(200, R"({"library_dir":"/home/restrictedUser/Pictures/photobook-library"})"));
    const auto connection = createConnection(QStringLiteral("9d623a36"));
    logInAsrestrictedUser(*connection);
    QTRY_VERIFY(!connection->localLibraryDir().isEmpty());

    connection->logOut();

    QVERIFY(connection->localLibraryDir().isEmpty());
}

}  // namespace photobook::connection

QTEST_GUILESS_MAIN(photobook::connection::TestConnection)
#include "TestConnection.moc"
