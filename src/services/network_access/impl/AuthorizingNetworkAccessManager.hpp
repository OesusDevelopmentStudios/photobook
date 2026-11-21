/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include <QNetworkAccessManager>

#include "NetworkAccess.hpp"

namespace photobook::network_access
{

// Adds the login token to the requests that go to the server, and only to those.
class AuthorizingNetworkAccessManager : public QNetworkAccessManager
{
public:
    AuthorizingNetworkAccessManager(std::shared_ptr<const ServerCredentials> credentials, QObject* parent);

protected:
    QNetworkReply* createRequest(Operation operation, const QNetworkRequest& request, QIODevice* outgoingData) override;

private:
    std::shared_ptr<const ServerCredentials> m_credentials;
};

}  // namespace photobook::network_access
