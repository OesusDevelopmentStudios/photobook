/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <chrono>
#include <memory>
#include <optional>

#include <QString>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

#include "Connection.hpp"
#include "ServerClientFactory.hpp"
#include "Settings.hpp"
#include "SettingsFactory.hpp"
#include "fakes/FakeTokenStore.hpp"

namespace pictorio::e2e
{

// The accounts tests/e2e/run_end_to_end_tests.py creates, all with the same password
inline const QString userName = QStringLiteral("sample_user");
inline const QString restrictedUserName = QStringLiteral("kid");  // restricted from rating:nsfw

// A real server answers over the network, more slowly than the fakes of the unit tests.
constexpr std::chrono::milliseconds answerTimeout{10'000};

// The pictorio-db server the script started, as it describes it in the environment
struct EndToEndServer
{
    QUrl url;               // e.g. http://127.0.0.1:41234
    QString password;       // of every account
    QString libraryDir;     // where the server keeps the image files
    QString machineIdFile;  // the server's computer's machine ID: the app reading it is on that computer

    // Nothing unless the script set the environment, e.g. in a plain ctest run
    static std::optional<EndToEndServer> fromEnvironment()
    {
        const QString url = qEnvironmentVariable("PICTORIO_E2E_SERVER_URL");
        if (url.isEmpty())
        {
            return std::nullopt;
        }

        return EndToEndServer{QUrl(url),
                              qEnvironmentVariable("PICTORIO_E2E_PASSWORD"),
                              qEnvironmentVariable("PICTORIO_E2E_LIBRARY_DIR"),
                              qEnvironmentVariable("PICTORIO_E2E_MACHINE_ID_FILE")};
    }
};

// A connection as the app makes one, over real HTTP, with its settings in a temporary folder and the login
// token in a fake keyring, never the user's own. Restarting the app is a second one with the same settings
// file and keyring. appSpecificMachineId: as utils/MachineId.hpp makes it; empty: never asks where the
// library folder is.
class AppConnection
{
public:
    AppConnection(const QString& settingsFile,
                  std::shared_ptr<fakes::Keyring> keyring,
                  const QString& appSpecificMachineId = {})
        : m_settings(settings::createSettings(settingsFile))
        , m_connection(std::make_unique<connection::Connection>(
              [](const QUrl& url) { return server_client::createServerClient(url); },
              std::make_unique<fakes::FakeTokenStore>(std::move(keyring)),
              m_settings.get(),
              appSpecificMachineId))
    {}

    connection::Connection& operator*() const
    {
        return *m_connection;
    }

    connection::Connection* operator->() const
    {
        return m_connection.get();
    }

    // As on the connect page: the server's address, then the account. True once logged in.
    bool logIn(const EndToEndServer& server, const QString& username, const QString& password) const
    {
        using State = connection::Connection::State;
        m_connection->checkServer(server.url.toString());
        if (!QTest::qWaitFor([this]() { return m_connection->state() == State::ServerFound; }, answerTimeout))
        {
            return false;
        }

        m_connection->logIn(username, password);
        return QTest::qWaitFor([this]() { return m_connection->state() != State::LoggingIn; }, answerTimeout) &&
               m_connection->state() == State::LoggedIn;
    }

private:
    std::unique_ptr<settings::Settings> m_settings;
    std::unique_ptr<connection::Connection> m_connection;
};

}  // namespace pictorio::e2e
