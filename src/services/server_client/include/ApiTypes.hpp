/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <expected>
#include <optional>

#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

namespace pictorio::server_client
{

// Error codes made by the client itself; every other code comes from the server.
inline const QString networkErrorCode = QStringLiteral("network_error");        // no answer, e.g. offline
inline const QString invalidResponseCode = QStringLiteral("invalid_response");  // not what the API promises

struct FieldError
{
    QString field;  // e.g. "body.password"
    QString message;
};

// Why a request failed: the server's Problem Details (RFC 9457), or a network or response error. The app
// branches on `code` (e.g. "tag_restricted", "duplicate_image"), never on the title.
struct ApiError
{
    int httpStatus = 0;  // 0 if no answer arrived
    QString code;
    QString title;
    QString detail;
    QList<FieldError> fieldErrors;  // for validation errors
    QJsonObject problem;            // the whole answer, for fields that only some codes have
};

template <typename T> using Result = std::expected<T, ApiError>;

enum class Role
{
    User,
    Admin,
};

struct ServerInfo
{
    QString name;
    QString version;
    int apiVersion = 0;
    bool setupRequired = false;  // no admin account yet: the server must be set up in a browser first
};

struct UserSummary
{
    int id = 0;
    QString username;
    Role role = Role::User;
};

struct CurrentUser
{
    UserSummary user;
    QStringList restrictedTags;  // search terms; the app refuses these tags before sending anything
};

struct Login
{
    int tokenId = 0;
    QString token;  // the only time the token is available; store it safely
    UserSummary user;
};

// A logged-in device of the current user
struct DeviceToken
{
    int tokenId = 0;
    QString deviceName;
    QDateTime createdAt;  // UTC
    QDateTime lastUsedAt;
    std::optional<QDateTime> expiresAt;  // only web admin logins end by themselves
    bool isCurrent = false;              // the token this app is using
};

}  // namespace pictorio::server_client
