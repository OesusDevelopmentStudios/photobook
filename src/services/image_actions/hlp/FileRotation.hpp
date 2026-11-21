/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <expected>

#include <QString>

namespace photobook::image_actions
{

enum class RotationSaving
{
    OrientationTag,  // only the EXIF orientation changes: JPEG and TIFF, whose tag Qt applies when showing them
    Pixels,          // the pixels are turned and saved again: lossless formats without that tag, PNG and BMP
    NotPossible,     // e.g. WebP (Qt ignores its tag, and it is usually lossy) or an animated GIF
};

RotationSaving rotationSavingFor(const QString& mimeType);

// Turns the image in the file clockwise by 90, 180 or 270 degrees from how it is shown now. The reason if
// it could not.
std::expected<void, QString> saveRotation(const QString& filePath, const QString& mimeType, int clockwiseDegrees);

}  // namespace photobook::image_actions
