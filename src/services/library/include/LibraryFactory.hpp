/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include "Library.hpp"

namespace photobook::library
{

// The connection is not owned and must outlive the library.
std::unique_ptr<Library> createLibrary(connection::Connection* connection);

}  // namespace photobook::library
