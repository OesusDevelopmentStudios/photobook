/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "impl/QtHttpTransport.hpp"

#include <chrono>
#include <memory>

#include <QFile>
#include <QHttpMultiPart>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace
{

using pictorio::server_client::HttpResponse;
using pictorio::server_client::MultipartForm;

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

QHttpPart formPart(const QByteArray& contentDisposition)
{
    QHttpPart part;
    part.setRawHeader("Content-Disposition", contentDisposition);  // raw, so UTF-8 file names stay intact
    return part;
}

// Quotes would end the name early in the header; RFC 7578 percent-encodes them.
QByteArray quotedName(const QString& name)
{
    return '"' + name.toUtf8().replace('"', "%22") + '"';
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

    if (request.form.has_value())
    {
        return sendForm(networkRequest, request.method, *request.form);
    }

    return finished(m_network.sendCustomRequest(networkRequest, request.method, request.body));
}

QFuture<HttpResponse>
QtHttpTransport::sendForm(const QNetworkRequest& networkRequest, const QByteArray& method, const MultipartForm& form)
{
    auto multiPart = std::make_unique<QHttpMultiPart>(QHttpMultiPart::FormDataType);
    for (const auto& [name, value] : form.fields)
    {
        QHttpPart part = formPart("form-data; name=" + quotedName(QString::fromUtf8(name)));
        part.setBody(value);
        multiPart->append(part);
    }

    auto* file = new QFile(form.filePath, multiPart.get());  // read while it is sent, never all at once
    if (!file->open(QIODevice::ReadOnly))
    {
        HttpResponse response;
        response.networkError = QStringLiteral("Cannot read %1: %2").arg(form.filePath, file->errorString());
        return QtFuture::makeReadyValueFuture(response);
    }

    QHttpPart filePart = formPart("form-data; name=" + quotedName(QString::fromUtf8(form.fileField)) +
                                  "; filename=" + quotedName(form.fileName));
    filePart.setRawHeader("Content-Type", "application/octet-stream");  // the server looks at the content
    filePart.setBodyDevice(file);
    multiPart->append(filePart);

    QNetworkReply* reply = m_network.sendCustomRequest(networkRequest, method, multiPart.get());
    multiPart.release()->setParent(reply);  // lives as long as the request
    return finished(reply);
}

QFuture<HttpResponse> QtHttpTransport::finished(QNetworkReply* reply)
{
    return QtFuture::connect(reply, &QNetworkReply::finished).then([reply]() {
        const HttpResponse response = responseFrom(*reply);
        reply->deleteLater();
        return response;
    });
}

}  // namespace pictorio::server_client
