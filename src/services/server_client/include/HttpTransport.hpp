/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <QByteArray>
#include <QFuture>
#include <QList>
#include <QPair>
#include <QString>
#include <QUrl>

namespace pictorio::server_client
{

struct HttpRequest
{
    QByteArray method;  // "GET", "POST"…
    QUrl url;
    QList<QPair<QByteArray, QByteArray>> headers;
    QByteArray body;
};

struct HttpResponse
{
    int status = 0;  // 0 if no answer arrived
    QList<QPair<QByteArray, QByteArray>> headers;
    QByteArray body;
    QString networkError;  // why no answer arrived; empty if one did

    QByteArray header(const QByteArray& name) const;  // case-insensitive; empty if missing
};

// Sends HTTP requests. The server client builds the requests and reads the answers; this only carries
// them, so tests can answer with canned responses instead of a network.
class HttpTransport
{
public:
    virtual ~HttpTransport();

    virtual QFuture<HttpResponse> send(const HttpRequest& request) = 0;
};

}  // namespace pictorio::server_client
