/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "LibraryView.hpp"

namespace pictorio::library_view
{

LibraryView::LibraryView(settings::Settings* settings, QObject* parent)
    : QObject(parent)
    , m_settings(settings)
    , m_sidebarVisible(m_settings->librarySidebarVisible().value_or(true))
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

}  // namespace pictorio::library_view
