/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "impl/RestServerClient.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>

#include "hlp/ApiJson.hpp"
#include "hlp/ProblemDetails.hpp"

namespace
{

using namespace pictorio::server_client;

const QString apiPath = QStringLiteral("/api/v1");

constexpr int statusOk = 200;
constexpr int statusCreated = 201;
constexpr int statusNoContent = 204;

// The answer as parse() reads it, if it has the expected status; otherwise the error it stands for.
template <typename Parse>
auto jsonAnswer(const HttpResponse& response, int expectedStatus, Parse parse) -> decltype(parse(QJsonValue()))
{
    if (response.status != expectedStatus)
    {
        return std::unexpected(apiErrorFrom(response));
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(response.body, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        return std::unexpected(invalidResponse(QStringLiteral("not JSON: %1").arg(parseError.errorString())));
    }

    return parse(document.isArray() ? QJsonValue(document.array()) : QJsonValue(document.object()));
}

Result<void> emptyAnswer(const HttpResponse& response, int expectedStatus)
{
    if (response.status != expectedStatus)
    {
        return std::unexpected(apiErrorFrom(response));
    }

    return {};
}

}  // namespace

namespace pictorio::server_client
{

RestServerClient::RestServerClient(const QUrl& serverUrl, std::shared_ptr<HttpTransport> transport)
    : m_serverUrl(serverUrl)
    , m_transport(std::move(transport))
{}

QUrl RestServerClient::serverUrl() const
{
    return m_serverUrl;
}

void RestServerClient::setToken(const QString& token)
{
    m_token = token;
}

QFuture<Result<ServerInfo>> RestServerClient::serverInfo()
{
    return send("GET", QStringLiteral("/server/info")).then([](const HttpResponse& response) {
        return jsonAnswer(response, statusOk, serverInfoFrom);
    });
}

QFuture<Result<Login>>
RestServerClient::logIn(const QString& username, const QString& password, const QString& deviceName)
{
    const QJsonObject body{
        {QStringLiteral("username"), username},
        {QStringLiteral("password"), password},
        {QStringLiteral("device_name"), deviceName},
    };
    return send("POST", QStringLiteral("/auth/tokens"), body).then([](const HttpResponse& response) {
        return jsonAnswer(response, statusCreated, loginFrom);
    });
}

QFuture<Result<CurrentUser>> RestServerClient::currentUser()
{
    return send("GET", QStringLiteral("/auth/me")).then([](const HttpResponse& response) {
        return jsonAnswer(response, statusOk, currentUserFrom);
    });
}

QFuture<Result<CurrentUser>> RestServerClient::changeOwnPassword(const QString& currentPassword,
                                                                 const QString& newPassword)
{
    const QJsonObject body{
        {QStringLiteral("current_password"), currentPassword},
        {QStringLiteral("new_password"), newPassword},
    };
    return send("PATCH", QStringLiteral("/auth/me"), body).then([](const HttpResponse& response) {
        return jsonAnswer(response, statusOk, currentUserFrom);
    });
}

QFuture<Result<QList<DeviceToken>>> RestServerClient::devices()
{
    return send("GET", QStringLiteral("/auth/tokens")).then([](const HttpResponse& response) {
        return jsonAnswer(response, statusOk, devicesFrom);
    });
}

QFuture<Result<void>> RestServerClient::revokeDevice(int tokenId)
{
    return send("DELETE", QStringLiteral("/auth/tokens/%1").arg(tokenId)).then([](const HttpResponse& response) {
        return emptyAnswer(response, statusNoContent);
    });
}

QFuture<HttpResponse>
RestServerClient::send(const QByteArray& method, const QString& path, const std::optional<QJsonObject>& body) const
{
    HttpRequest request;
    request.method = method;
    request.url = endpointUrl(path);
    request.headers.emplaceBack("Accept", "application/json");
    if (!m_token.isEmpty())
    {
        request.headers.emplaceBack("Authorization", "Bearer " + m_token.toUtf8());
    }

    if (body.has_value())
    {
        request.headers.emplaceBack("Content-Type", "application/json");
        request.body = QJsonDocument(*body).toJson(QJsonDocument::Compact);
    }

    return m_transport->send(request);
}

QUrl RestServerClient::endpointUrl(const QString& path) const
{
    QUrl url = m_serverUrl;
    QString basePath = url.path();
    while (basePath.endsWith(QLatin1Char('/')))
    {
        basePath.chop(1);
    }

    url.setPath(basePath + apiPath + path);
    url.setQuery(QString());
    url.setFragment(QString());
    return url;
}

}  // namespace pictorio::server_client
