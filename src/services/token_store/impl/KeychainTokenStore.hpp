/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include "TokenStore.hpp"

namespace photobook::token_store
{

class KeychainTokenStore : public TokenStore
{
public:
    explicit KeychainTokenStore(const QString& serviceName);

    QFuture<StoreResult<QString>> readToken(const QString& serverKey) override;
    QFuture<StoreResult<void>> writeToken(const QString& serverKey, const QString& token) override;
    QFuture<StoreResult<void>> deleteToken(const QString& serverKey) override;

private:
    QString m_serviceName;
};

}  // namespace photobook::token_store
