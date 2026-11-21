/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include <QUrl>

#include "HttpTransport.hpp"
#include "ServerClient.hpp"

namespace photobook::server_client
{

// A client for the server at serverUrl, e.g. https://photos.example.org or http://raspberrypi.local:8737.
// A path in the URL is kept, for a server behind a proxy under e.g. /photobook.
std::unique_ptr<ServerClient> createServerClient(const QUrl& serverUrl);

// The same, sending its requests through the given transport, e.g. canned answers in tests.
std::unique_ptr<ServerClient> createServerClient(const QUrl& serverUrl, std::shared_ptr<HttpTransport> transport);

}  // namespace photobook::server_client
