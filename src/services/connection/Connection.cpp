/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "Connection.hpp"

#include <KLocalizedString>

#include "hlp/ServerAddress.hpp"

namespace
{

using namespace photobook::server_client;
using photobook::connection::serverPage;
using photobook::token_store::StoreResult;
using photobook::token_store::TokenStoreError;

const QString expectedServerName = QStringLiteral("photobook-db");
constexpr int supportedApiVersion = 1;
constexpr int statusUnauthorized = 401;

// Why an address that answered is not a server this app can use; empty if it is one.
QString problemWithServer(const QUrl& serverUrl, const ServerInfo& info)
{
    if (info.name != expectedServerName)
    {
        return i18nc("@info %1 is an address", "%1 is not a photobook-db server.", serverUrl.toDisplayString());
    }

    if (info.apiVersion != supportedApiVersion)
    {
        return i18nc("@info %1 and %2 are version numbers",
                     "The server speaks version %1 of the photobook-db API, this Photobook version %2. Update the "
                     "older of the two.",
                     info.apiVersion,
                     supportedApiVersion);
    }

    if (info.setupRequired)
    {
        return i18nc("@info %1 is a web address",
                     "The server is not set up yet. Open %1 in a browser to create its admin account.",
                     serverPage(serverUrl, QStringLiteral("/admin/setup")).toDisplayString());
    }

    return {};
}

}  // namespace

namespace photobook::connection
{

Connection::Connection(ServerClientFactory serverClientFactory,
                       std::unique_ptr<token_store::TokenStore> tokenStore,
                       settings::Settings* settings,
                       QString appSpecificMachineId,
                       QObject* parent)
    : QObject(parent)
    , m_serverClientFactory(std::move(serverClientFactory))
    , m_tokenStore(std::move(tokenStore))
    , m_settings(settings)
    , m_appSpecificMachineId(std::move(appSpecificMachineId))
{}

Connection::~Connection() = default;

void Connection::restoreLogin()
{
    const std::optional<QUrl> serverUrl = m_settings->serverUrl();
    if (!serverUrl.has_value() || !serverUrl->isValid())
    {
        setState(State::NoServer);
        return;
    }

    m_serverUrl = *serverUrl;
    m_serverDescription.clear();
    m_client = m_serverClientFactory(m_serverUrl);
    m_username = m_settings->serverUsername().value_or(QString());
    Q_EMIT serverChanged();
    Q_EMIT userChanged();
    setErrorMessage({});
    setState(State::RestoringLogin);

    m_tokenStore->readToken(keyringKey()).then(this, [this](const StoreResult<QString>& token) {
        if (!token.has_value() || token->isEmpty())
        {
            if (!token.has_value())
            {
                setKeyringWarning(token.error());
            }

            setState(State::ServerFound);  // nothing kept: the user logs in
            return;
        }

        m_token = *token;
        m_client->setToken(m_token);
        m_client->currentUser().then(this, [this](const Result<CurrentUser>& currentUser) {
            if (currentUser.has_value())
            {
                m_username = currentUser->user.username;
                m_restrictedTags = currentUser->restrictedTags;
                Q_EMIT userChanged();
                setState(State::LoggedIn);
                askWhereTheLibraryIs();
                return;
            }

            if (currentUser.error().httpStatus == statusUnauthorized)  // revoked, or the password was changed
            {
                m_token.clear();
                m_client->setToken({});
                m_tokenStore->deleteToken(keyringKey());
                setErrorMessage(i18nc("@info", "Your login has ended. Log in again."));
                setState(State::ServerFound);
                return;
            }

            const ApiError& error = currentUser.error();
            setErrorMessage(describeServerError(error.code, error.title, error.detail));
            setState(State::Offline);  // the token is kept for when the server answers again
        });
    });
}

void Connection::checkServer(const QString& typedAddress)
{
    ++m_checkNumber;
    const QList<QUrl> candidates = serverUrlCandidates(typedAddress);
    if (candidates.isEmpty())
    {
        setErrorMessage(
            i18nc("@info", "Enter the server's address, e.g. photos.example.org or raspberrypi.local:8737."));
        setState(State::NoServer);
        return;
    }

    setErrorMessage({});
    setState(State::CheckingServer);
    tryServerAddress(candidates, 0, m_checkNumber);
}

void Connection::logIn(const QString& username, const QString& password)
{
    if (!m_client || (m_state != State::ServerFound && m_state != State::Offline))
    {
        return;
    }

    setErrorMessage({});
    setState(State::LoggingIn);
    m_client->logIn(username, password, deviceName()).then(this, [this](const Result<Login>& login) {
        if (!login.has_value())
        {
            setErrorMessage(describeServerError(login.error().code, login.error().title, login.error().detail));
            setState(State::ServerFound);
            return;
        }

        finishLogin(*login);
    });
}

void Connection::logOut()
{
    if (!m_client)
    {
        return;
    }

    revokeOnServer(m_token);
    m_token.clear();
    m_client->setToken({});
    m_tokenStore->deleteToken(keyringKey());
    m_restrictedTags.clear();
    Q_EMIT userChanged();
    setLocalLibraryDir({});
    setErrorMessage({});
    setState(State::ServerFound);
}

void Connection::changeServer()
{
    if (m_state == State::LoggedIn)
    {
        return;  // log out first
    }

    ++m_checkNumber;  // an answer still on its way is ignored
    m_candidateClient.reset();
    setErrorMessage({});
    setState(State::NoServer);
}

Connection::State Connection::state() const
{
    return m_state;
}

QString Connection::serverAddress() const
{
    return m_serverUrl.toDisplayString();
}

QUrl Connection::webAdminUrl() const
{
    return m_serverUrl.isEmpty() ? QUrl() : serverPage(m_serverUrl, QStringLiteral("/admin/login"));
}

QString Connection::serverDescription() const
{
    return m_serverDescription;
}

bool Connection::isPlainHttp() const
{
    return m_serverUrl.scheme() == QLatin1String("http");
}

QString Connection::username() const
{
    return m_username;
}

QStringList Connection::restrictedTags() const
{
    return m_restrictedTags;
}

QString Connection::localLibraryDir() const
{
    return m_localLibraryDir;
}

QString Connection::token() const
{
    return m_state == State::LoggedIn ? m_token : QString();
}

QString Connection::errorMessage() const
{
    return m_errorMessage;
}

QString Connection::keyringWarning() const
{
    return m_keyringWarning;
}

server_client::ServerClient* Connection::client() const
{
    return m_client.get();
}

void Connection::tryServerAddress(const QList<QUrl>& candidates, qsizetype index, int checkNumber)
{
    const QUrl serverUrl = candidates.at(index);
    m_candidateClient = m_serverClientFactory(serverUrl);
    m_candidateClient->serverInfo().then(
        this, [this, candidates, index, checkNumber, serverUrl](const Result<ServerInfo>& info) {
            if (checkNumber != m_checkNumber)
            {
                return;  // the user has asked for something else since
            }

            const bool noAnswer = !info.has_value() && info.error().code == networkErrorCode;
            if (noAnswer && index + 1 < candidates.size())
            {
                tryServerAddress(candidates, index + 1, checkNumber);  // e.g. no https: try plain http
                return;
            }

            if (!info.has_value())
            {
                const ApiError& error = info.error();
                setErrorMessage(noAnswer ? describeServerError(error.code, error.title, error.detail)
                                         : i18nc("@info %1 is an address",
                                                 "%1 is not a photobook-db server.",
                                                 serverUrl.toDisplayString()));
                setState(State::NoServer);
                return;
            }

            const QString problem = problemWithServer(serverUrl, *info);
            if (!problem.isEmpty())
            {
                setErrorMessage(problem);
                setState(State::NoServer);
                return;
            }

            acceptServer(serverUrl, std::move(m_candidateClient), *info);
        });
}

void Connection::acceptServer(const QUrl& serverUrl,
                              std::unique_ptr<server_client::ServerClient> client,
                              const server_client::ServerInfo& info)
{
    m_serverUrl = serverUrl;
    m_client = std::move(client);
    m_serverDescription = QStringLiteral("%1 %2").arg(info.name, info.version);
    Q_EMIT serverChanged();
    setState(State::ServerFound);
}

void Connection::finishLogin(const server_client::Login& login)
{
    m_token = login.token;
    m_client->setToken(m_token);
    m_username = login.user.username;
    m_restrictedTags.clear();
    m_settings->setServerUrl(m_serverUrl);
    m_settings->setServerUsername(m_username);
    Q_EMIT userChanged();
    keepToken(m_token);
    setState(State::LoggedIn);
    readCurrentUser();  // the restricted tags; until they arrive, the server still refuses what it must
    askWhereTheLibraryIs();
}

void Connection::keepToken(const QString& token)
{
    m_keyringWarning.clear();
    Q_EMIT keyringWarningChanged();
    m_tokenStore->writeToken(keyringKey(), token).then(this, [this](const StoreResult<void>& kept) {
        if (!kept.has_value())
        {
            setKeyringWarning(kept.error());  // logged in all the same, until the app ends
        }
    });
}

void Connection::readCurrentUser()
{
    m_client->currentUser().then(this, [this](const Result<CurrentUser>& currentUser) {
        if (currentUser.has_value())
        {
            m_restrictedTags = currentUser->restrictedTags;
            Q_EMIT userChanged();
        }
    });
}

// Only a server on this computer answers with its library folder; any other answer leaves it unknown.
void Connection::askWhereTheLibraryIs()
{
    if (m_appSpecificMachineId.isEmpty())
    {
        return;
    }

    m_client->localLibrary(m_appSpecificMachineId).then(this, [this](const Result<LocalLibrary>& library) {
        if (m_state == State::LoggedIn)
        {
            setLocalLibraryDir(library.has_value() ? library->libraryDir : QString());
        }
    });
}

void Connection::setLocalLibraryDir(const QString& folder)
{
    if (folder != m_localLibraryDir)
    {
        m_localLibraryDir = folder;
        Q_EMIT localLibraryDirChanged();
    }
}

void Connection::revokeOnServer(const QString& token)
{
    if (token.isEmpty())
    {
        return;
    }

    // A client of its own, which keeps the token while the requests are on their way.
    std::shared_ptr<server_client::ServerClient> revoker = m_serverClientFactory(m_serverUrl);
    revoker->setToken(token);
    revoker->devices().then(this, [revoker](const Result<QList<DeviceToken>>& devices) {
        if (!devices.has_value())
        {
            return;  // offline: the user can revoke it later from another device or the admin pages
        }

        for (const DeviceToken& device : *devices)
        {
            if (device.isCurrent)
            {
                revoker->revokeDevice(device.tokenId).then([revoker](const Result<void>&) {});
                return;
            }
        }
    });
}

void Connection::setState(State state)
{
    if (m_state != state)
    {
        m_state = state;
        Q_EMIT stateChanged();
    }
}

void Connection::setErrorMessage(const QString& message)
{
    if (m_errorMessage != message)
    {
        m_errorMessage = message;
        Q_EMIT errorMessageChanged();
    }
}

void Connection::setKeyringWarning(const TokenStoreError& error)
{
    switch (error.kind)
    {
    case TokenStoreError::Kind::Unavailable:
        m_keyringWarning = i18nc("@info",
                                 "No keyring is available, so you will have to log in again when Photobook "
                                 "starts.");
        break;
    case TokenStoreError::Kind::AccessDenied:
        m_keyringWarning = i18nc("@info",
                                 "Photobook may not use the keyring, so you will have to log in again when "
                                 "it starts.");
        break;
    case TokenStoreError::Kind::Failed:
        m_keyringWarning =
            i18nc("@info %1 is the keyring's message", "The login could not be kept in the keyring: %1", error.message);
        break;
    }

    Q_EMIT keyringWarningChanged();
}

QString Connection::keyringKey() const
{
    return m_serverUrl.adjusted(QUrl::StripTrailingSlash).toString();
}

}  // namespace photobook::connection
