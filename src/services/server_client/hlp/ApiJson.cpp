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
    image.revision = reader.integer(QStringLiteral("revision"));
    return image;
}

TagCount tagCountFrom(JsonReader& reader)
{
    TagCount tagCount;
    tagCount.tag = reader.string(QStringLiteral("tag"));
    tagCount.imageCount = reader.integer(QStringLiteral("image_count"));
    return tagCount;
}

Namespace namespaceFrom(JsonReader& reader)
{
    Namespace info;
    info.name = reader.string(QStringLiteral("name"));
    info.singleValue = reader.boolean(QStringLiteral("single_value"));
    info.isBuiltin = reader.boolean(QStringLiteral("is_builtin"));
    info.inPathTemplate = reader.boolean(QStringLiteral("in_path_template"));
    info.allowedValues = reader.optionalStringList(QStringLiteral("allowed_values"));
    return info;
}

JobStatus jobStatusFrom(JsonReader& reader)
{
    const QString status = reader.string(QStringLiteral("status"));
    if (status == QLatin1String("running"))
    {
        return JobStatus::Running;
    }

    if (status == QLatin1String("done"))
    {
        return JobStatus::Done;
    }

    if (status == QLatin1String("failed"))
    {
        return JobStatus::Failed;
    }

    if (status != QLatin1String("queued") && reader.isValid())
    {
        reader.reject(QStringLiteral("status"), QStringLiteral("unknown status %1").arg(status));
    }

    return JobStatus::Queued;
}

Job jobFieldsFrom(JsonReader& reader)
{
    Job job;
    job.id = reader.integer(QStringLiteral("id"));
    job.kind = reader.string(QStringLiteral("kind"));
    job.status = jobStatusFrom(reader);
    job.params = reader.object(QStringLiteral("params"));
    job.progressDone = reader.integer(QStringLiteral("progress_done"));
    job.progressTotal = reader.optionalInteger(QStringLiteral("progress_total"));
    job.result = reader.optionalObject(QStringLiteral("result"));
    job.error = reader.optionalString(QStringLiteral("error"));
    job.createdBy = reader.optionalInteger(QStringLiteral("created_by"));
    job.createdAt = reader.dateTime(QStringLiteral("created_at"));
    job.finishedAt = reader.optionalDateTime(QStringLiteral("finished_at"));
    return job;
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

// The items of a JSON list, each read by readItem; or the first error, which names the item, e.g. "items[3].id".
template <typename T>
Result<QList<T>> listFrom(const QJsonValue& json, const QString& context, T (*readItem)(JsonReader&))
{
    if (!json.isArray())
    {
        const QString name = context.isEmpty() ? QStringLiteral("the answer") : context;
        return std::unexpected(invalidResponse(QStringLiteral("%1: not a list").arg(name)));
    }

    QList<T> items;
    const QJsonArray array = json.toArray();
    for (qsizetype index = 0; index < array.size(); ++index)
    {
        JsonReader reader(array.at(index).toObject(), QStringLiteral("%1[%2]").arg(context).arg(index));
        const T item = readItem(reader);
        if (!reader.isValid())
        {
            return std::unexpected(invalidResponse(reader.error()));
        }

        items.append(item);
    }

    return items;
}

// For answers of the form {"items": [...]}
template <typename T> Result<QList<T>> itemsFrom(const QJsonValue& json, T (*readItem)(JsonReader&))
{
    return listFrom(json.toObject().value(QStringLiteral("items")), QStringLiteral("items"), readItem);
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

Result<QList<DeviceToken>> devicesFrom(const QJsonValue& json)
{
    return listFrom(json, QString(), deviceTokenFrom);
}

Result<Image> imageFrom(const QJsonValue& json)
{
    JsonReader reader(json.toObject());
    const Image image = imageFieldsFrom(reader);
    return checked(reader, image);
}

Result<ImagePage> imagePageFrom(const QJsonValue& json)
{
    const Result<QList<Image>> items = itemsFrom(json, imageFieldsFrom);
    if (!items.has_value())
    {
        return std::unexpected(items.error());
    }

    JsonReader reader(json.toObject());
    ImagePage page;
    page.items = *items;
    page.nextCursor = reader.optionalString(QStringLiteral("next_cursor"));  // null on the last page
    page.effectiveQuery = reader.string(QStringLiteral("effective_query"));
    return checked(reader, page);
}

Result<LocalLibrary> localLibraryFrom(const QJsonValue& json)
{
    JsonReader reader(json.toObject());
    LocalLibrary library;
    library.libraryDir = reader.string(QStringLiteral("library_dir"));
    return checked(reader, library);
}

Result<ImageCount> imageCountFrom(const QJsonValue& json)
{
    JsonReader reader(json.toObject());
    ImageCount found;
    found.count = reader.integer(QStringLiteral("count"));
    found.effectiveQuery = reader.string(QStringLiteral("effective_query"));
    return checked(reader, found);
}

Result<int> updatedCountFrom(const QJsonValue& json)
{
    JsonReader reader(json.toObject());
    const int updated = reader.integer(QStringLiteral("updated"));
    return checked(reader, updated);
}

Result<QList<TagCount>> tagCountsFrom(const QJsonValue& json)
{
    return itemsFrom(json, tagCountFrom);
}

Result<QList<Namespace>> namespacesFrom(const QJsonValue& json)
{
    return itemsFrom(json, namespaceFrom);
}

Result<Preferences> preferencesFrom(const QJsonValue& json)
{
    JsonReader reader(json.toObject());
    Preferences preferences;
    preferences.hiddenFilters = reader.stringList(QStringLiteral("hidden_filters"));
    return checked(reader, preferences);
}

Result<Job> jobFrom(const QJsonValue& json)
{
    JsonReader reader(json.toObject());
    const Job job = jobFieldsFrom(reader);
    return checked(reader, job);
}

Result<QList<Job>> jobsFrom(const QJsonValue& json)
{
    return listFrom(json, QString(), jobFieldsFrom);
}

}  // namespace pictorio::server_client
