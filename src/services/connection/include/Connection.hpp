/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <functional>
#include <memory>

#include <QObject>
#include <QStringList>
#include <QUrl>

#include "ServerClient.hpp"
#include "Settings.hpp"
#include "TokenStore.hpp"

namespace pictorio::connection
{

// The connection to a pictorio-db server: finding it, logging in, staying logged in after a restart and
// logging out. The pages show its state; the services that talk to the server use client().
class Connection : public QObject
{
    Q_OBJECT

    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString serverAddress READ serverAddress NOTIFY serverChanged)
    Q_PROPERTY(QString serverDescription READ serverDescription NOTIFY serverChanged)
    Q_PROPERTY(bool plainHttp READ isPlainHttp NOTIFY serverChanged)
    Q_PROPERTY(QString username READ username NOTIFY userChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString keyringWarning READ keyringWarning NOTIFY keyringWarningChanged)

public:
    enum class State
    {
        NoServer,        // no server chosen yet, or the address did not work
        CheckingServer,  // asking the address whether it is a pictorio-db server
        ServerFound,     // the server is known; the user can log in
        LoggingIn,
        RestoringLogin,  // at start: checking the login kept from the last run
        Offline,         // logged in last time, but the server does not answer now
        LoggedIn,
    };
    Q_ENUM(State)

    // Makes the client for a server address; tests answer from canned responses through it.
    using ServerClientFactory = std::function<std::unique_ptr<server_client::ServerClient>(const QUrl&)>;

    // The settings are not owned and must outlive the connection.
    Connection(ServerClientFactory serverClientFactory,
               std::unique_ptr<token_store::TokenStore> tokenStore,
               settings::Settings* settings,
               QObject* parent = nullptr);
    ~Connection() override;

    // At start: logs in again with the token kept from the last run, if there is one.
    Q_INVOKABLE void restoreLogin();
    // "photos.example.org" or "raspberrypi.local:8737": without http:// or https://, https is tried first.
    Q_INVOKABLE void checkServer(const QString& typedAddress);
    Q_INVOKABLE void logIn(const QString& username, const QString& password);
    // Ends this device's login on the server too, if it answers; the local token is deleted in any case.
    Q_INVOKABLE void logOut();
    // Back to choosing a server, e.g. after a typo in the address.
    Q_INVOKABLE void changeServer();

    State state() const;
    QString serverAddress() const;
    QString serverDescription() const;  // e.g. "pictorio-db 0.1.0"
    bool isPlainHttp() const;           // passwords and images travel unencrypted
    QString username() const;
    QStringList restrictedTags() const;  // what the logged-in user may not see or use
    QString errorMessage() const;        // why the last step failed; empty if it did not
    QString keyringWarning() const;      // set if the login cannot be kept for the next start

    // The client of the current server; logged in while the state is LoggedIn. Null before a server is found.
    server_client::ServerClient* client() const;

Q_SIGNALS:
    void stateChanged();
    void serverChanged();
    void userChanged();
    void errorMessageChanged();
    void keyringWarningChanged();

private:
    void tryServerAddress(const QList<QUrl>& candidates, qsizetype index, int checkNumber);
    void acceptServer(const QUrl& serverUrl,
                      std::unique_ptr<server_client::ServerClient> client,
                      const server_client::ServerInfo& info);
    void finishLogin(const server_client::Login& login);
    void keepToken(const QString& token);
    void readCurrentUser();
    void revokeOnServer(const QString& token);
    void setState(State state);
    void setErrorMessage(const QString& message);
    void setKeyringWarning(const token_store::TokenStoreError& error);
    QString keyringKey() const;

    ServerClientFactory m_serverClientFactory;
    std::unique_ptr<token_store::TokenStore> m_tokenStore;
    settings::Settings* m_settings;

    State m_state = State::NoServer;
    QUrl m_serverUrl;
    QString m_serverDescription;
    std::unique_ptr<server_client::ServerClient> m_client;
    std::unique_ptr<server_client::ServerClient> m_candidateClient;  // the address being checked
    int m_checkNumber = 0;                                           // answers to an older check are ignored
    QString m_token;
    QString m_username;
    QStringList m_restrictedTags;
    QString m_errorMessage;
    QString m_keyringWarning;
};

}  // namespace pictorio::connection
