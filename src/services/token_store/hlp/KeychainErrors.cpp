/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "hlp/KeychainErrors.hpp"

namespace pictorio::token_store
{

TokenStoreError errorFrom(QKeychain::Error error, const QString& errorString)
{
    TokenStoreError storeError;
    storeError.message = errorString;
    switch (error)
    {
    case QKeychain::NoBackendAvailable:
    case QKeychain::NotImplemented:
        storeError.kind = TokenStoreError::Kind::Unavailable;
        break;
    case QKeychain::AccessDeniedByUser:
    case QKeychain::AccessDenied:
        storeError.kind = TokenStoreError::Kind::AccessDenied;
        break;
    default:
        storeError.kind = TokenStoreError::Kind::Failed;
        break;
    }

    return storeError;
}

}  // namespace pictorio::token_store
