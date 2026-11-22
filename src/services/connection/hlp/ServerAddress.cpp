/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "hlp/ServerAddress.hpp"

#include <KLocalizedString>
#include <QCoreApplication>
#include <QSysInfo>

namespace
{

// A usable server URL: http or https, with a host, and nothing after the path.
bool isServerUrl(const QUrl& url)
{
    const bool webScheme = url.scheme() == QLatin1String("http") || url.scheme() == QLatin1String("https");
    return url.isValid() && webScheme && !url.host().isEmpty() && !url.hasQuery() && !url.hasFragment();
}

}  // namespace

namespace photobook::connection
{

QList<QUrl> serverUrlCandidates(const QString& typedAddress)
{
    const QString address = typedAddress.trimmed();
    if (address.isEmpty())
    {
        return {};
    }

    if (address.contains(QLatin1String("://")))
    {
        const QUrl url(address, QUrl::StrictMode);
        return isServerUrl(url) ? QList<QUrl>{url} : QList<QUrl>{};
    }

    QList<QUrl> candidates;
    for (const QString& scheme : {QStringLiteral("https://"), QStringLiteral("http://")})
    {
        const QUrl url(scheme + address, QUrl::StrictMode);
        if (isServerUrl(url))
        {
            candidates.append(url);
        }
    }

    return candidates;
}

QUrl serverPage(const QUrl& serverUrl, const QString& path)
{
    QUrl page = serverUrl.adjusted(QUrl::StripTrailingSlash | QUrl::RemoveQuery | QUrl::RemoveFragment);
    page.setPath(page.path() + path);
    return page;
}

QString deviceName()
{
    const QString version = QCoreApplication::applicationVersion();
    const QString app = version.isEmpty() ? QStringLiteral("Photobook") : QStringLiteral("Photobook %1").arg(version);
    return QStringLiteral("%1 (%2)").arg(QSysInfo::machineHostName(), app);
}

QString describeServerError(const QString& code, const QString& title, const QString& detail)
{
    if (code == QLatin1String("not_authenticated"))
    {
        return i18nc("@info", "Wrong username or password.");
    }

    if (code == QLatin1String("too_many_attempts"))
    {
        return i18nc("@info", "Too many failed logins. Wait a few minutes, then try again.");
    }

    if (code == QLatin1String("network_error"))
    {
        return i18nc("@info %1 is the reason, e.g. Connection refused", "The server does not answer: %1", detail);
    }

    return detail.isEmpty() ? title : i18nc("@info %1 is a title, %2 an explanation", "%1. %2", title, detail);
}

}  // namespace photobook::connection
