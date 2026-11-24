/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <KConfigGroup>
#include <KSharedConfig>

#include "Settings.hpp"

namespace photobook::settings
{

class KConfigSettings : public Settings
{
public:
    explicit KConfigSettings(KSharedConfigPtr config);

    std::optional<int> slideshowIntervalSeconds() const override;
    void setSlideshowIntervalSeconds(int intervalSeconds) override;

    std::optional<QString> slideshowOrder() const override;
    void setSlideshowOrder(const QString& order) override;

    std::optional<QUrl> serverUrl() const override;
    void setServerUrl(const QUrl& serverUrl) override;
    std::optional<QString> serverUsername() const override;
    void setServerUsername(const QString& username) override;

    std::optional<bool> librarySidebarVisible() const override;
    void setLibrarySidebarVisible(bool visible) override;
    std::optional<int> libraryTileSize() const override;
    void setLibraryTileSize(int gridUnits) override;
    std::optional<QString> libraryTab() const override;
    void setLibraryTab(const QString& tab) override;
    std::optional<bool> libraryTabShown() const override;
    void setLibraryTabShown(bool shown) override;
    std::optional<bool> albumsTabShown() const override;
    void setAlbumsTabShown(bool shown) override;

    std::optional<bool> tagPaneOpen() const override;
    void setTagPaneOpen(bool open) override;
    std::optional<QStringList> foldedTagNamespaces() const override;
    void setFoldedTagNamespaces(const QStringList& namespaceNames) override;

    std::optional<QString> albumSort() const override;
    void setAlbumSort(const QString& sort) override;
    std::optional<bool> showAlbumsWithHiddenImages() const override;
    void setShowAlbumsWithHiddenImages(bool show) override;

private:
    KConfigGroup slideshowGroup() const;
    void writeSlideshowEntry(const char* key, const QVariant& value);
    std::optional<QString> readServerEntry(const char* key) const;
    void writeServerEntry(const char* key, const QString& value);
    KConfigGroup libraryGroup() const;
    void writeLibraryEntry(const char* key, const QVariant& value);
    std::optional<bool> readLibraryFlag(const char* key) const;
    KConfigGroup tagPaneGroup() const;
    void writeTagPaneEntry(const char* key, const QVariant& value);
    KConfigGroup albumsGroup() const;
    void writeAlbumsEntry(const char* key, const QVariant& value);

    KSharedConfigPtr m_config;
};

}  // namespace photobook::settings
