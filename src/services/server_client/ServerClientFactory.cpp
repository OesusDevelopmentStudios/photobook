/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "ServerClientFactory.hpp"

#include "impl/QtHttpTransport.hpp"
#include "impl/RestServerClient.hpp"

namespace pictorio::server_client
{

std::unique_ptr<ServerClient> createServerClient(const QUrl& serverUrl)
{
    return createServerClient(serverUrl, std::make_shared<QtHttpTransport>());
}

std::unique_ptr<ServerClient> createServerClient(const QUrl& serverUrl, std::shared_ptr<HttpTransport> transport)
{
    return std::make_unique<RestServerClient>(serverUrl, std::move(transport));
}

}  // namespace pictorio::server_client
