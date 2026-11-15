/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "impl/KConfigSettings.hpp"

namespace
{

// Group and key names as they appear in the config file
const QString slideshowGroupName = QStringLiteral("Slideshow");
constexpr const char* intervalKey = "IntervalSeconds";
constexpr const char* orderKey = "Order";
const QString serverGroupName = QStringLiteral("Server");
constexpr const char* urlKey = "Url";
constexpr const char* usernameKey = "Username";

}  // namespace

namespace pictorio::settings
{

KConfigSettings::KConfigSettings(KSharedConfigPtr config)
    : m_config(std::move(config))
{}

std::optional<int> KConfigSettings::slideshowIntervalSeconds() const
{
    const KConfigGroup group = slideshowGroup();
    if (!group.hasKey(intervalKey))
    {
        return std::nullopt;
    }

    return group.readEntry(intervalKey, 0);
}

void KConfigSettings::setSlideshowIntervalSeconds(int intervalSeconds)
{
    writeSlideshowEntry(intervalKey, intervalSeconds);
}

std::optional<QString> KConfigSettings::slideshowOrder() const
{
    const KConfigGroup group = slideshowGroup();
    if (!group.hasKey(orderKey))
    {
        return std::nullopt;
    }

    return group.readEntry(orderKey, QString());
}

void KConfigSettings::setSlideshowOrder(const QString& order)
{
    writeSlideshowEntry(orderKey, order);
}

std::optional<QUrl> KConfigSettings::serverUrl() const
{
    const std::optional<QString> url = readServerEntry(urlKey);
    if (!url.has_value())
    {
        return std::nullopt;
    }

    return QUrl(*url);
}

void KConfigSettings::setServerUrl(const QUrl& serverUrl)
{
    writeServerEntry(urlKey, serverUrl.toString());
}

std::optional<QString> KConfigSettings::serverUsername() const
{
    return readServerEntry(usernameKey);
}

void KConfigSettings::setServerUsername(const QString& username)
{
    writeServerEntry(usernameKey, username);
}

KConfigGroup KConfigSettings::slideshowGroup() const
{
    return m_config->group(slideshowGroupName);
}

void KConfigSettings::writeSlideshowEntry(const char* key, const QVariant& value)
{
    KConfigGroup group = slideshowGroup();
    group.writeEntry(key, value);
    m_config->sync();  // saved at once, so a crash does not lose the change
}

std::optional<QString> KConfigSettings::readServerEntry(const char* key) const
{
    const KConfigGroup group = m_config->group(serverGroupName);
    if (!group.hasKey(key))
    {
        return std::nullopt;
    }

    return group.readEntry(key, QString());
}

void KConfigSettings::writeServerEntry(const char* key, const QString& value)
{
    KConfigGroup group = m_config->group(serverGroupName);
    group.writeEntry(key, value);
    m_config->sync();
}

}  // namespace pictorio::settings
