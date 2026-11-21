/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "TokenStoreFactory.hpp"

#include "impl/KeychainTokenStore.hpp"

namespace photobook::token_store
{

std::unique_ptr<TokenStore> createTokenStore()
{
    return std::make_unique<KeychainTokenStore>(QStringLiteral("io.github.oesusdevelopmentstudios.photobook"));
}

}  // namespace photobook::token_store
