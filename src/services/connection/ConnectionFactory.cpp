/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "ConnectionFactory.hpp"

#include "ServerClientFactory.hpp"
#include "TokenStoreFactory.hpp"

namespace pictorio::connection
{

std::unique_ptr<Connection> createConnection(settings::Settings* settings)
{
    auto serverClientFactory = [](const QUrl& serverUrl) { return server_client::createServerClient(serverUrl); };
    return std::make_unique<Connection>(serverClientFactory, token_store::createTokenStore(), settings);
}

}  // namespace pictorio::connection
