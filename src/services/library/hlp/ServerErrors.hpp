/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <QString>

#include "ApiTypes.hpp"

namespace photobook::library
{

// What to tell the user when the server cannot answer a request, or says why it refused it
QString describeError(const server_client::ApiError& error);

}  // namespace photobook::library
