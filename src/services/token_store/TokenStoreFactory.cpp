/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "TokenStoreFactory.hpp"

#include "impl/KeychainTokenStore.hpp"

namespace pictorio::token_store
{

std::unique_ptr<TokenStore> createTokenStore()
{
    return std::make_unique<KeychainTokenStore>(QStringLiteral("io.github.oesusdevelopmentstudios.pictorio"));
}

}  // namespace pictorio::token_store
