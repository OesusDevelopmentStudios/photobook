/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include "Albums.hpp"
#include "Library.hpp"

namespace photobook::library
{

// The connection is not owned and must outlive the library.
std::unique_ptr<Library> createLibrary(connection::Connection* connection);

// Neither is owned; both must outlive the albums.
std::unique_ptr<Albums> createAlbums(connection::Connection* connection, settings::Settings* settings);

}  // namespace photobook::library
