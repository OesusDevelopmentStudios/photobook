/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include "HttpTransport.hpp"

namespace pictorio::server_client
{

// Sends the requests over the network with Qt. Used from the thread that created it, like any
// QNetworkAccessManager; the futures finish in that thread.
class QtHttpTransport : public HttpTransport
{
public:
    QFuture<HttpResponse> send(const HttpRequest& request) override;

private:
    QFuture<HttpResponse>
    sendForm(const QNetworkRequest& networkRequest, const QByteArray& method, const MultipartForm& form);
    static QFuture<HttpResponse> finished(QNetworkReply* reply);

    QNetworkAccessManager m_network;
};

}  // namespace pictorio::server_client
