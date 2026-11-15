/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "NetworkAccess.hpp"

#include <QMutexLocker>
#include <QNetworkDiskCache>

#include "impl/AuthorizingNetworkAccessManager.hpp"

namespace
{

// About 25,000 small thumbnails; the oldest are removed beyond it.
constexpr qint64 maximumCacheBytes = 500LL * 1024 * 1024;

int portOf(const QUrl& url)
{
    return url.port(url.scheme() == QLatin1String("https") ? 443 : 80);
}

// The same scheme, host and port, and a path at or below the server's.
bool belongsToServer(const QUrl& url, const QUrl& serverUrl)
{
    if (url.scheme() != serverUrl.scheme() || url.host().compare(serverUrl.host(), Qt::CaseInsensitive) != 0 ||
        portOf(url) != portOf(serverUrl))
    {
        return false;
    }

    QString serverPath = serverUrl.path();
    while (serverPath.endsWith(QLatin1Char('/')))
    {
        serverPath.chop(1);
    }

    return url.path() == serverPath || url.path().startsWith(serverPath + QLatin1Char('/'));
}

class NetworkAccessFactory : public QQmlNetworkAccessManagerFactory
{
public:
    NetworkAccessFactory(std::shared_ptr<const pictorio::network_access::ServerCredentials> credentials,
                         const QString& cacheDirectory)
        : m_credentials(std::move(credentials))
        , m_cacheDirectory(cacheDirectory)
    {}

    QNetworkAccessManager* create(QObject* parent) override
    {
        return pictorio::network_access::createNetworkAccessManager(m_credentials, m_cacheDirectory, parent);
    }

private:
    std::shared_ptr<const pictorio::network_access::ServerCredentials> m_credentials;
    QString m_cacheDirectory;
};

}  // namespace

namespace pictorio::network_access
{

void ServerCredentials::setLogin(const QUrl& serverUrl, const QString& token)
{
    const QMutexLocker locker(&m_mutex);
    m_serverUrl = serverUrl;
    m_token = token;
}

void ServerCredentials::clear()
{
    setLogin({}, {});
}

QString ServerCredentials::tokenFor(const QUrl& url) const
{
    const QMutexLocker locker(&m_mutex);
    if (m_token.isEmpty() || !belongsToServer(url, m_serverUrl))
    {
        return {};
    }

    return m_token;
}

QNetworkAccessManager* createNetworkAccessManager(std::shared_ptr<const ServerCredentials> credentials,
                                                  const QString& cacheDirectory,
                                                  QObject* parent)
{
    auto* networkAccess = new AuthorizingNetworkAccessManager(std::move(credentials), parent);
    auto* cache = new QNetworkDiskCache(networkAccess);
    cache->setCacheDirectory(cacheDirectory);
    cache->setMaximumCacheSize(maximumCacheBytes);
    networkAccess->setCache(cache);
    return networkAccess;
}

std::unique_ptr<QQmlNetworkAccessManagerFactory>
createNetworkAccessFactory(std::shared_ptr<const ServerCredentials> credentials, const QString& cacheDirectory)
{
    return std::make_unique<NetworkAccessFactory>(std::move(credentials), cacheDirectory);
}

}  // namespace pictorio::network_access
