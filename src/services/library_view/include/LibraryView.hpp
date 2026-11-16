/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <QObject>

#include "Settings.hpp"

namespace pictorio::library_view
{

// How the library page is laid out, kept between runs: whether its sidebar is shown, and how large the tiles
// of the grid are.
class LibraryView : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool sidebarVisible READ isSidebarVisible WRITE setSidebarVisible NOTIFY sidebarVisibleChanged)
    Q_PROPERTY(int tileSize READ tileSize WRITE setTileSize NOTIFY tileSizeChanged)
    Q_PROPERTY(int minimumTileSize READ minimumTileSizeForQml CONSTANT)
    Q_PROPERTY(int maximumTileSize READ maximumTileSizeForQml CONSTANT)

public:
    // Tile sizes in Kirigami grid units, so tiles scale with the system font
    static constexpr int minimumTileSize = 6;
    static constexpr int maximumTileSize = 24;
    static constexpr int defaultTileSize = 10;

    // The settings are not owned and must outlive the library view.
    explicit LibraryView(settings::Settings* settings, QObject* parent = nullptr);

    bool isSidebarVisible() const;  // shown until the user hides it
    void setSidebarVisible(bool visible);

    int tileSize() const;
    void setTileSize(int gridUnits);  // limited to minimum..maximum

    // One step per mouse wheel notch or key press; negative makes the tiles smaller. A touchpad sends parts
    // of a step, which add up.
    Q_INVOKABLE void resizeTilesBySteps(qreal steps);

    // Whether a tile this many physical pixels wide needs the large thumbnail to look sharp
    Q_INVOKABLE bool needsLargeThumbnail(qreal tilePixels) const;

Q_SIGNALS:
    void sidebarVisibleChanged();
    void tileSizeChanged();

private:
    // The limits for QML, which cannot read static members
    int minimumTileSizeForQml() const;
    int maximumTileSizeForQml() const;

    settings::Settings* m_settings;
    bool m_sidebarVisible;
    int m_tileSize;
    qreal m_partialSteps = 0;  // from a touchpad, not yet a whole step
};

}  // namespace pictorio::library_view
