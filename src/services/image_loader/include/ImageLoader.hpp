/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <expected>

#include <QByteArray>
#include <QDateTime>
#include <QImageIOHandler>
#include <QSize>
#include <QString>

namespace photobook::image_loader
{

struct ImageInfo
{
    QString filePath;   // absolute, cleaned up
    QSize displaySize;  // after the EXIF orientation: a portrait photo stored sideways is portrait here
    QImageIOHandler::Transformations exifTransformation = QImageIOHandler::TransformationNone;  // the viewer applies it
    QString mimeType;  // from the file content, not its name
    bool animated = false;
    QDateTime captureDate;  // invalid if the EXIF data does not say
};

// describeError() gives a message for the user.
enum class ImageLoadError
{
    FileNotFound,
    NotAFile,  // e.g. a folder
    PermissionDenied,
    UnsupportedFormat,  // not an image, or an image format this system cannot decode
    InvalidImageData,   // the format is known but the file is damaged
};

// Does not decode the pixels.
class ImageLoader
{
public:
    virtual ~ImageLoader();

    virtual std::expected<ImageInfo, ImageLoadError> loadInfo(const QString& filePath) const = 0;

    // Only ImageInfo::captureDate; much faster than loadInfo(). Thread-safe.
    virtual QDateTime readCaptureDate(const QString& filePath) const = 0;
};

QString describeError(ImageLoadError error);

}  // namespace photobook::image_loader
