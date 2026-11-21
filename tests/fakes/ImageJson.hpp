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
    };
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
                            {QStringLiteral("effective_query"), QStringLiteral("-rating:nsfw")},
                        });
}

}  // namespace photobook::fakes
