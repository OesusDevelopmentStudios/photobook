/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "fakes/FakeServer.hpp"

namespace photobook::fakes
{

inline server_client::HttpResponse jsonResponse(int status, const QJsonObject& json)
{
    return response(status, QJsonDocument(json).toJson(QJsonDocument::Compact));
}

// An image object with everything the client reads, e.g. original_filename "photo_7.jpg"
inline QJsonObject imageJson(int imageId)
{
    const QString moment = QStringLiteral("2026-11-15T19:22:03Z");
    return {
        {QStringLiteral("id"), imageId},
        {QStringLiteral("sha256"), QStringLiteral("aa")},
        {QStringLiteral("original_filename"), QStringLiteral("photo_%1.jpg").arg(imageId)},
        {QStringLiteral("storage_path"), QStringLiteral("photo/photo_%1.jpg").arg(imageId)},
        {QStringLiteral("mime_type"), QStringLiteral("image/jpeg")},
        {QStringLiteral("width"), 40},
        {QStringLiteral("height"), 30},
        {QStringLiteral("file_size"), 1},
        {QStringLiteral("taken_at"), QStringLiteral("2025-07-14T18:03:12")},
        {QStringLiteral("taken_at_source"), QStringLiteral("exif")},
        {QStringLiteral("imported_at"), moment},
        {QStringLiteral("updated_at"), moment},
        {QStringLiteral("tags"), QJsonArray()},
        {QStringLiteral("revision"), 0},
        {QStringLiteral("links"), QJsonObject()},
        {QStringLiteral("media_kind"), QStringLiteral("image")},
    };
}

// A video object, e.g. original_filename "video_7.mp4", recorded on 2025-07-14
inline QJsonObject videoJson(int imageId, qint64 durationMs)
{
    QJsonObject video = imageJson(imageId);
    video.insert(QStringLiteral("original_filename"), QStringLiteral("video_%1.mp4").arg(imageId));
    video.insert(QStringLiteral("mime_type"), QStringLiteral("video/mp4"));
    video.insert(QStringLiteral("taken_at_source"), QStringLiteral("container"));
    video.insert(QStringLiteral("media_kind"), QStringLiteral("video"));
    video.insert(QStringLiteral("duration_ms"), durationMs);
    video.insert(QStringLiteral("video"),
                 QJsonObject{{QStringLiteral("video_codec"), QStringLiteral("h264")},
                             {QStringLiteral("audio_codec"), QStringLiteral("aac")},
                             {QStringLiteral("bitrate"), 8120000},
                             {QStringLiteral("frame_rate"), 29.97}});
    return video;
}

// A page of a search; an empty cursor: the last page.
inline server_client::HttpResponse imagePageJson(const QList<int>& imageIds, const QString& nextCursor)
{
    QJsonArray items;
    for (const int imageId : imageIds)
    {
        items.append(imageJson(imageId));
    }

    return jsonResponse(200,
                        {
                            {QStringLiteral("items"), items},
                            {QStringLiteral("next_cursor"),
                             nextCursor.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(nextCursor)},
                            {QStringLiteral("effective_query"), QStringLiteral("-rating:private")},
                        });
}

}  // namespace photobook::fakes
