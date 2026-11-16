/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include "LibraryView.hpp"

namespace pictorio::library_view
{

// The settings are not owned and must outlive the library view.
std::unique_ptr<LibraryView> createLibraryView(settings::Settings* settings);

}  // namespace pictorio::library_view
