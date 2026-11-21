/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "impl/KeychainTokenStore.hpp"

#include <qt6keychain/keychain.h>

#include "hlp/KeychainErrors.hpp"

namespace
{

using photobook::token_store::StoreResult;

// The job's outcome once it has finished. Jobs delete themselves after their finished signal, so this runs
// while the job still exists.
template <typename Read> auto whenFinished(QKeychain::Job* job, Read readOutcome)
{
    QFuture<decltype(readOutcome(job))> outcome =
        QtFuture::connect(job, &QKeychain::Job::finished).then([readOutcome](QKeychain::Job* finishedJob) {
            return readOutcome(finishedJob);
        });
    job->start();
    return outcome;
}

}  // namespace

namespace photobook::token_store
{

KeychainTokenStore::KeychainTokenStore(const QString& serviceName)
    : m_serviceName(serviceName)
{}

QFuture<StoreResult<QString>> KeychainTokenStore::readToken(const QString& serverKey)
{
    auto* job = new QKeychain::ReadPasswordJob(m_serviceName);
    job->setKey(serverKey);
    return whenFinished(job, [](QKeychain::Job* finishedJob) -> StoreResult<QString> {
        if (finishedJob->error() == QKeychain::EntryNotFound)
        {
            return QString();
        }

        if (finishedJob->error() != QKeychain::NoError)
        {
            return std::unexpected(errorFrom(finishedJob->error(), finishedJob->errorString()));
        }

        return static_cast<QKeychain::ReadPasswordJob*>(finishedJob)->textData();
    });
}

QFuture<StoreResult<void>> KeychainTokenStore::writeToken(const QString& serverKey, const QString& token)
{
    auto* job = new QKeychain::WritePasswordJob(m_serviceName);
    job->setKey(serverKey);
    job->setTextData(token);
    return whenFinished(job, [](QKeychain::Job* finishedJob) -> StoreResult<void> {
        if (finishedJob->error() != QKeychain::NoError)
        {
            return std::unexpected(errorFrom(finishedJob->error(), finishedJob->errorString()));
        }

        return {};
    });
}

QFuture<StoreResult<void>> KeychainTokenStore::deleteToken(const QString& serverKey)
{
    auto* job = new QKeychain::DeletePasswordJob(m_serviceName);
    job->setKey(serverKey);
    return whenFinished(job, [](QKeychain::Job* finishedJob) -> StoreResult<void> {
        const QKeychain::Error error = finishedJob->error();
        if (error != QKeychain::NoError && error != QKeychain::EntryNotFound)
        {
            return std::unexpected(errorFrom(error, finishedJob->errorString()));
        }

        return {};
    });
}

}  // namespace photobook::token_store
