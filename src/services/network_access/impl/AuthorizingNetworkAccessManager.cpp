/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "impl/AuthorizingNetworkAccessManager.hpp"

#include <QNetworkRequest>

namespace pictorio::network_access
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
    const QString token = m_credentials->tokenFor(request.url());
    if (token.isEmpty())
    {
        return QNetworkAccessManager::createRequest(operation, request, outgoingData);
    }

    QNetworkRequest authorizedRequest(request);
    authorizedRequest.setRawHeader("Authorization", "Bearer " + token.toUtf8());
    return QNetworkAccessManager::createRequest(operation, authorizedRequest, outgoingData);
}

}  // namespace pictorio::network_access
