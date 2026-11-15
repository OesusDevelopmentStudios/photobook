/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <qt6keychain/keychain.h>

#include "TokenStore.hpp"

namespace pictorio::token_store
{

// What a failed keyring job means for the app. Not for NoError, nor for EntryNotFound, which callers
// treat as "nothing kept".
TokenStoreError errorFrom(QKeychain::Error error, const QString& errorString);

}  // namespace pictorio::token_store
