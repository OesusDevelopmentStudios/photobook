/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "LibraryViewFactory.hpp"

namespace pictorio::library_view
{

std::unique_ptr<LibraryView> createLibraryView(settings::Settings* settings)
{
    return std::make_unique<LibraryView>(settings);
}

}  // namespace pictorio::library_view
