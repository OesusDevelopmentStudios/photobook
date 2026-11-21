/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include "TokenStore.hpp"

namespace photobook::token_store
{

std::unique_ptr<TokenStore> createTokenStore();

}  // namespace photobook::token_store
