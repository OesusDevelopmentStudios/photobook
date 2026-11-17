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
Result<LocalLibrary> localLibraryFrom(const QJsonValue& json);
Result<Login> loginFrom(const QJsonValue& json);
Result<CurrentUser> currentUserFrom(const QJsonValue& json);
Result<QList<DeviceToken>> devicesFrom(const QJsonValue& json);
Result<Image> imageFrom(const QJsonValue& json);
Result<ImagePage> imagePageFrom(const QJsonValue& json);
Result<ImageCount> imageCountFrom(const QJsonValue& json);
Result<int> updatedCountFrom(const QJsonValue& json);  // of a bulk tag change
Result<QList<TagCount>> tagCountsFrom(const QJsonValue& json);
Result<QList<Namespace>> namespacesFrom(const QJsonValue& json);
Result<Preferences> preferencesFrom(const QJsonValue& json);
Result<Job> jobFrom(const QJsonValue& json);
Result<QList<Job>> jobsFrom(const QJsonValue& json);

}  // namespace pictorio::server_client
