/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "impl/KConfigSettings.hpp"

namespace
{

// As in the config file: a renamed key would lose the user's value.
const QString slideshowGroupName = QStringLiteral("Slideshow");
constexpr const char* intervalKey = "IntervalSeconds";
constexpr const char* orderKey = "Order";
const QString serverGroupName = QStringLiteral("Server");
constexpr const char* urlKey = "Url";
constexpr const char* usernameKey = "Username";
const QString libraryGroupName = QStringLiteral("Library");
constexpr const char* sidebarVisibleKey = "SidebarVisible";
constexpr const char* tileSizeKey = "TileSize";
constexpr const char* tabKey = "Tab";
constexpr const char* showLibraryTabKey = "ShowLibraryTab";
constexpr const char* showAlbumsTabKey = "ShowAlbumsTab";
const QString albumsGroupName = QStringLiteral("Albums");
constexpr const char* sortKey = "Sort";
constexpr const char* showWithHiddenImagesKey = "ShowWithHiddenImages";

}  // namespace

namespace photobook::settings
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

std::optional<bool> KConfigSettings::librarySidebarVisible() const
{
    const KConfigGroup group = libraryGroup();
    if (!group.hasKey(sidebarVisibleKey))
    {
        return std::nullopt;
    }

    return group.readEntry(sidebarVisibleKey, true);
}

void KConfigSettings::setLibrarySidebarVisible(bool visible)
{
    writeLibraryEntry(sidebarVisibleKey, visible);
}

std::optional<int> KConfigSettings::libraryTileSize() const
{
    const KConfigGroup group = libraryGroup();
    if (!group.hasKey(tileSizeKey))
    {
        return std::nullopt;
    }

    return group.readEntry(tileSizeKey, 0);
}

void KConfigSettings::setLibraryTileSize(int gridUnits)
{
    writeLibraryEntry(tileSizeKey, gridUnits);
}

std::optional<QString> KConfigSettings::libraryTab() const
{
    const KConfigGroup group = libraryGroup();
    if (!group.hasKey(tabKey))
    {
        return std::nullopt;
    }

    return group.readEntry(tabKey, QString());
}

void KConfigSettings::setLibraryTab(const QString& tab)
{
    writeLibraryEntry(tabKey, tab);
}

std::optional<bool> KConfigSettings::libraryTabShown() const
{
    return readLibraryFlag(showLibraryTabKey);
}

void KConfigSettings::setLibraryTabShown(bool shown)
{
    writeLibraryEntry(showLibraryTabKey, shown);
}

std::optional<bool> KConfigSettings::albumsTabShown() const
{
    return readLibraryFlag(showAlbumsTabKey);
}

void KConfigSettings::setAlbumsTabShown(bool shown)
{
    writeLibraryEntry(showAlbumsTabKey, shown);
}

std::optional<QString> KConfigSettings::albumSort() const
{
    const KConfigGroup group = albumsGroup();
    if (!group.hasKey(sortKey))
    {
        return std::nullopt;
    }

    return group.readEntry(sortKey, QString());
}

void KConfigSettings::setAlbumSort(const QString& sort)
{
    writeAlbumsEntry(sortKey, sort);
}

std::optional<bool> KConfigSettings::showAlbumsWithHiddenImages() const
{
    const KConfigGroup group = albumsGroup();
    if (!group.hasKey(showWithHiddenImagesKey))
    {
        return std::nullopt;
    }

    return group.readEntry(showWithHiddenImagesKey, false);
}

void KConfigSettings::setShowAlbumsWithHiddenImages(bool show)
{
    writeAlbumsEntry(showWithHiddenImagesKey, show);
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

KConfigGroup KConfigSettings::libraryGroup() const
{
    return m_config->group(libraryGroupName);
}

void KConfigSettings::writeLibraryEntry(const char* key, const QVariant& value)
{
    KConfigGroup group = libraryGroup();
    group.writeEntry(key, value);
    m_config->sync();
}

std::optional<bool> KConfigSettings::readLibraryFlag(const char* key) const
{
    const KConfigGroup group = libraryGroup();
    if (!group.hasKey(key))
    {
        return std::nullopt;
    }

    return group.readEntry(key, true);
}

KConfigGroup KConfigSettings::albumsGroup() const
{
    return m_config->group(albumsGroupName);
}

void KConfigSettings::writeAlbumsEntry(const char* key, const QVariant& value)
{
    KConfigGroup group = albumsGroup();
    group.writeEntry(key, value);
    m_config->sync();
}

}  // namespace photobook::settings
