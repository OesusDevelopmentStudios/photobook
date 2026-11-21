/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <optional>

#include <QByteArray>
#include <QFuture>
#include <QList>
#include <QPair>
#include <QString>
#include <QUrl>

namespace photobook::server_client
{

// A multipart/form-data body: text fields, and one file read from disk while it is sent.
struct MultipartForm
{
    QList<QPair<QByteArray, QByteArray>> fields;  // name, value
    QByteArray fileField;
    QString filePath;
    QString fileName;  // as the server sees it
};

struct HttpRequest
{
    QByteArray method;
    QUrl url;
    QList<QPair<QByteArray, QByteArray>> headers;
    QByteArray body;
    std::optional<MultipartForm> form;  // instead of body
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

}  // namespace photobook::server_client
