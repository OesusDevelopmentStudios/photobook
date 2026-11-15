/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>
#include <optional>

#include <QJsonObject>

#include "HttpTransport.hpp"
#include "ServerClient.hpp"

namespace pictorio::server_client
{

// Builds the requests of the REST API and reads its answers; the transport carries them.
class RestServerClient : public ServerClient
{
public:
    RestServerClient(const QUrl& serverUrl, std::shared_ptr<HttpTransport> transport);

    QUrl serverUrl() const override;
    void setToken(const QString& token) override;

    QFuture<Result<ServerInfo>> serverInfo() override;
    QFuture<Result<Login>> logIn(const QString& username, const QString& password, const QString& deviceName) override;
    QFuture<Result<CurrentUser>> currentUser() override;
    QFuture<Result<CurrentUser>> changeOwnPassword(const QString& currentPassword, const QString& newPassword) override;
    QFuture<Result<QList<DeviceToken>>> devices() override;
    QFuture<Result<void>> revokeDevice(int tokenId) override;

private:
    QFuture<HttpResponse>
    send(const QByteArray& method, const QString& path, const std::optional<QJsonObject>& body = std::nullopt) const;
    QUrl endpointUrl(const QString& path) const;

    QUrl m_serverUrl;
    QString m_token;
    std::shared_ptr<HttpTransport> m_transport;
};

}  // namespace pictorio::server_client
