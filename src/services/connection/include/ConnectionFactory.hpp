/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include "Connection.hpp"

namespace pictorio::connection
{

// The settings are not owned and must outlive the connection.
std::unique_ptr<Connection> createConnection(settings::Settings* settings);

}  // namespace pictorio::connection
