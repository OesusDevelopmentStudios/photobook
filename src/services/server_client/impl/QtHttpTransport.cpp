/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "impl/QtHttpTransport.hpp"

#include <chrono>

#include <QNetworkReply>
#include <QNetworkRequest>

namespace
{

using pictorio::server_client::HttpResponse;

// A request is given up when no data moves for this long; a long upload that keeps moving is fine.
constexpr std::chrono::seconds transferTimeout{30};

HttpResponse responseFrom(QNetworkReply& reply)
{
    HttpResponse response;
    response.status = reply.attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (response.status == 0)  // no answer at all; an HTTP error status is an answer
    {
        response.networkError = reply.errorString();
        return response;
    }

    for (const QNetworkReply::RawHeaderPair& header : reply.rawHeaderPairs())
    {
        response.headers.append({header.first, header.second});
    }

    response.body = reply.readAll();
    return response;
}

}  // namespace

namespace pictorio::server_client
{

QFuture<HttpResponse> QtHttpTransport::send(const HttpRequest& request)
{
    QNetworkRequest networkRequest(request.url);
    networkRequest.setTransferTimeout(transferTimeout);
    for (const auto& [name, value] : request.headers)
    {
        networkRequest.setRawHeader(name, value);
    }

    QNetworkReply* reply = m_network.sendCustomRequest(networkRequest, request.method, request.body);
    return QtFuture::connect(reply, &QNetworkReply::finished).then([reply]() {
        const HttpResponse response = responseFrom(*reply);
        reply->deleteLater();
        return response;
    });
}

}  // namespace pictorio::server_client
