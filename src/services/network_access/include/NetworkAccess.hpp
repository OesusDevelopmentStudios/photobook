/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include <QMutex>
#include <QNetworkAccessManager>
#include <QQmlNetworkAccessManagerFactory>
#include <QString>
#include <QUrl>

namespace pictorio::network_access
{

// The login, for images QML loads straight from the server. QML loads images on other threads, so this may be
// used from any thread.
class ServerCredentials
{
public:
    void setLogin(const QUrl& serverUrl, const QString& token);
    void clear();

    // Empty for any address but the server's own
    QString tokenFor(const QUrl& url) const;

private:
    mutable QMutex m_mutex;
    QUrl m_serverUrl;
    QString m_token;
};

// Network access that sends the token to the server and keeps what it loads in a disk cache, under
// cacheDirectory. The server marks thumbnails as never changing, so they load from disk after the first time.
QNetworkAccessManager* createNetworkAccessManager(std::shared_ptr<const ServerCredentials> credentials,
                                                  const QString& cacheDirectory,
                                                  QObject* parent);

// The same for a QML engine (QQmlEngine::setNetworkAccessManagerFactory), which makes one per thread.
std::unique_ptr<QQmlNetworkAccessManagerFactory>
createNetworkAccessFactory(std::shared_ptr<const ServerCredentials> credentials, const QString& cacheDirectory);

}  // namespace pictorio::network_access
