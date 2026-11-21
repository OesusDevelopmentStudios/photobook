/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <KConfigGroup>
#include <KSharedConfig>

#include "Settings.hpp"

namespace pictorio::settings
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

private:
    KConfigGroup slideshowGroup() const;
    void writeSlideshowEntry(const char* key, const QVariant& value);
    std::optional<QString> readServerEntry(const char* key) const;
    void writeServerEntry(const char* key, const QString& value);
    KConfigGroup libraryGroup() const;
    void writeLibraryEntry(const char* key, const QVariant& value);

    KSharedConfigPtr m_config;
};

}  // namespace pictorio::settings
