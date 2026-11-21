/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <QDateTime>
#include <QString>

namespace photobook::image_loader
{

// DateTimeOriginal, else DateTimeDigitized; invalid if neither can be read. EXIF dates carry no time zone:
// read as local time. Thread-safe.
QDateTime readExifCaptureDate(const QString& filePath);

}  // namespace photobook::image_loader
