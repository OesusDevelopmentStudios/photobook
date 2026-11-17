/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <memory>
#include <optional>

#include <KLocalizedString>
#include <QFuture>
#include <QTemporaryDir>
#include <QTest>

#include "Connection.hpp"
#include "ServerClientFactory.hpp"
#include "e2e/EndToEndServer.hpp"

namespace pictorio::e2e
{

namespace
{

using State = connection::Connection::State;

// The HTTP status of GET /auth/me with the client's token, once the server has answered; 0 if it did not
int currentUserStatus(server_client::ServerClient& client)
{
    QFuture<server_client::Result<server_client::CurrentUser>> answer = client.currentUser();
    if (!QTest::qWaitFor([&answer]() { return answer.isFinished(); }, answerTimeout))
    {
        return 0;
    }

    return answer.result().has_value() ? 200 : answer.result().error().httpStatus;
}

}  // namespace

// Logging in to a real server, as the connect page does, and staying logged in after a restart. Runs only
// against the server that tests/e2e/run_end_to_end_tests.py starts.
class TestEndToEndLogin : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();

    void logsInWithTheRightPassword();
    void refusesAWrongPassword();
    void learnsWhatIsRestrictedForTheUser();
    void staysLoggedInAfterARestart();
    void logsOutOnTheServerToo();

private:
    QString settingsFile() const;

    std::optional<EndToEndServer> m_server;
    std::unique_ptr<QTemporaryDir> m_folder;
    std::shared_ptr<fakes::Keyring> m_keyring;
};

void TestEndToEndLogin::initTestCase()
{
    KLocalizedString::setApplicationDomain("pictorio");
    m_server = EndToEndServer::fromEnvironment();
    if (!m_server.has_value())
    {
        QSKIP("No server: run tests/e2e/run_end_to_end_tests.py");
    }
}

void TestEndToEndLogin::init()
{
    m_folder = std::make_unique<QTemporaryDir>();
    m_keyring = std::make_shared<fakes::Keyring>();
}

void TestEndToEndLogin::logsInWithTheRightPassword()
{
    const AppConnection connection(settingsFile(), m_keyring);

    QVERIFY2(connection.logIn(*m_server, userName, m_server->password), qPrintable(connection->errorMessage()));

    QCOMPARE(connection->username(), userName);
    QVERIFY(connection->isPlainHttp());  // the script's server has no TLS
    QVERIFY(connection->serverDescription().startsWith(QStringLiteral("pictorio-db")));
    QVERIFY(!m_keyring->tokens.isEmpty());  // kept for the next start
}

void TestEndToEndLogin::refusesAWrongPassword()
{
    const AppConnection connection(settingsFile(), m_keyring);

    QVERIFY(!connection.logIn(*m_server, userName, QStringLiteral("not the password")));

    QCOMPARE(connection->state(), State::ServerFound);  // to try again
    QVERIFY(!connection->errorMessage().isEmpty());
    QVERIFY(m_keyring->tokens.isEmpty());
}

void TestEndToEndLogin::learnsWhatIsRestrictedForTheUser()
{
    const AppConnection connection(settingsFile(), m_keyring);

    QVERIFY(connection.logIn(*m_server, restrictedUserName, m_server->password));

    // Asked for right after logging in
    QTRY_COMPARE_WITH_TIMEOUT(
        connection->restrictedTags(), QStringList({QStringLiteral("rating:nsfw")}), answerTimeout);
}

void TestEndToEndLogin::staysLoggedInAfterARestart()
{
    {
        const AppConnection firstRun(settingsFile(), m_keyring);
        QVERIFY(firstRun.logIn(*m_server, userName, m_server->password));
    }

    const AppConnection nextRun(settingsFile(), m_keyring);
    nextRun->restoreLogin();

    QTRY_COMPARE_WITH_TIMEOUT(nextRun->state(), State::LoggedIn, answerTimeout);
    QCOMPARE(nextRun->username(), userName);
}

void TestEndToEndLogin::logsOutOnTheServerToo()
{
    const AppConnection connection(settingsFile(), m_keyring);
    QVERIFY(connection.logIn(*m_server, userName, m_server->password));
    const QString token = connection->token();

    connection->logOut();

    QCOMPARE(connection->state(), State::ServerFound);
    QVERIFY(m_keyring->tokens.isEmpty());
    // The token, as if a copy of it had been kept: the server no longer takes it.
    const std::unique_ptr<server_client::ServerClient> client = server_client::createServerClient(m_server->url);
    client->setToken(token);
    // Logging out does not wait for the server, so it may not have heard of it yet.
    QTRY_COMPARE_WITH_TIMEOUT(currentUserStatus(*client), 401, answerTimeout);
}

QString TestEndToEndLogin::settingsFile() const
{
    return m_folder->filePath(QStringLiteral("pictoriorc"));
}

}  // namespace pictorio::e2e

QTEST_GUILESS_MAIN(pictorio::e2e::TestEndToEndLogin)

#include "TestEndToEndLogin.moc"
