/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include "ImageLoader.hpp"

namespace photobook::image_loader
{

class QtImageLoader : public ImageLoader
{
public:
    std::expected<ImageInfo, ImageLoadError> loadInfo(const QString& filePath) const override;
    QDateTime readCaptureDate(const QString& filePath) const override;
};

}  // namespace photobook::image_loader
