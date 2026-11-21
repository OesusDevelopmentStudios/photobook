/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include "FolderScanner.hpp"
#include "ImageLoader.hpp"

namespace photobook::folder_scanner
{

// The image loader provides the capture dates for SortOrder::Date.
std::unique_ptr<FolderScanner> createFolderScanner(std::unique_ptr<const image_loader::ImageLoader> imageLoader);

}  // namespace photobook::folder_scanner
