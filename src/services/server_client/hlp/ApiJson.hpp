/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <QJsonValue>
#include <QList>

#include "ApiTypes.hpp"

namespace pictorio::server_client
{

// The API's JSON answers as the client's types. Anything missing or of the wrong type gives an
// invalid_response error that names the field.
Result<ServerInfo> serverInfoFrom(const QJsonValue& json);
Result<Login> loginFrom(const QJsonValue& json);
Result<CurrentUser> currentUserFrom(const QJsonValue& json);
Result<QList<DeviceToken>> devicesFrom(const QJsonValue& json);

}  // namespace pictorio::server_client
