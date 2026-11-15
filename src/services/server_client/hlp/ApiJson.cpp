/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "hlp/ApiJson.hpp"

#include <QJsonArray>
#include <QJsonObject>

#include "hlp/JsonReader.hpp"
#include "hlp/ProblemDetails.hpp"

namespace
{

using namespace pictorio::server_client;

Role roleFrom(JsonReader& reader)
{
    const QString role = reader.string(QStringLiteral("role"));
    if (role == QLatin1String("admin"))
    {
        return Role::Admin;
    }

    if (role != QLatin1String("user") && reader.isValid())
    {
        reader.reject(QStringLiteral("role"), QStringLiteral("unknown role %1").arg(role));
    }

    return Role::User;
}

UserSummary userSummaryFrom(JsonReader& reader)
{
    UserSummary user;
    user.id = reader.integer(QStringLiteral("id"));
    user.username = reader.string(QStringLiteral("username"));
    user.role = roleFrom(reader);
    return user;
}

DeviceToken deviceTokenFrom(JsonReader& reader)
{
    DeviceToken device;
    device.tokenId = reader.integer(QStringLiteral("token_id"));
    device.deviceName = reader.string(QStringLiteral("device_name"));
    device.createdAt = reader.dateTime(QStringLiteral("created_at"));
    device.lastUsedAt = reader.dateTime(QStringLiteral("last_used_at"));
    device.expiresAt = reader.optionalDateTime(QStringLiteral("expires_at"));
    device.isCurrent = reader.boolean(QStringLiteral("is_current"));
    return device;
}

TakenAtSource takenAtSourceFrom(JsonReader& reader)
{
    const QString source = reader.string(QStringLiteral("taken_at_source"));
    if (source == QLatin1String("exif"))
    {
        return TakenAtSource::Exif;
    }

    if (source == QLatin1String("file_mtime"))
    {
        return TakenAtSource::FileMtime;
    }

    if (source != QLatin1String("import_time") && reader.isValid())
    {
        reader.reject(QStringLiteral("taken_at_source"), QStringLiteral("unknown source %1").arg(source));
    }

    return TakenAtSource::ImportTime;
}

Image imageFieldsFrom(JsonReader& reader)
{
    Image image;
    image.id = reader.integer(QStringLiteral("id"));
    image.sha256 = reader.string(QStringLiteral("sha256"));
    image.originalFilename = reader.string(QStringLiteral("original_filename"));
    image.storagePath = reader.string(QStringLiteral("storage_path"));
    image.mimeType = reader.string(QStringLiteral("mime_type"));
    image.width = reader.integer(QStringLiteral("width"));
    image.height = reader.integer(QStringLiteral("height"));
    image.fileSize = reader.integer64(QStringLiteral("file_size"));
    image.takenAt = reader.wallClockTime(QStringLiteral("taken_at"));
    image.takenAtSource = takenAtSourceFrom(reader);
    image.importedAt = reader.dateTime(QStringLiteral("imported_at"));
    image.updatedAt = reader.dateTime(QStringLiteral("updated_at"));
    image.tags = reader.stringList(QStringLiteral("tags"));
    return image;
}

// The parsed value, or the reader's first error.
template <typename T> Result<T> checked(const JsonReader& reader, T value)
{
    if (!reader.isValid())
    {
        return std::unexpected(invalidResponse(reader.error()));
    }

    return value;
}

}  // namespace

namespace pictorio::server_client
{

Result<ServerInfo> serverInfoFrom(const QJsonValue& json)
{
    JsonReader reader(json.toObject());
    ServerInfo info;
    info.name = reader.string(QStringLiteral("name"));
    info.version = reader.string(QStringLiteral("version"));
    info.apiVersion = reader.integer(QStringLiteral("api_version"));
    info.setupRequired = reader.boolean(QStringLiteral("setup_required"));
    return checked(reader, info);
}

Result<Login> loginFrom(const QJsonValue& json)
{
    JsonReader reader(json.toObject());
    Login login;
    login.tokenId = reader.integer(QStringLiteral("token_id"));
    login.token = reader.string(QStringLiteral("token"));
    JsonReader userReader(reader.object(QStringLiteral("user")), QStringLiteral("user"));
    login.user = userSummaryFrom(userReader);
    if (!userReader.isValid())
    {
        return std::unexpected(invalidResponse(userReader.error()));
    }

    return checked(reader, login);
}

Result<CurrentUser> currentUserFrom(const QJsonValue& json)
{
    JsonReader reader(json.toObject());
    CurrentUser currentUser;
    currentUser.user = userSummaryFrom(reader);
    currentUser.restrictedTags = reader.stringList(QStringLiteral("restricted_tags"));
    return checked(reader, currentUser);
}

Result<Image> imageFrom(const QJsonValue& json)
{
    JsonReader reader(json.toObject());
    const Image image = imageFieldsFrom(reader);
    return checked(reader, image);
}

Result<ImagePage> imagePageFrom(const QJsonValue& json)
{
    JsonReader reader(json.toObject());
    ImagePage page;
    page.effectiveQuery = reader.string(QStringLiteral("effective_query"));
    const QJsonValue nextCursor = json.toObject().value(QStringLiteral("next_cursor"));
    if (!nextCursor.isNull())  // null on the last page
    {
        page.nextCursor = reader.string(QStringLiteral("next_cursor"));
    }

    const QJsonArray items = json.toObject().value(QStringLiteral("items")).toArray();
    if (!json.toObject().value(QStringLiteral("items")).isArray())
    {
        reader.reject(QStringLiteral("items"), QStringLiteral("not a list"));
    }

    for (qsizetype index = 0; index < items.size() && reader.isValid(); ++index)
    {
        JsonReader itemReader(items.at(index).toObject(), QStringLiteral("items[%1]").arg(index));
        page.items.append(imageFieldsFrom(itemReader));
        if (!itemReader.isValid())
        {
            return std::unexpected(invalidResponse(itemReader.error()));
        }
    }

    return checked(reader, page);
}

Result<int> updatedCountFrom(const QJsonValue& json)
{
    JsonReader reader(json.toObject());
    const int updated = reader.integer(QStringLiteral("updated"));
    return checked(reader, updated);
}

Result<QList<DeviceToken>> devicesFrom(const QJsonValue& json)
{
    if (!json.isArray())
    {
        return std::unexpected(invalidResponse(QStringLiteral("the device list is not a list")));
    }

    QList<DeviceToken> devices;
    const QJsonArray items = json.toArray();
    for (qsizetype index = 0; index < items.size(); ++index)
    {
        JsonReader reader(items.at(index).toObject(), QStringLiteral("[%1]").arg(index));
        const DeviceToken device = deviceTokenFrom(reader);
        if (!reader.isValid())
        {
            return std::unexpected(invalidResponse(reader.error()));
        }

        devices.append(device);
    }

    return devices;
}

}  // namespace pictorio::server_client
