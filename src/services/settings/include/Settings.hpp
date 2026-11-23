/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <optional>

#include <QString>
#include <QUrl>

namespace photobook::settings
{

// Getters return nothing for values never stored, so each user of a setting decides its default. Changes are
// saved at once.
class Settings
{
public:
    virtual ~Settings();

    virtual std::optional<int> slideshowIntervalSeconds() const = 0;
    virtual void setSlideshowIntervalSeconds(int intervalSeconds) = 0;

    // A word such as "date", so the config file stays readable
    virtual std::optional<QString> slideshowOrder() const = 0;
    virtual void setSlideshowOrder(const QString& order) = 0;

    // The photobook-db server last connected to, and who logged in there. The login token itself is kept in
    // the system keyring, never here.
    virtual std::optional<QUrl> serverUrl() const = 0;
    virtual void setServerUrl(const QUrl& serverUrl) = 0;
    virtual std::optional<QString> serverUsername() const = 0;
    virtual void setServerUsername(const QString& username) = 0;

    virtual std::optional<bool> librarySidebarVisible() const = 0;
    virtual void setLibrarySidebarVisible(bool visible) = 0;
    // In Kirigami grid units, so tiles scale with the system font
    virtual std::optional<int> libraryTileSize() const = 0;
    virtual void setLibraryTileSize(int gridUnits) = 0;

    // A word such as "name", as for the slideshow's order
    virtual std::optional<QString> albumSort() const = 0;
    virtual void setAlbumSort(const QString& sort) = 0;
    // Albums with an image the user's hidden filters match are listed too
    virtual std::optional<bool> showAlbumsWithHiddenImages() const = 0;
    virtual void setShowAlbumsWithHiddenImages(bool show) = 0;
};

}  // namespace photobook::settings
