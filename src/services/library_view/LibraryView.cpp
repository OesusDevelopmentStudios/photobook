/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "LibraryView.hpp"

#include <algorithm>
#include <cmath>

#include "ApiTypes.hpp"

namespace
{

using pictorio::library_view::LibraryView;

int limitedTileSize(int gridUnits)
{
    return std::clamp(gridUnits, LibraryView::minimumTileSize, LibraryView::maximumTileSize);
}

}  // namespace

namespace pictorio::library_view
{

LibraryView::LibraryView(settings::Settings* settings, QObject* parent)
    : QObject(parent)
    , m_settings(settings)
    , m_sidebarVisible(m_settings->librarySidebarVisible().value_or(true))
    , m_tileSize(limitedTileSize(m_settings->libraryTileSize().value_or(defaultTileSize)))
{}

bool LibraryView::isSidebarVisible() const
{
    return m_sidebarVisible;
}

void LibraryView::setSidebarVisible(bool visible)
{
    if (visible != m_sidebarVisible)
    {
        m_sidebarVisible = visible;
        m_settings->setLibrarySidebarVisible(visible);
        Q_EMIT sidebarVisibleChanged();
    }
}

int LibraryView::tileSize() const
{
    return m_tileSize;
}

void LibraryView::setTileSize(int gridUnits)
{
    const int tileSize = limitedTileSize(gridUnits);
    if (tileSize != m_tileSize)
    {
        m_tileSize = tileSize;
        m_settings->setLibraryTileSize(tileSize);
        Q_EMIT tileSizeChanged();
    }
}

void LibraryView::resizeTilesBySteps(qreal steps)
{
    m_partialSteps += steps;
    const int wholeSteps = static_cast<int>(std::trunc(m_partialSteps));
    m_partialSteps -= wholeSteps;

    const int newTileSize = m_tileSize + wholeSteps;
    if (newTileSize != limitedTileSize(newTileSize))
    {
        m_partialSteps = 0;  // at a limit, so turning back responds at once
    }

    setTileSize(newTileSize);
}

bool LibraryView::needsLargeThumbnail(qreal tilePixels) const
{
    return tilePixels > static_cast<int>(server_client::ThumbnailSize::Small);
}

int LibraryView::minimumTileSizeForQml() const
{
    return minimumTileSize;
}

int LibraryView::maximumTileSizeForQml() const
{
    return maximumTileSize;
}

}  // namespace pictorio::library_view
