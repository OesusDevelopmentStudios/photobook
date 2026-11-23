/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "LibraryFactory.hpp"

namespace photobook::library
{

std::unique_ptr<Library> createLibrary(connection::Connection* connection)
{
    return std::make_unique<Library>(connection);
}

std::unique_ptr<Albums> createAlbums(connection::Connection* connection, settings::Settings* settings)
{
    return std::make_unique<Albums>(connection, settings);
}

}  // namespace photobook::library
