/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <expected>

#include <QFuture>
#include <QString>

namespace pictorio::token_store
{

struct TokenStoreError
{
    enum class Kind
    {
        Unavailable,   // no keyring on this system: the user logs in at every start
        AccessDenied,  // the user or the system refused access to the keyring
        Failed,        // anything else; message says what
    };

    Kind kind = Kind::Failed;
    QString message;
};

template <typename T> using StoreResult = std::expected<T, TokenStoreError>;

// In the system keyring, never in a plain file; one entry per server
class TokenStore
{
public:
    virtual ~TokenStore();

    // Empty if none is kept
    virtual QFuture<StoreResult<QString>> readToken(const QString& serverKey) = 0;
    virtual QFuture<StoreResult<void>> writeToken(const QString& serverKey, const QString& token) = 0;
    // Nothing kept is fine too.
    virtual QFuture<StoreResult<void>> deleteToken(const QString& serverKey) = 0;
};

}  // namespace pictorio::token_store
