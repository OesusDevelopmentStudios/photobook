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

using photobook::library_view::LibraryView;

// As stored in the settings: words, so the config file stays readable
const QString libraryWord = QStringLiteral("library");
const QString albumsWord = QStringLiteral("albums");

LibraryView::Tab otherThan(LibraryView::Tab tab)
{
    return tab == LibraryView::Tab::Albums ? LibraryView::Tab::Library : LibraryView::Tab::Albums;
}

int limitedTileSize(int gridUnits)
{
    return std::clamp(gridUnits, LibraryView::minimumTileSize, LibraryView::maximumTileSize);
}

}  // namespace

namespace photobook::library_view
{

LibraryView::LibraryView(settings::Settings* settings, QObject* parent)
    : QObject(parent)
    , m_settings(settings)
    , m_tab(m_settings->libraryTab() == albumsWord ? Tab::Albums : Tab::Library)
    , m_libraryTabShown(m_settings->libraryTabShown().value_or(true))
    , m_albumsTabShown(m_settings->albumsTabShown().value_or(true))
    , m_sidebarVisible(m_settings->librarySidebarVisible().value_or(true))
    , m_tileSize(limitedTileSize(m_settings->libraryTileSize().value_or(defaultTileSize)))
{
    if (!m_libraryTabShown && !m_albumsTabShown)
    {
        m_libraryTabShown = m_albumsTabShown = true;  // a settings file edited by hand: one must be shown
    }
}

LibraryView::Tab LibraryView::tab() const
{
    return isShown(m_tab) ? m_tab : otherThan(m_tab);
}

void LibraryView::setTab(Tab tab)
{
    if (tab == m_tab || !isShown(tab))
    {
        return;
    }

    const Tab tabBefore = this->tab();
    m_tab = tab;
    m_settings->setLibraryTab(tab == Tab::Albums ? albumsWord : libraryWord);
    if (this->tab() != tabBefore)
    {
        Q_EMIT tabChanged();
    }
}

bool LibraryView::isLibraryTabShown() const
{
    return m_libraryTabShown;
}

void LibraryView::setLibraryTabShown(bool shown)
{
    setShown(Tab::Library, shown);
}

bool LibraryView::isAlbumsTabShown() const
{
    return m_albumsTabShown;
}

void LibraryView::setAlbumsTabShown(bool shown)
{
    setShown(Tab::Albums, shown);
}

bool LibraryView::isShown(Tab tab) const
{
    return tab == Tab::Albums ? m_albumsTabShown : m_libraryTabShown;
}

void LibraryView::setShown(Tab tab, bool shown)
{
    if (shown == isShown(tab) || (!shown && !isShown(otherThan(tab))))
    {
        return;  // unchanged, or the last tab shown
    }

    const Tab tabBefore = this->tab();
    if (tab == Tab::Albums)
    {
        m_albumsTabShown = shown;
        m_settings->setAlbumsTabShown(shown);
    }
    else
    {
        m_libraryTabShown = shown;
        m_settings->setLibraryTabShown(shown);
    }

    Q_EMIT shownTabsChanged();
    if (this->tab() != tabBefore)
    {
        Q_EMIT tabChanged();
    }
}

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

}  // namespace photobook::library_view
