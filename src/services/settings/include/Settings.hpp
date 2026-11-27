/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <optional>

#include <QString>
#include <QStringList>
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
    // The library page's tab used last, a word such as "albums"
    virtual std::optional<QString> libraryTab() const = 0;
    virtual void setLibraryTab(const QString& tab) = 0;
    // Each tab can be hidden in the settings
    virtual std::optional<bool> libraryTabShown() const = 0;
    virtual void setLibraryTabShown(bool shown) = 0;
    virtual std::optional<bool> albumsTabShown() const = 0;
    virtual void setAlbumsTabShown(bool shown) = 0;

    // The viewer's tag pane: open or not, and the namespaces whose sections are folded
    virtual std::optional<bool> tagPaneOpen() const = 0;
    virtual void setTagPaneOpen(bool open) = 0;
    virtual std::optional<QStringList> foldedTagNamespaces() const = 0;
    virtual void setFoldedTagNamespaces(const QStringList& namespaceNames) = 0;

    // A word such as "name", as for the slideshow's order
    virtual std::optional<QString> albumSort() const = 0;
    virtual void setAlbumSort(const QString& sort) = 0;
    // Albums with an image the user's hidden filters match are listed too
    virtual std::optional<bool> showAlbumsWithHiddenImages() const = 0;
    virtual void setShowAlbumsWithHiddenImages(bool show) = 0;

    // The video player's volume (0 to 1) and speed (1 = normal), as last chosen
    virtual std::optional<double> videoVolume() const = 0;
    virtual void setVideoVolume(double volume) = 0;
    virtual std::optional<double> videoSpeed() const = 0;
    virtual void setVideoSpeed(double speed) = 0;
    // Every video starts without sound; the user can unmute it
    virtual std::optional<bool> startVideosMuted() const = 0;
    virtual void setStartVideosMuted(bool muted) = 0;
};

}  // namespace photobook::settings
