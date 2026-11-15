/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "hlp/ProblemDetails.hpp"

#include <QJsonArray>
#include <QJsonDocument>

namespace
{

using pictorio::server_client::ApiError;
using pictorio::server_client::FieldError;

QList<FieldError> fieldErrorsIn(const QJsonObject& problem)
{
    QList<FieldError> fieldErrors;
    for (const QJsonValue& item : problem.value(QStringLiteral("errors")).toArray())
    {
        const QJsonObject error = item.toObject();
        fieldErrors.append(
            {error.value(QStringLiteral("field")).toString(), error.value(QStringLiteral("message")).toString()});
    }

    return fieldErrors;
}

}  // namespace

namespace pictorio::server_client
{

ApiError apiErrorFrom(const HttpResponse& response)
{
    ApiError error;
    if (!response.networkError.isEmpty())
    {
        error.code = networkErrorCode;
        error.title = QStringLiteral("No answer from the server");
        error.detail = response.networkError;
        return error;
    }

    error.httpStatus = response.status;
    const QJsonObject problem = QJsonDocument::fromJson(response.body).object();
    if (!problem.value(QStringLiteral("code")).isString())
    {
        error.code = QStringLiteral("http_%1").arg(response.status);
        error.title = QStringLiteral("Unexpected answer from the server");
        error.detail = QStringLiteral("HTTP status %1").arg(response.status);
        return error;
    }

    error.code = problem.value(QStringLiteral("code")).toString();
    error.title = problem.value(QStringLiteral("title")).toString();
    error.detail = problem.value(QStringLiteral("detail")).toString();
    error.fieldErrors = fieldErrorsIn(problem);
    error.problem = problem;
    return error;
}

ApiError invalidResponse(const QString& reason)
{
    ApiError error;
    error.code = invalidResponseCode;
    error.title = QStringLiteral("Unexpected answer from the server");
    error.detail = reason;
    return error;
}

}  // namespace pictorio::server_client
