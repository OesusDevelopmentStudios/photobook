/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include "TokenStore.hpp"

namespace pictorio::token_store
{

// The tokens in the system keyring, under the application's name.
std::unique_ptr<TokenStore> createTokenStore();

}  // namespace pictorio::token_store
