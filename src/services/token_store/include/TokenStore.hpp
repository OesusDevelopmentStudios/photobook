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

// Keeps a server's login token in the system keyring (KWallet or the Secret Service), so the app stays
// logged in after a restart. The token is never written to a plain file. Each server has its own entry.
class TokenStore
{
public:
    virtual ~TokenStore();

    // The token kept for the server; empty if there is none.
    virtual QFuture<StoreResult<QString>> readToken(const QString& serverKey) = 0;
    virtual QFuture<StoreResult<void>> writeToken(const QString& serverKey, const QString& token) = 0;
    // Nothing kept is fine too.
    virtual QFuture<StoreResult<void>> deleteToken(const QString& serverKey) = 0;
};

}  // namespace pictorio::token_store
