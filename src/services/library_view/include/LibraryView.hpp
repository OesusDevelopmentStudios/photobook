/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <QObject>

#include "Settings.hpp"

namespace photobook::library_view
{

// The library page's layout, kept between runs
class LibraryView : public QObject
{
    Q_OBJECT

    Q_PROPERTY(Tab tab READ tab WRITE setTab NOTIFY tabChanged)
    Q_PROPERTY(bool libraryTabShown READ isLibraryTabShown WRITE setLibraryTabShown NOTIFY shownTabsChanged)
    Q_PROPERTY(bool albumsTabShown READ isAlbumsTabShown WRITE setAlbumsTabShown NOTIFY shownTabsChanged)
    Q_PROPERTY(bool sidebarVisible READ isSidebarVisible WRITE setSidebarVisible NOTIFY sidebarVisibleChanged)
    Q_PROPERTY(int tileSize READ tileSize WRITE setTileSize NOTIFY tileSizeChanged)
    Q_PROPERTY(int minimumTileSize READ minimumTileSizeForQml CONSTANT)
    Q_PROPERTY(int maximumTileSize READ maximumTileSizeForQml CONSTANT)

public:
    enum class Tab
    {
        Library,  // the images of a search
        Albums,
    };
    Q_ENUM(Tab)

    // Tile sizes in Kirigami grid units, so tiles scale with the system font
    static constexpr int minimumTileSize = 6;
    static constexpr int maximumTileSize = 24;
    static constexpr int defaultTileSize = 10;

    explicit LibraryView(settings::Settings* settings, QObject* parent = nullptr);

    Tab tab() const;       // the one used last, if it is shown; the Library tab at first
    void setTab(Tab tab);  // a hidden tab is not chosen

    // Settings → Sections: either tab can be hidden, not both; turning off the last one shown does nothing. The tab
    // shown then is the other one.
    bool isLibraryTabShown() const;
    void setLibraryTabShown(bool shown);
    bool isAlbumsTabShown() const;
    void setAlbumsTabShown(bool shown);

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
    void tabChanged();
    void shownTabsChanged();
    void sidebarVisibleChanged();
    void tileSizeChanged();

private:
    // The limits for QML, which cannot read static members
    int minimumTileSizeForQml() const;
    int maximumTileSizeForQml() const;

    settings::Settings* m_settings;
    bool isShown(Tab tab) const;
    void setShown(Tab tab, bool shown);

    Tab m_tab;
    bool m_libraryTabShown;
    bool m_albumsTabShown;
    bool m_sidebarVisible;
    int m_tileSize;
    qreal m_partialSteps = 0;  // from a touchpad, not yet a whole step
};

}  // namespace photobook::library_view
