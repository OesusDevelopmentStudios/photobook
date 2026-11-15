/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include "Library.hpp"

namespace pictorio::library
{

// The connection is not owned and must outlive the library.
std::unique_ptr<Library> createLibrary(connection::Connection* connection);

}  // namespace pictorio::library
