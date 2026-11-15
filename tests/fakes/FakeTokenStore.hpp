/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include <QHash>

#include "TokenStore.hpp"

// A keyring for tests, so they never touch the user's real one.
namespace pictorio::fakes
{

// The tokens by key, or no keyring at all on this system. Shared, so a test can look inside after
// handing the store over.
struct Keyring
{
    QHash<QString, QString> tokens;
    bool available = true;
};

class FakeTokenStore : public token_store::TokenStore
{
public:
    explicit FakeTokenStore(std::shared_ptr<Keyring> keyring)
        : m_keyring(std::move(keyring))
    {}

    QFuture<token_store::StoreResult<QString>> readToken(const QString& serverKey) override
    {
        if (!m_keyring->available)
        {
            return ready<QString>(std::unexpected(unavailable()));
        }

        return ready<QString>(m_keyring->tokens.value(serverKey));
    }

    QFuture<token_store::StoreResult<void>> writeToken(const QString& serverKey, const QString& token) override
    {
        if (!m_keyring->available)
        {
            return ready<void>(std::unexpected(unavailable()));
        }

        m_keyring->tokens.insert(serverKey, token);
        return ready<void>({});
    }

    QFuture<token_store::StoreResult<void>> deleteToken(const QString& serverKey) override
    {
        m_keyring->tokens.remove(serverKey);
        return ready<void>({});
    }

private:
    template <typename T> static QFuture<token_store::StoreResult<T>> ready(token_store::StoreResult<T> result)
    {
        return QtFuture::makeReadyValueFuture(std::move(result));
    }

    static token_store::TokenStoreError unavailable()
    {
        return {token_store::TokenStoreError::Kind::Unavailable, QStringLiteral("No keyring")};
    }

    std::shared_ptr<Keyring> m_keyring;
};

}  // namespace pictorio::fakes
