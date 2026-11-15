/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include "ApiTypes.hpp"
#include "HttpTransport.hpp"

namespace pictorio::server_client
{

// The error a failed response stands for: the server's Problem Details, or a network error if no
// answer arrived. An answer without Problem Details (e.g. a proxy's error page) gets the code "http_<status>".
ApiError apiErrorFrom(const HttpResponse& response);

// The server answered, but not as the API promises.
ApiError invalidResponse(const QString& reason);

}  // namespace pictorio::server_client
