/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <QFuture>
#include <QString>
#include <QUrl>

#include "ApiTypes.hpp"

namespace pictorio::server_client
{

// The pictorio-db REST API (version 1): one function per endpoint the app uses. Each returns at once; the
// future finishes with the parsed answer or an ApiError. The functions only talk to the server: storing
// the token, retrying or showing errors is up to their callers.
class ServerClient
{
public:
    virtual ~ServerClient();

    virtual QUrl serverUrl() const = 0;

    // Sent with every request; empty: not logged in.
    virtual void setToken(const QString& token) = 0;

    // GET /server/info, works without a login
    virtual QFuture<Result<ServerInfo>> serverInfo() = 0;

    // POST /auth/tokens: logs this device in. Does not set the token; the caller stores it first.
    virtual QFuture<Result<Login>>
    logIn(const QString& username, const QString& password, const QString& deviceName) = 0;

    // GET /auth/me
    virtual QFuture<Result<CurrentUser>> currentUser() = 0;

    // PATCH /auth/me: the user's other devices are logged out, this one stays logged in
    virtual QFuture<Result<CurrentUser>> changeOwnPassword(const QString& currentPassword,
                                                           const QString& newPassword) = 0;

    // GET /auth/tokens
    virtual QFuture<Result<QList<DeviceToken>>> devices() = 0;

    // DELETE /auth/tokens/{id}; revoking the current token logs this app out
    virtual QFuture<Result<void>> revokeDevice(int tokenId) = 0;
};

}  // namespace pictorio::server_client
