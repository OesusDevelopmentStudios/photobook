/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "impl/AuthorizingNetworkAccessManager.hpp"

#include <QNetworkRequest>

namespace photobook::network_access
{

AuthorizingNetworkAccessManager::AuthorizingNetworkAccessManager(std::shared_ptr<const ServerCredentials> credentials,
                                                                 QObject* parent)
    : QNetworkAccessManager(parent)
    , m_credentials(std::move(credentials))
{}

QNetworkReply* AuthorizingNetworkAccessManager::createRequest(Operation operation,
                                                              const QNetworkRequest& request,
                                                              QIODevice* outgoingData)
{
    QNetworkRequest serverRequest(request);

    // Only thumbnails are kept on disk: a few full-size images would push thousands of them out of the cache.
    if (!request.url().path().endsWith(QLatin1String("/thumbnail")))
    {
        serverRequest.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
    }

    const QString token = m_credentials->tokenFor(request.url());
    if (!token.isEmpty())
    {
        serverRequest.setRawHeader("Authorization", "Bearer " + token.toUtf8());
    }

    return QNetworkAccessManager::createRequest(operation, serverRequest, outgoingData);
}

}  // namespace photobook::network_access
